#include "csgo_integration/RemoteConsole.h"

#include <functional>
#include <iostream>
#include <stdexcept>
#include <utility>

#include <asio/buffer.hpp>
#include <asio/connect.hpp>
#include <asio/error_code.hpp>
#include <asio/io_context.hpp>
#include <asio/read_until.hpp>
#include <asio/write.hpp>

using asio::ip::tcp;

using namespace csgo_integration;

RemoteConsole::RemoteConsole(size_t max_received_line_len)
    : _connected(false)
    , _connecting(false)
    , _failed_to_connect(false)
    , _disconnecting(false)
    , _async_op_abort_signal(true)
    , _last_error_msg("")
    , _io_context()
    , _socket(_io_context)
    , _read_buf_pos(0)
    , _discard_current_read_line(false) {
    // _read_buf will not change size throughout its lifetime and the
    // underlying memory must remain valid while read operations are running
    _read_buf.resize(max_received_line_len);
}

RemoteConsole::~RemoteConsole() {
    // Close socket
    Disconnect();
    // Wait for I/O thread to terminate
    if (_io_thread.joinable()) {
        std::cout << "[RCon] Joining thread...\n";
        _io_thread.join();
        std::cout << "[RCon] Joined thread!\n";
    }
}

bool RemoteConsole::IsConnecting() {
    return _connecting;
}

bool RemoteConsole::IsConnected() {
    return _connected;
}

bool RemoteConsole::HasFailedToConnect() {
    return _failed_to_connect;
}

bool RemoteConsole::IsDisconnecting() {
    return _disconnecting;
}

std::string RemoteConsole::GetLastErrorMessage() {
    std::string m;
    {
        std::lock_guard<std::mutex> lock(_last_error_msg_mutex);
        m = _last_error_msg; // Copy error msg
    }
    return m;
}

void RemoteConsole::StartConnecting(const std::string& host, int port) {
    // Make sure there's no connection already, and if it is, make sure it is
    // already being stopped
    if ((_connecting || _connected) && !_disconnecting)
        throw std::invalid_argument("RemoteConsole::StartConnecting(): "
            "We are already connected or trying to connect! Did you forget "
            "to call Disconnect() before calling StartConnecting() ?");

    // I/O thread is not joined if there was a previous connection
    if (_io_thread.joinable()) {
        std::cout << "[RCon] Joining thread...\n";
        _io_thread.join();
        std::cout << "[RCon] Joined thread!\n";
    }

    // Clear the message queues. No synchronization needed here since I/O thread
    // is not running.
    _read_msg_q.clear();
    _write_msg_q.clear();

    _connected = false;
    _connecting = true;
    _failed_to_connect = false;
    _disconnecting = false;
    _async_op_abort_signal = false;
    _last_error_msg = "";
    _discard_current_read_line = false;

    // If the io_context's run() method has been invoked before
    if (_io_context.stopped())
        // Prepare the io_context for a subsequent run() invocation.
        _io_context.restart();

    tcp::resolver r(_io_context);
    _endpoints = r.resolve(host, std::to_string(port));

    do_connect(_endpoints);

    // Start I/O thread that handles all asynchronous connects, reads,
    // writes and other handlers
    _io_thread = std::thread([this]() {
        // Blocks until all work has finished and there are no more handlers
        // to be dispatched, or until the io_context has been stopped. We
        // are always busy with reading the next line, so we're always
        // blocking here until an error occurs or we've been stopped.
        _io_context.run();
    });
}

void RemoteConsole::Disconnect() {
    // If sending a disconnect instruction makes sense
    if ((_connecting || _connected) && !_disconnecting) {
        _disconnecting = true;
        // Queue a socket close function that will be run by the I/O thread
        asio::post(
            _io_context,
            [this]() {
                // If socket is already closed (due to error), return
                if (_async_op_abort_signal) {
                    _disconnecting = false;
                    return;
                }
                // Close socket
                close_socket_gracefully();
            });
    }
    // I/O thread might still be running for a while after returning here!
}

std::deque<std::string> RemoteConsole::ReadLines() {
    std::deque<std::string> ret;
    {
        std::lock_guard<std::mutex> lock(_read_msg_q_mutex);
        _read_msg_q.swap(ret);
    }
    return ret;
}

void RemoteConsole::WriteLine(const std::string& msg) {
    if (!_connected || _async_op_abort_signal)
        return;

    // Copy and add new-line character
    std::string msg_nl = msg + '\n';

    // Start an asynchronous operation to add the message to the queue and
    // start the write operation if it isn't happening already.
    // asio::post() causes this function to be run by the I/O thread
    // -> No race conditions possible here
    asio::post(_io_context,
        [this, msg_nl]() {
            if (_async_op_abort_signal)
                return;

            bool write_in_progress = !_write_msg_q.empty();
            _write_msg_q.push_back(msg_nl);
            if (!write_in_progress)
                do_write();
        });
}

void RemoteConsole::WriteLines(const std::vector<std::string>& lines) {
    if (lines.size() == 0)
        return;

    size_t total_len = 0;
    for (auto l : lines) {
        total_len += l.length() + 1; // count line length + new-line char
    }
    total_len -= 1; // Subtract new-line char of final line

    std::string m;
    m.reserve(total_len); // Avoid reallocations
    for (size_t i = 0; i < lines.size(); ++i) {
        m += lines[i];
        // Last line gets its '\n' not here, but in the WriteLine() call
        if(i != lines.size() - 1)
            m += '\n';
    }
    WriteLine(m);
}

void RemoteConsole::close_socket_gracefully() {
    // Signal to async operations to abort
    _async_op_abort_signal = true;

    // Ignore errors that might happen upon closing the socket.
    // Even if an error occurs, the underlying descriptor is closed.
    asio::error_code ignored_error;
    // Disable read and write operations on the socket.
    _socket.shutdown(tcp::socket::shutdown_both, ignored_error);
    // Cancel any asynchronous read/write/connect operations immediately. They
    // will complete with the asio::error::operation_aborted error.
    _socket.close(ignored_error);

    // Signal to user thread: Socket is closed and disconnect is complete
    _connecting = false;
    _connected = false;
    // Make sure _disconnecting switches to false AFTER _connecting and
    // _connected switch to false, so that the user thread can know if the
    // connection will exist by checking
    // if ((IsConnecting() || IsConnected()) && !IsDisconnecting()) { ... }
    // That check could fail if _disconnecting would switch to false
    // BEFORE _connecting and _connected switch to false!
    _disconnecting = false;
}

void RemoteConsole::do_connect(const tcp::resolver::results_type& endpoints) {
    // Start the asynchronous connect operation.
    asio::async_connect(_socket, endpoints,
        [this](const asio::error_code& ec, const tcp::endpoint& ep) {
            if (_async_op_abort_signal)
                return;

            if (!ec) {
                _failed_to_connect = false;
                _connected = true;
                std::cout << "[RCon] Connected to " << ep << "\n";
                // Start listening on this connection
                _read_buf_pos = 0; // Discard previous buffer contents
                do_read();
            }
            else {
                std::string error_msg = ec.message();
                std::cout << "[RCon] Connect error: " << error_msg << "\n";
                {
                    std::lock_guard<std::mutex> lock(_last_error_msg_mutex);
                    _last_error_msg = std::move(error_msg);
                }
                _failed_to_connect = true;
            }

            // Make sure _connecting switches to false AFTER _connected
            // switched to true, so that the user thread can know the
            // connection exists or is being made by checking:
            // if (IsConnecting() || IsConnected()) { ... }
            // That check could fail if _connecting would switch to false
            // before _connected switches to true!
            _connecting = false;
        });
}

void RemoteConsole::do_read() {
    // Use the remaining buffer space that is not yet occupied by received
    // chars. If the buffer fills up entirely, without encountering a
    // new-line character, discard the entire buffer's content. We don't
    // want to become a memory hog once a faulty or malicious program sends
    // us tons of data that doesn't have new-line chars.
    auto remaining_buf = asio::buffer(
        _read_buf.data() + _read_buf_pos,  // remaining buf start
        _read_buf.size() - _read_buf_pos); // remaining buf size

    auto read_completion_handler =
    // n is the number of new chars added to _read_buf
    [this](const asio::error_code& ec, size_t n) {
        if (_async_op_abort_signal)
            return;

        if (ec == asio::error::eof) {
            std::cout << "[RCon] EOF (Connection was closed by CS:GO)\n";
            close_socket_gracefully();
            return;
        }
        if (ec) { // Any other error
            std::string error_msg = ec.message();
            std::cout << "[RCon] Read error: " << error_msg << "\n";
            {
                std::lock_guard<std::mutex> lock(_last_error_msg_mutex);
                _last_error_msg = std::move(error_msg);
            }
            close_socket_gracefully();
            return;
        }

        // Extract new-line delimited lines if possible
        const char* new_chars_end = _read_buf.data() + _read_buf_pos + n;
        const char* cur_line_start = _read_buf.data();
        const char* p = _read_buf.data() + _read_buf_pos;
        bool discard_entire_buffer = false;
        while (true) {
            // Find next new-line char position
            for (; p < new_chars_end; ++p)
                if (*p == '\n')
                    break;

            // If no more new-line chars can be found
            if (p == new_chars_end) {
                // If the buffer is full
                if (p == _read_buf.data() + _read_buf.size()) {
                    // If no lines were extracted
                    if (cur_line_start == _read_buf.data()) {
                        // Discard the entire buffer! There's no new-line char
                        // anywhere and we don't care about lines this long!
                        discard_entire_buffer = true;
                        // Discard every char we receive next until we get a NL
                        _discard_current_read_line = true;
                    }
                }
                break;
            }
            // Else, p points to the next new-line char.
            // Extract the line from the buffer (without new-line). If the
            // last char in the line is carriage-return, exclude it too.
            if (!_discard_current_read_line) {
                size_t line_len = p - cur_line_start;
                if (line_len > 0 && *(p - 1) == '\r')
                    line_len--;
                std::string line(cur_line_start, line_len);
                // Add line to the queue in a thread-safe fashion
                {
                    std::lock_guard<std::mutex> lock(_read_msg_q_mutex);
                    _read_msg_q.push_back(std::move(line));
                }
            }
            else {
                // We received a NL, now we can continue reading lines without
                // discarding them
                _discard_current_read_line = false;
            }

            p += 1; // Start checking again right after the new-line char
            cur_line_start = p; // Remember start of next line
        }

        if (discard_entire_buffer) {
            _read_buf_pos = 0;
        }
        else {
            // If NL chars were received, move all remaining chars after the
            // last NL to the beginning of the buffer
            if (cur_line_start > _read_buf.data()) {
                size_t move_count = new_chars_end - cur_line_start;
                for (size_t i = 0; i < move_count; ++i) {
                    _read_buf[i] = cur_line_start[i];
                }
                _read_buf_pos = move_count;
            }
            // Else, no NL chars were received and the buffer just grew a bit.
            else {
                _read_buf_pos += n;
            }
        }

        // Start the next read operation. By continously reading, we keep
        // the io_context object busy and prevent the I/O thread from
        // terminating until an error occurs or the socket is closed.
        do_read();
    };

    // Start an asynchronous operation to read an unspecified number of
    // bytes (at most as much as the remaining part of the buffer can hold).
    _socket.async_read_some(remaining_buf, read_completion_handler);
}

void RemoteConsole::do_write() {
    asio::async_write(_socket,
        asio::buffer(
            _write_msg_q.front().data(),
            _write_msg_q.front().length()),
        [this](asio::error_code ec, std::size_t /*length*/) {
            if (_async_op_abort_signal)
                return;

            if (ec == asio::error::eof) {
                std::cout << "[RCon] EOF (Connection was closed by CS:GO)\n";
                close_socket_gracefully();
                return;
            }
            if (ec) { // Any other error
                std::string error_msg = ec.message();
                std::cout << "[RCon] Write error: " << error_msg << "\n";
                {
                    std::lock_guard<std::mutex> lock(_last_error_msg_mutex);
                    _last_error_msg = std::move(error_msg);
                }
                close_socket_gracefully();
                return;
            }

            _write_msg_q.pop_front();
            // Start next write operation if possible
            if (!_write_msg_q.empty())
                do_write();
        });
}
