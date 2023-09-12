#ifndef GUI_GUI_H_
#define GUI_GUI_H_

#include <string>

#include <Corrade/Containers/ArrayView.h>
#include <Corrade/Containers/StringView.h>
#include <Corrade/Utility/Resource.h>
#include <Magnum/ImGuiIntegration/Context.hpp>
#include <Magnum/Tags.h>

#ifdef DZSIM_WEB_PORT
#include <Magnum/Platform/EmscriptenApplication.h>
#else
#include <Magnum/Platform/Sdl2Application.h>
#endif

#include "gui/GuiState.h"

#include "gui/MenuWindow.h"
#include "gui/Popup.h"

// IMPORTANT:
// Do not use more than one Gui instance, a lot of internal methods use static
// variables for convenience. They were written with the assumption that there
// will be only one Gui instance.

namespace gui {

class Gui {
public:
#ifdef DZSIM_WEB_PORT
    typedef Magnum::Platform::EmscriptenApplication Application;
#else
    typedef Magnum::Platform::Sdl2Application Application;
#endif

public:

    Gui(Application& app, Corrade::Utility::Resource& res, GuiState& state);

    void Init(
        const Corrade::Containers::ArrayView<const char>& font_data_disp = {},
        const Corrade::Containers::ArrayView<const char>& font_data_mono = {});

    void Draw();

    void HandleViewportEvent(Application::ViewportEvent& event);

    float GetTotalGuiScaling(); // Getter for _total_gui_scaling
    void UpdateGuiScaling(); // Must be called before ImGui::NewFrame()

    Magnum::ImGuiIntegration::Context _context{ Magnum::NoCreate };
    GuiState& state;

private:
    // Decide which characters each font must be able to draw later on. Latin
    // characters and some extensions are always included for each font.
    void BuildGlyphRanges(
        Corrade::Containers::StringView font_chars_disp = "",
        Corrade::Containers::StringView font_chars_mono = "");

    void SetUnscaledGuiStyle(ImGuiStyle& style);

    // GUI must be resized accordingly after calling this method
    void CalcNewTotalGuiScalingFactor();

    void LoadImGuiFonts(int size_pixels);

    // Should only be used after calling CalcNewTotalGuiScalingFactor()
    void UpdateGuiStyleScaling();

    void HelpMarker(const char* desc);

    void DrawCtrlHelpWindow();
    void DrawLegalNoticesWindow();

    std::string OpenBspFileDialog();


    Application& _app;
    Corrade::Utility::Resource& _resources;

    const float DEFAULT_FONT_SIZE = 26.0f;
    const float MIN_TOTAL_GUI_SCALING_FACTOR = 0.7f; // Smallest readable GUI size
    const float MAX_USER_GUI_SCALING_FACTOR = 2.5f; // Limit user GUI scale to sensible val

    // These 3 variables must only be changed by CalcNewTotalGuiScalingFactor()
    float _total_gui_scaling = 1.0f;
    float _extra_imgui_style_scaling = 1.0f;
    int _min_user_gui_scaling_factor_pct; // lower bound for _user_gui_scaling_factor_pct

    Corrade::Containers::ArrayView<const char> _imgui_disp_font_data = {};
    Corrade::Containers::ArrayView<const char> _imgui_mono_font_data = {};
    int _loaded_imgui_font_size_pixels = -1;
    // Character ranges that specify which chars the loaded font can draw
    ImVector<ImWchar> _glyph_ranges_disp;
    ImVector<ImWchar> _glyph_ranges_mono;
    // Select font with ImGui::PushFont(ImFont* f) and ImGui::PopFont() after.
    // If a font couldn't be loaded, its ImFont* member is NULL.
    // ImGui::PushFont(NULL) selects the default font.
    ImFont* _font_display = NULL;
    ImFont* _font_mono = NULL;


    bool _gui_scaling_update_required = false;
    
    Corrade::Containers::StringView _legal_notices;

    // -------- GUI modules --------
    MenuWindow _menu_window;
    Popup _popup;


    // Let modules access this class's private members
    friend class gui::MenuWindow;
    friend class gui::Popup;
};

} // namespace gui

#endif // GUI_GUI_H_
