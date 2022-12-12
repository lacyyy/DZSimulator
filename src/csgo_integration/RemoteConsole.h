#ifndef CSGO_INTEGRATION_REMOTECONSOLE_H_
#define CSGO_INTEGRATION_REMOTECONSOLE_H_

#include <atomic>
#include <deque>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <asio/ip/tcp.hpp>

namespace csgo_integration {

// An asynchronous tcp client that connects to the ingame console of a running
// CS:GO instance that was started with the launch option "-netconport <PORT>".
// Enables reading CS:GO console output and directly issuing console commands.
class RemoteConsole {
public:
    // Received lines that are longer than max_received_line_len (including CR
    // and NL chars) will be discarded.
    RemoteConsole(size_t max_received_line_len = 2048);
    // Terminates the connection if it exists.
    ~RemoteConsole();

    // This class isn't copyable or movable to avoid potential member troubles
    // (_thread and _socket)
    RemoteConsole(const RemoteConsole& other) = delete;
    RemoteConsole& operator=(const RemoteConsole& other) = delete;

    bool IsConnecting(); // If we are currently attempting to connect.
    bool IsConnected(); // If we can read from and send to the CS:GO console.
    bool HasFailedToConnect(); // If the attempt to connect failed.
    bool IsDisconnecting(); // If we're in the process of closing the connection
    std::string GetLastErrorMessage(); // Connection error message or empty string

    // Might block. Starts the asynchronous connect process. Throws an exception
    // if the following is true:
    // (IsConnecting() || IsConnected()) && !IsDisconnecting()
    void StartConnecting(const std::string& host, int port);

    // Non-blocking. Starts the asynchronous disconnect process if possible.
    void Disconnect();

    // Non-blocking. Dequeues and returns the newest received lines. Received
    // lines can be preceded by random text from previous lines that were too
    // long and had been cut off and discarded.
    std::deque<std::string> ReadLines();

    // Non-blocking. Writes the message and appends a new-line character.
    // Does nothing if IsConnected() returns false. If you will read your
    // command's console output, it is recommended to make sure it is always
    // preceded by a new-line because the output could get discarded if the
    // previous console line was too long and did not contain a NL char, causing
    // your command's output to get appended to it and get discarded with it!
    // Conversely, if your command's output does not have line breaks (NL
    // chars), you can screw up any following output lines.
    void WriteLine(const std::string& msg);

    // Non-blocking. Just like consecutive WriteLine() calls, but probably
    // achieves a higher write throughput.
    void WriteLines(const std::vector<std::string>& lines);

private:
    void close_socket_gracefully(); // Must only be called by the I/O thread

    void do_connect(const asio::ip::tcp::resolver::results_type& endpoints);
    void do_read();
    void do_write();

private:
    // If socket successfully connected
    std::atomic<bool> _connected;
    // If we are currently trying to connect to the socket
    std::atomic<bool> _connecting;
    // If connection could not be established
    std::atomic<bool> _failed_to_connect;
    // If we sent the socket close instruction, but the socket isn't closed yet
    std::atomic<bool> _disconnecting;
    // When true, tells all async operations to abort immediately
    std::atomic<bool> _async_op_abort_signal;

    // Last tcp connection error message, empty string indicates no error
    std::mutex _last_error_msg_mutex; // Protects _last_error_msg
    std::string _last_error_msg;

    asio::io_context _io_context; // Must be declared/initialized before _socket
    asio::ip::tcp::socket _socket; // Must be declared/initialized after _io_context
    asio::ip::tcp::resolver::results_type _endpoints;
    std::thread _io_thread;

    // The buffer being written to by the current read operation. The underlying
    // memory of this buffer must remain valid until the read operation's
    // completion handler is called. No more than one read operation must be
    // running at once. This string does not change size after this class's
    // constructor is called.
    std::string _read_buf;
    // _read_buf_pos can be interpreted in 2 ways: The number of received bytes
    // in _read_buf or the index in _read_buf of the byte that's received next.
    size_t _read_buf_pos;
    // We set this flag once we receive a line longer than the max length given
    // in the constructor. This flag tells us to discard every received char
    // until we receive a NL. Then we reset this flag and receive normally again.
    bool _discard_current_read_line;

    // Received messages, accessed by both the I/O thread and user thread
    // -> mutex required
    std::mutex _read_msg_q_mutex; // Protects _read_msg_q
    std::deque<std::string> _read_msg_q;

    // Messages to be written, only accessed by I/O thread -> no mutex required
    std::deque<std::string> _write_msg_q;
};

}

#endif // CSGO_INTEGRATION_REMOTECONSOLE_H_
