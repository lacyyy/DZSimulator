#ifndef GUI_HUD_H_
#define GUI_HUD_H_

namespace gui {

    class Gui;
    class GuiState;

    class Hud {
    public:
        Hud(Gui& gui);
        void Draw();

    private:

        void DrawSpeedometer();
        void DrawArrow(float x, float y, float scale);

        Gui& _gui;
        GuiState& _gui_state;
    };

}


#endif // GUI_HUD_H_
