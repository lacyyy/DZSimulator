#include "gui/Popup.h"

#include <cfloat>
#include <sstream>

#include "gui/Gui.h"
#include "gui/GuiState.h"

using namespace std::string_literals;
using namespace gui;

#define POPUP_MSG_TYPE_ID_INFO  "tINFO"
#define POPUP_MSG_TYPE_ID_WARN  "tWARN"
#define POPUP_MSG_TYPE_ID_ERROR "tERROR"

void GuiState::Popup::QueueMsgInfo(const std::string& msg) {
    const char* title = "INFO";
    OUT_msg_q.push_back(POPUP_MSG_TYPE_ID_INFO + "\0"s + title + "\0"s + msg);
}
void GuiState::Popup::QueueMsgWarn(const std::string& msg) {
    const char* title = "WARNING";
    OUT_msg_q.push_back(POPUP_MSG_TYPE_ID_WARN + "\0"s + title + "\0"s + msg);
}
void GuiState::Popup::QueueMsgError(const std::string& msg) {
    const char* title = "ERROR";
    OUT_msg_q.push_back(POPUP_MSG_TYPE_ID_ERROR + "\0"s + title + "\0"s + msg);
}

Popup::Popup(Gui& gui) : _gui(gui), _gui_state(gui.state)
{}

void Popup::Draw()
{
    const std::string POPUP_ID_SUFFIX = "##RegularPopup";
    const int MSG_TYPE_INFO = 0;
    const int MSG_TYPE_WARN = 1;
    const int MSG_TYPE_ERROR = 2;

    static int popup_msg_current_type = MSG_TYPE_INFO;
    static std::string popup_msg_cur_window_name = POPUP_ID_SUFFIX;
    static std::vector<std::string> popup_msg_cur_msg_lines = {};
    // For each line, a float for every char: The line width up until that char,
    // divided by font size
    static std::vector<std::vector<float>> popup_msg_cur_msg_cum_line_widths = {};

    // If game code signaled us to close the current popup
    bool current_popup_must_be_closed = false;
    if (_gui_state.popup.OUT_close_current) {
        _gui_state.popup.OUT_close_current = false;
        if (ImGui::IsPopupOpen(popup_msg_cur_window_name.c_str()))
            current_popup_must_be_closed = true;
    }

    if (!ImGui::IsPopupOpen(popup_msg_cur_window_name.c_str()) &&
        !_gui_state.popup.OUT_msg_q.empty()) {

        // Decode next popup msg
        std::string next = _gui_state.popup.OUT_msg_q.front();
        _gui_state.popup.OUT_msg_q.pop_front();
        std::vector<std::string> components;
        for (size_t i = 0, comp_start = 0; i <= next.size(); i++) {
            if (i == next.size() || next[i] == '\0') {
                components.push_back(next.substr(comp_start, i - comp_start));
                comp_start = i + 1;
            }
        }
        // Get popup msg type
        popup_msg_current_type = MSG_TYPE_INFO; // Default msg type
        if (components.size() >= 3) {
            if (components[0].compare(POPUP_MSG_TYPE_ID_WARN) == 0)
                popup_msg_current_type = MSG_TYPE_WARN;
            else if (components[0].compare(POPUP_MSG_TYPE_ID_ERROR) == 0)
                popup_msg_current_type = MSG_TYPE_ERROR;
        }
        // Get window name
        if (components.size() < 2)
            popup_msg_cur_window_name = "INFO" + POPUP_ID_SUFFIX;
        else if (components.size() == 2)
            popup_msg_cur_window_name = components[0] + POPUP_ID_SUFFIX;
        else
            popup_msg_cur_window_name = components[1] + POPUP_ID_SUFFIX;
        // Get popup msg
        popup_msg_cur_msg_lines.clear();
        popup_msg_cur_msg_cum_line_widths.clear();
        if (components.size() != 0) {
            // Separate msg with new-line chars
            size_t msg_comp_idx = 2;
            if (components.size() == 1) msg_comp_idx = 0;
            if (components.size() == 2) msg_comp_idx = 1;
            // Separate lines
            std::istringstream iss;
            iss.str(std::move(components[msg_comp_idx]));
            for (std::string line; std::getline(iss, line, '\n'); )
                popup_msg_cur_msg_lines.push_back(line);
            // Calculate char widths cumulatively
            ImFont* font = ImGui::GetFont();
            float font_size = ImGui::GetFontSize();
            for (const std::string& line : popup_msg_cur_msg_lines) {
                std::vector<float> cum_widths;
                cum_widths.reserve(line.size());

                const char* line_begin = line.c_str();
                const char* line_end = line.c_str() + line.size();
                float cum_width = 0.0f; // Cumulative line width, divided by font size
                for (const char* c = line_begin; c < line_end; c++) {
                    cum_width += font->CalcTextSizeA(font_size, FLT_MAX, 0.0f,
                        c, c + 1, NULL).x / font_size;
                    cum_widths.push_back(cum_width);
                }
                popup_msg_cur_msg_cum_line_widths.push_back(std::move(cum_widths));
            }
        }

        ImGui::OpenPopup(popup_msg_cur_window_name.c_str());
    }

    // Color popup depending on msg type
    size_t num_pushed_style_colors = 0;
    if (popup_msg_current_type == MSG_TYPE_WARN) {
        ImGui::PushStyleColor(ImGuiCol_TitleBgActive, (ImVec4)ImColor::HSV(0.167f, 0.7f, 0.5f));
        ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0.167f, 0.7f, 0.5f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(0.167f, 0.6f, 0.6f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(0.167f, 0.4f, 0.7f));
        num_pushed_style_colors = 4;
    }
    else if (popup_msg_current_type == MSG_TYPE_ERROR) {
        ImGui::PushStyleColor(ImGuiCol_TitleBgActive, (ImVec4)ImColor::HSV(0.0f, 0.7f, 0.5f));
        ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0.0f, 0.7f, 0.5f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(0.0f, 0.6f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(0.0f, 0.6f, 0.7f));
        num_pushed_style_colors = 4;
    }

    // Always center this window
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));

    ImGuiWindowFlags flags = ImGuiWindowFlags_AlwaysAutoResize;
    if (ImGui::BeginPopupModal(popup_msg_cur_window_name.c_str(), NULL, flags)) {
        // Determine how to break lines depending on screen aspect ratio
        ImVec2 work_area_size = ImGui::GetMainViewport()->WorkSize;
        float work_area_ratio = work_area_size.y < 1.0f ?
            1.0f : work_area_size.x / work_area_size.y;

        // For each original line, char pos after which to perform a line break
        auto line_breaks = std::vector<std::vector<size_t>>(popup_msg_cur_msg_lines.size());

        float line_height = ImGui::GetFontSize() + ImGui::GetStyle().ItemSpacing.y;

        float longest_line_width = 0.0f;

        // More iterations -> we get closer to the desired text aspect ratio
        const size_t LAYOUT_ITERATIONS = 6;
        float prev_target_text_width = 0.0f;
        for (size_t l_iter = 0; l_iter < LAYOUT_ITERATIONS; l_iter++) {
            // Calculate current text height
            float text_height = 0.0f;
            text_height += 5 * line_height; // Roughly account for popup title and button
            for (auto& breaks_in_line : line_breaks) // For every original line
                text_height += line_height * (1 + breaks_in_line.size());

            // Calculate desired text width
            longest_line_width = 0.0f; // reset
            float target_text_width = text_height * work_area_ratio;
            target_text_width = std::max(target_text_width, 0.30f * work_area_size.x);
            target_text_width = std::min(target_text_width, 0.95f * work_area_size.x);
            if (l_iter != 0) // Avoid overcompensation. Fails to converge otherwise.
                target_text_width = 0.5f * (target_text_width + prev_target_text_width);
            prev_target_text_width = target_text_width;
            // Work with text width regardless of the current font size
            float target_text_width_norm = target_text_width / ImGui::GetFontSize();

            // Recalculate line-breaks for every original line
            for (size_t i = 0; i < popup_msg_cur_msg_lines.size(); i++) {
                const std::string& orig_line = popup_msg_cur_msg_lines[i];
                std::vector<float>& cum_widths = popup_msg_cur_msg_cum_line_widths[i];
                std::vector<size_t>& breaks = line_breaks[i];
                breaks.clear();

                auto it_begin = cum_widths.begin();
                auto it_end = cum_widths.end();
                auto it_next_line_begin = it_begin;
                float cum_width_start = 0.0f;

                while (true) {
                    // Use binary search here because cumulative widths array is sorted
                    auto it_break = std::upper_bound(it_next_line_begin, it_end,
                        cum_width_start + target_text_width_norm);
                    if (it_break == it_end) {
                        // Before aborting, check line width
                        if (cum_widths.size() > 0) {
                            float current_line_width =
                                cum_widths[cum_widths.size() - 1] - cum_width_start;
                            if (current_line_width > longest_line_width)
                                longest_line_width = current_line_width;
                        }
                        break;
                    }

                    // Break line after char at this pos
                    size_t line_break_pos;
                    if (it_break == it_next_line_begin) // min 1 char per line
                        line_break_pos = std::distance(it_begin, it_break);
                    else
                        line_break_pos = std::distance(it_begin, it_break) - 1;

                    // Possibly move line break pos to avoid splitting a word
                    if (orig_line[line_break_pos] != ' '
                        && line_break_pos != orig_line.size() - 1) {

                        if (orig_line[line_break_pos + 1] == ' ')
                            line_break_pos++;
                        else {
                            // Find start of word we are splitting
                            // If word isn't preceded by a space char, just split it
                            size_t line_start_pos = std::distance(it_begin, it_next_line_begin);
                            for (long j = line_break_pos - 1; j >= (long)line_start_pos; j--) {
                                if (orig_line[j] == ' ') {
                                    line_break_pos = j;
                                    break;
                                }
                            }
                        }
                    }

                    // Remember longest line width
                    float current_line_width =
                        cum_widths[line_break_pos] - cum_width_start;
                    if (current_line_width > longest_line_width)
                        longest_line_width = current_line_width;

                    breaks.push_back(line_break_pos);
                    it_next_line_begin = it_begin + line_break_pos + 1;
                    cum_width_start = cum_widths[line_break_pos];
                }
            }
        }
        // Convert longest line width value to pixels
        longest_line_width *= ImGui::GetFontSize();

        // Draw text with previously determined line breaks
        for (size_t i = 0; i < popup_msg_cur_msg_lines.size(); i++) {
            const std::string& orig_line = popup_msg_cur_msg_lines[i];
            const char* oline_begin = orig_line.c_str();
            const char* oline_end = orig_line.c_str() + orig_line.size();
            const char* line_begin = oline_begin;
            for (size_t break_pos : line_breaks[i]) {
                const char* line_end = oline_begin + break_pos + 1;
                if (line_end < line_begin || line_end > oline_end) // safety
                    break;
                // Exclude last char if it's space
                if (line_end - line_begin >= 1 && *(line_end - 1) == ' ')
                    ImGui::TextUnformatted(line_begin, line_end - 1);
                else
                    ImGui::TextUnformatted(line_begin, line_end);
                line_begin = line_end;
            }
            ImGui::TextUnformatted(line_begin, oline_end);
        }

        //ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
        //ImGui::PopStyleVar();

        ImGui::NewLine();

        // Center the OK button
        float button_width = 200.0 * _gui._extra_imgui_style_scaling;
        float button_offset_x = 0.5f * (longest_line_width - button_width);
        if (button_offset_x < 0.0f)
            button_offset_x = 0.0f;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + button_offset_x);

        if (ImGui::Button("OK", ImVec2(button_width, 0))
            || current_popup_must_be_closed) {
            ImGui::CloseCurrentPopup();
            // Free memory
            popup_msg_cur_msg_lines.clear();
            popup_msg_cur_msg_cum_line_widths.clear();
        }
        ImGui::SetItemDefaultFocus();
        ImGui::EndPopup();
    }
    ImGui::PopStyleColor(num_pushed_style_colors);

    // If popup is open, give game code a signal (e.g. to release the cursor to
    // close the popup)
    _gui_state.popup.IN_visible = ImGui::IsPopupOpen(popup_msg_cur_window_name.c_str());
}
