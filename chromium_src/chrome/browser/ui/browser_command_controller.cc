#include "../../../../../../chrome/browser/ui/browser_command_controller.cc"

namespace chrome {

void BrowserCommandController::UpdateCommandsForTor() {
  if (!profile()->IsTorProfile())
    return;
  command_updater_.UpdateCommandEnabled(IDC_SHOW_BOOKMARK_BAR, false);
  command_updater_.UpdateCommandEnabled(IDC_OPTIONS, false);
  command_updater_.UpdateCommandEnabled(IDC_ABOUT, false);
  command_updater_.UpdateCommandEnabled(IDC_NEW_WINDOW, false);
  command_updater_.UpdateCommandEnabled(IDC_CLEAR_BROWSING_DATA, false);
  command_updater_.UpdateCommandEnabled(IDC_RECENT_TABS_MENU, false);
  command_updater_.UpdateCommandEnabled(IDC_SHOW_AVATAR_MENU, false);
  command_updater_.UpdateCommandEnabled(IDC_BOOKMARKS_MENU, false);
  command_updater_.UpdateCommandEnabled(IDC_SHOW_BOOKMARK_MANAGER, false);
  command_updater_.UpdateCommandEnabled(IDC_SHOW_HISTORY, false);
}

}  // namespace chrome
