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
        void ShowMovementRecreationDetails();
        void ShowKnownIssues();
        void ShowFeatureIdeas();

        void ShowOverlayLagAdvice();

        void DrawMapSelection();

        void DrawFirstPersonControls();

        void DrawVisualizations();

        void DrawGameConfig();

        void DrawPerformanceStats();

        void DrawVideoSettings();
        std::string GetDisplayName(int idx, int w, int h);

        void DrawMovementDebugging();

        void DrawCollisionDebugging();

        void DrawTestSettings();

        Gui& _gui;
        GuiState& _gui_state;
    };

}

#endif // GUI_MENUWINDOW_H_
