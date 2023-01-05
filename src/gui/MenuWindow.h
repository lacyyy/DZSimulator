#ifndef GUI_MENUWINDOW_H_
#define GUI_MENUWINDOW_H_

#include <string>

namespace gui {

    class Gui;
    class GuiState;

    class MenuWindow {
    public:
        MenuWindow(Gui& gui);
        void Draw();

    private:
        void ShowAppExplanation();
        void ShowTechnicalities();
        void ShowKnownIssues();
        void ShowPlannedFeatures();

        void ShowOverlayLagAdvice();

        void DrawMapSelection();

        void DrawVideoSettings();
        std::string GetDisplayName(int idx, int w, int h);

        void DrawTestSettings();

        Gui& _gui;
        GuiState& _gui_state;
    };

}

#endif // GUI_MENUWINDOW_H_
