#include "../../../../../../chrome/browser/profiles/profile_window.cc"

#include "brave/browser/profiles/brave_profile_manager.h"

namespace profiles {

#if !defined(OS_ANDROID)
void SwitchToTorProfile(ProfileManager::CreateCallback callback) {
  const base::FilePath& path = BraveProfileManager::GetTorProfilePath();
  // TODO: profile metrics for tor
  // ProfileMetrics::LogProfileSwitch(ProfileMetrics::SWITCH_PROFILE_GUEST,
  //                                  g_browser_process->profile_manager(),
  //                                  path);
  g_browser_process->profile_manager()->CreateProfileAsync(
      path, base::Bind(&profiles::OpenBrowserWindowForProfile,
                       callback,
                       false,
                       false),
      base::string16(), std::string());
}
#endif

void CloseTorProfileWindows() {
  ProfileManager* profile_manager = g_browser_process->profile_manager();
  Profile* profile = profile_manager->GetProfileByPath(
      BraveProfileManager::GetTorProfilePath());

  if (profile) {
    BrowserList::CloseAllBrowsersWithProfile(
        profile, base::Bind(&ProfileBrowserCloseSuccess),
        BrowserList::CloseCallback(), false);
  }
}

}  // namespace profiles
