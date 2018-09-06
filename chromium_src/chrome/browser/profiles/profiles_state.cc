#include "brave/browser/profiles/brave_profile_manager.h"
#include "chrome/browser/profiles/profiles_state.h"
#define SetActiveProfileToGuestIfLocked SetActiveProfileToGuestIfLocked_ChromiumImpl
#include "../../../../../../chrome/browser/profiles/profiles_state.cc"
#undef SetActiveProfileToGuestIfLocked

namespace profiles {
#if !defined(OS_CHROMEOS)
bool SetActiveProfileToGuestIfLocked() {
  ProfileManager* profile_manager = g_browser_process->profile_manager();

  const base::FilePath& active_profile_path =
      profile_manager->GetLastUsedProfileDir(profile_manager->user_data_dir());
  const base::FilePath& tor_path = BraveProfileManager::GetTorProfilePath();
  if (active_profile_path == tor_path)
    return true;

  return SetActiveProfileToGuestIfLocked_ChromiumImpl();
}
#endif  // !defined(OS_CHROMEOS)

}  // namespace profiles
