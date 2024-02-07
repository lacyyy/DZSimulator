#include "gui/Gui.h"

#include <cmath>
#include <stdexcept>
#include <utility>
#include <vector>

#include <Tracy.hpp>

#include <Corrade/Utility/Path.h>
#include <Corrade/Utility/Resource.h>
#include <Magnum/ImGuiIntegration/Context.hpp>

#ifndef DZSIM_WEB_PORT
#include <portable-file-dialogs.h>
#endif

using namespace Magnum;
using namespace gui;

Gui::Gui(Application& app, Utility::Resource& res, GuiState& state)
    : _app{ app }
    , _resources { res }
    , state{ state }
    // Stay above lower bound by rounding up percentage points with ceil()
    , _min_user_gui_scaling_factor_pct
        { (int)std::ceil(MIN_TOTAL_GUI_SCALING_FACTOR * 100.0f) }
    , _menu_window{ *this }
    , _popup{ *this }
{
    // Make sure only one GUI instance is ever created
    static size_t s_gui_instance_cnt = 0;
    s_gui_instance_cnt++;
    if (s_gui_instance_cnt > 1)
        throw std::logic_error("ERROR: Tried to create a second Gui instance, "
            "there must only be one!");
}

void Gui::Init(
    const Corrade::Containers::ArrayView<const char>& font_data_disp,
    const Corrade::Containers::ArrayView<const char>& font_data_mono)
{
    ZoneScoped;

    _imgui_disp_font_data = font_data_disp;
    _imgui_mono_font_data = font_data_mono;

    // Application MUST display legal notices somewhere. The following code
    // crashes the application if it can't load the third party licenses file.
    _legal_notices = _resources.getString("LICENSES-THIRD-PARTY.txt");

    ImGui::CreateContext();

    // The legal text will be displayed using the monospace font -> Add all
    // the legal text's chars to the monospace font glyph range
    BuildGlyphRanges("", _legal_notices);
    
    // Disable automatic ImGui config file saving/loading for release builds
    // so end user isn't confused by .ini files appearing
    //if (strcmp(build_info::GetBuildTypeStr(), "Debug") != 0)
        ImGui::GetIO().IniFilename = NULL;

    CalcNewTotalGuiScalingFactor();
    UpdateGuiStyleScaling();

    // Load any font if available
    if (!_imgui_disp_font_data.isEmpty() || !_imgui_mono_font_data.isEmpty()) {
        // ImGui FAQ advises to round down font size
        int font_size = DEFAULT_FONT_SIZE * _total_gui_scaling;
        LoadImGuiFonts(font_size);
        _loaded_imgui_font_size_pixels = font_size;
    }

    // This lets ImGui react to different DPI scenarios. Additionally, it builds
    // the font atlas for all fonts that were added before.
    _context = ImGuiIntegration::Context(
        *ImGui::GetCurrentContext(),
        Vector2{ _app.windowSize() } / _app.dpiScaling(),
        _app.windowSize(),
        _app.framebufferSize());
}

void Gui::HandleViewportEvent(Application::ViewportEvent& /* event */)
{
    UpdateGuiScaling();
}

float Gui::GetTotalGuiScaling()
{
    return _total_gui_scaling;
}

void Gui::UpdateGuiScaling()
{
    CalcNewTotalGuiScalingFactor();
    UpdateGuiStyleScaling();

    if (!_imgui_disp_font_data.isEmpty() || !_imgui_mono_font_data.isEmpty()) {
        // Reload fonts if font size was changed by gui scale
        // ImGui FAQ advises to round down font size
        int new_font_size = DEFAULT_FONT_SIZE * _total_gui_scaling;
        if (new_font_size != _loaded_imgui_font_size_pixels) {
            ImGui::GetIO().Fonts->Clear(); // important
            LoadImGuiFonts(new_font_size); // Add resized fonts
            _loaded_imgui_font_size_pixels = new_font_size;
        }
    }

    // This lets ImGui react to different DPI scenarios. Additionally, it builds
    // the font atlas of the resized fonts we added just before.
    _context.relayout(
        Vector2{ _app.windowSize() } / _app.dpiScaling(),
        _app.windowSize(),
        _app.framebufferSize());
}

void Gui::BuildGlyphRanges(Corrade::Containers::StringView font_chars_disp,
    Corrade::Containers::StringView font_chars_mono)
{
    // The computed glyph ranges are passed to ImGui's AddFont*** functions, you
    // need to make sure that the ranges persist up until the atlas is build
    // (when calling GetTexData*** or Build(), which happens at
    // ImGuiIntegration::Context creation and its relayout() method)
    // ImGui only copies the pointer, not the data.
    // Since our glyph ranges are loaded throughout the application's lifetime,
    // this isn't a problem.
    
    ImFontGlyphRangesBuilder builder;
    _glyph_ranges_disp = ImVector<ImWchar>();
    _glyph_ranges_mono = ImVector<ImWchar>();

    // DISPLAY FONT CHARACTERS
    // Add default Latin chars + extensions
    builder.AddRanges(ImGui::GetIO().Fonts->GetGlyphRangesDefault());
    // Add extra chars from the supplied string
    if(!font_chars_disp.isEmpty())
        builder.AddText(font_chars_disp.data(),
            font_chars_disp.data() + font_chars_disp.size());
    builder.BuildRanges(&_glyph_ranges_disp);

    builder.Clear();

    // MONOSPACE FONT CHARACTERS
    // Add default Latin chars + extensions
    builder.AddRanges(ImGui::GetIO().Fonts->GetGlyphRangesDefault());
    // Add extra chars from the supplied string
    if (!font_chars_mono.isEmpty())
        builder.AddText(font_chars_mono.data(),
            font_chars_mono.data() + font_chars_mono.size());
    builder.BuildRanges(&_glyph_ranges_mono);
}

void Gui::SetUnscaledGuiStyle(ImGuiStyle& style)
{
    // Add style changes here, not scaled! Scaling is done somewhere else.
    style.WindowTitleAlign = ImVec2(0.5f, 0.5f); // Centered window titles
    style.DisabledAlpha = 0.2f; // Let user focus less on disabled elements
}

void Gui::CalcNewTotalGuiScalingFactor()
{
    Vector2i windowSize = _app.windowSize();
    // Accounts for system UI / DPI scaling, didn't test Linux and macOS
    float system_ui_factor =
        (float)(_app.framebufferSize().x())
        * (float)(_app.dpiScaling().x())
        / (float)(windowSize.x());

    // Let the GUI sensibly resize together with the window's size
    float window_aspect_ratio = (float)windowSize.x() / (float)windowSize.y();
    float window_size_factor;
    if (window_aspect_ratio < 16.0f / 9.0f) // GUI is intended for 16:9 screens
        window_size_factor = (float)(windowSize.x()) / 1920.0f;
    else
        window_size_factor = (float)(windowSize.y()) / 1080.0f;

    // Let the user adjust the GUI scale as well
    _total_gui_scaling = system_ui_factor * window_size_factor
        * (state.video.IN_user_gui_scaling_factor_pct / 100.0f);

    if (_total_gui_scaling < MIN_TOTAL_GUI_SCALING_FACTOR)
        _total_gui_scaling = MIN_TOTAL_GUI_SCALING_FACTOR;

    // Determine value of _user_gui_scaling_factor_pct that results in the
    // smallest allowed total GUI scale. Round up to stay above lower bound.
    _min_user_gui_scaling_factor_pct = std::ceil(
        100.0f * MIN_TOTAL_GUI_SCALING_FACTOR
        / (system_ui_factor * window_size_factor));

    // Commented to remember user value in case they accidentally resized window
    // and want to return to the previous window size and gui scaling value
    //if (_user_gui_scaling_factor < _min_user_gui_scaling_factor)
    //    _user_gui_scaling_factor = _min_user_gui_scaling_factor;

    // ImGui already handles system UI / DPI scaling at the creation of
    // ImGuiIntegration::Context() and its relayout() method
    // -> Scale ImGui style elements with all factors excluding the system factor
    _extra_imgui_style_scaling = _total_gui_scaling / system_ui_factor;
}

void Gui::LoadImGuiFonts(int size_pixels)
{
    // ImGui by default takes ownership of the passed data pointer and then
    // frees it (using what? free()?), that's why the non-const pointer.
    // We have to explicitly tell it to *not* do that, since the resources are
    // always in memory and on a static place.
    ImFontConfig fontConfig;
    fontConfig.FontDataOwnedByAtlas = false;

    // First font that gets added becomes the default font!
    // -> Let _font_display be the default font
    if (_imgui_disp_font_data.isEmpty()) {
        Debug{} << "[Gui] Couldn't load display font!";
        _font_display = NULL; // Trying to use NULL font gives the default font
    } else {
        _font_display = ImGui::GetIO().Fonts->AddFontFromMemoryTTF(
            const_cast<char*>(_imgui_disp_font_data.data()),
            _imgui_disp_font_data.size(),
            size_pixels,
            &fontConfig,
            _glyph_ranges_disp.Data);
    }

    if (_imgui_mono_font_data.isEmpty()) {
        Debug{} << "[Gui] Couldn't load monospace font!";
        _font_mono = NULL; // Trying to use NULL font gives the default font
    } else {
        _font_mono = ImGui::GetIO().Fonts->AddFontFromMemoryTTF(
            const_cast<char*>(_imgui_mono_font_data.data()),
            _imgui_mono_font_data.size(),
            size_pixels,
            &fontConfig,
            _glyph_ranges_mono.Data);
    }
}

void Gui::UpdateGuiStyleScaling()
{
    // Scale some GUI style variables
    // Create new style instance because scaling is lossy (integer rounding)
    ImGuiStyle unscaled_style = ImGuiStyle();
    SetUnscaledGuiStyle(unscaled_style); // Apply our own style
    // ImGui::GetStyle() can be modified during initialization and before
    // NewFrame(). During the frame, use ImGui::PushStyleVar(), PopStyleVar(),
    // ImGui::PushStyleColor() and PopStyleColor()
    ImGui::GetStyle() = unscaled_style;
    ImGui::GetStyle().ScaleAllSizes(_extra_imgui_style_scaling); // lossy scaling
}

void Gui::Draw()
{
    // Calls ImGui::NewFrame(), also decides if text input needs to be enabled
    _context.newFrame(); 

    // Enable text input, if needed
    if (ImGui::GetIO().WantTextInput && !_app.isTextInputActive())
        _app.startTextInput();
    else if (!ImGui::GetIO().WantTextInput && _app.isTextInputActive())
        _app.stopTextInput();

    ////// START GUI ELEMENTS
    _menu_window.Draw();

    // Only show control help if camera isn't controlled by CSGO data
    if (state.vis.IN_geo_vis_mode != state.vis.GLID_OF_CSGO_SESSION)
        DrawCtrlHelpWindow();
    
    if (state.show_window_legal_notices)
        DrawLegalNoticesWindow();

#ifndef NDEBUG
    // ImGui demo window is useful for development, but not needed for end user.
    // -> Exclude it from non-debug builds, this reduces binary size by ~0.3MB
    if (state.show_window_demo)
        ImGui::ShowDemoWindow();
#endif

    _popup.Draw();

    ////// END GUI ELEMENTS

    // Update application cursor
    //_context.updateApplicationCursor(_app); // App no longer seems to react to ESC

    _context.drawFrame();

    if (_gui_scaling_update_required) {
        _gui_scaling_update_required = false;
        UpdateGuiScaling(); // Can only be called in between frames
    }
}

void Gui::HelpMarker(const char* desc)
{
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered() && ImGui::BeginTooltip())
    {
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

void Gui::DrawCtrlHelpWindow()
{
    // -1=draggable,0=top left,1=top right,2=bottom left,3=bottom right
    static int corner = 3;
    ImGuiWindowFlags window_flags =
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoNav;

    if (corner != -1) {
        float PAD = 10.0f;
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        // Use work area to avoid menu-bar/task-bar, if any!
        ImVec2 work_pos = viewport->WorkPos;
        ImVec2 work_size = viewport->WorkSize;
        ImVec2 window_pos, window_pos_pivot;
        window_pos.x = (corner & 1) ? (work_pos.x + work_size.x - PAD) : (work_pos.x + PAD);
        window_pos.y = (corner & 2) ? (work_pos.y + work_size.y - PAD) : (work_pos.y + PAD);
        window_pos_pivot.x = (corner & 1) ? 1.0f : 0.0f;
        window_pos_pivot.y = (corner & 2) ? 1.0f : 0.0f;
        ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
        window_flags |= ImGuiWindowFlags_NoMove;
    }
    ImGui::SetNextWindowBgAlpha(0.35f); // Transparent background
    if (ImGui::Begin("[HIDDENTITLE] Ctrl Help Window", NULL, window_flags)) {
        if (state.ctrl_help.OUT_first_person_control_active) {
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f),
                "First Person Control ON");
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f),
                "(Toggle with ESC)");
        }
        else {
            ImGui::Text(
                "Toggle first person control mode with ESC. When enabled,\n"
                "move around with WASD, Ctrl, Space, Shift and your mouse!");
            ImGui::Separator();
            ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f),
                "First Person Control OFF");
        }
            
    }
    ImGui::End();
}

void Gui::DrawLegalNoticesWindow()
{
    // How much percent of the screen will be used for the license text box.
    // Stay below 100% to keep space for surrounding UI.
    float license_text_screen_fill_x = 0.8f;
    float license_text_screen_fill_y = 0.5f;

    ImGuiWindowFlags legal_window_flags = 0;
    legal_window_flags |= ImGuiWindowFlags_AlwaysAutoResize;
    legal_window_flags |= ImGuiWindowFlags_NoCollapse;
    legal_window_flags |= ImGuiWindowFlags_NoMove;

    // Center the window
    ImGui::SetNextWindowPos(
        ImVec2(
            ImGui::GetIO().DisplaySize.x * 0.5f,
            ImGui::GetIO().DisplaySize.y * 0.5f),
        ImGuiCond_Always,
        ImVec2(0.5f, 0.5f));

    if (!ImGui::Begin("Third Party Legal Notices", &state.show_window_legal_notices,
        legal_window_flags))
    {
        ImGui::End();
        return;
    }

    if (ImGui::Button("Copy everything to clipboard")) {
        // Copy license string into vector and add a null-terminator because
        // ImGui::SetClipboardText() expects a null-terminated string and
        // Corrade::Containers::StringView doesn't guarantee null-termination
        std::vector<char> license_c_str
            = std::vector<char>(_legal_notices.size() + 1);
        for (size_t i = 0; i < _legal_notices.size(); ++i)
            license_c_str[i] = _legal_notices[i];
        license_c_str[license_c_str.size() - 1] = '\0'; // Add null-terminator
        ImGui::SetClipboardText(license_c_str.data());
    }

    ImGuiWindowFlags child_window_flags = 0;
    child_window_flags |= ImGuiWindowFlags_HorizontalScrollbar;
    child_window_flags |= ImGuiWindowFlags_AlwaysHorizontalScrollbar;
    child_window_flags |= ImGuiWindowFlags_AlwaysVerticalScrollbar;
    
    ImGui::BeginChild("textbox_thirdparty",
        ImVec2(
            license_text_screen_fill_x * ImGui::GetMainViewport()->WorkSize.x,
            license_text_screen_fill_y * ImGui::GetMainViewport()->WorkSize.y),
        ImGuiChildFlags_Border,
        child_window_flags);

    ImGui::PushFont(_font_mono); // Select monospace font for legal text
    ImGui::TextUnformatted(
        _legal_notices.data(),
        _legal_notices.data() + _legal_notices.size());
    ImGui::PopFont();

    ImGui::EndChild();

    if (ImGui::Button("Close")) state.show_window_legal_notices = false;

    ImGui::End();
}

// Returns path with forward slash directory separators
std::string Gui::OpenBspFileDialog()
{
#ifdef DZSIM_WEB_PORT
    return {};
#else
    if (!pfd::settings::available()) {
        Error{} << "[ERR] pfd not available!";
        return {}; // Return empty string
    }

    // Blocks for user action
    std::vector<std::string> selection = pfd::open_file(
        "Select a CS:GO map to open",
        ".",
        { "CS:GO map file (*.bsp)", "*.bsp" }).result();

    if (selection.empty()) {
        Debug{} << "[Gui] No map file was selected!";
        return {}; // Return empty string
    }

    std::string path = Corrade::Utility::Path::fromNativeSeparators(selection[0]);
    Debug{} << "[Gui] Selected map file to open:" << path.c_str();
    return path;
#endif
}
