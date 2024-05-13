#ifndef SAVEDUSERDATAHANDLER_H_
#define SAVEDUSERDATAHANDLER_H_

#include <Corrade/Containers/Optional.h>
#include <Corrade/Containers/String.h>

#include "gui/GuiState.h"

namespace SavedUserDataHandler {

    // Returns Corrade::Containers::NullOpt if it fails
    Corrade::Containers::Optional<Corrade::Containers::String> GetAbsoluteSaveFilePath();

    void OpenSaveFileDirectoryInFileExplorer();

    gui::GuiState LoadUserSettingsFromFile();

    void SaveUserSettingsToFile(const gui::GuiState& gui_state);

    // Resets all fields of the GuiState, that get saved to file, to their
    // default values.
    void ResetUserSettingsToDefaults(gui::GuiState& gui_state);

}

#endif // SAVEDUSERDATAHANDLER_H_
