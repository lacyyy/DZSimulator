#ifndef GUI_POPUP_H_
#define GUI_POPUP_H_

namespace gui {

    class Gui;
    class GuiState;

    class Popup {
    public:
        Popup(Gui& gui);
        void Draw();

    private:
        Gui& _gui;
        GuiState& _gui_state;
    };

}

#endif // GUI_POPUP_H_
