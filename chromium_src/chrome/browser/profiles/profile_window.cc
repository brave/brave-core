#include "chrome/browser/profiles/profile_window.h"
#define IsLockAvailable IsLockAvailable_ChromiumImpl
#define HasProfileSwitchTargets HasProfileSwitchTargets_ChromiumImpl
#include "../../../../../../chrome/browser/profiles/profile_window.cc"
#undef HasProfileSwitchTargets
#undef IsLockAvailable

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
      base::string16(), std::string(), std::string());
}

#endif

bool IsLockAvailable(Profile* profile) {
  DCHECK(profile);
  if (profile->IsTorProfile())
    return false;
  return IsLockAvailable_ChromiumImpl(profile);
}

bool HasProfileSwitchTargets(Profile* profile) {
  if (profile->IsTorProfile()) {
  size_t number_of_profiles =
      g_browser_process->profile_manager()->GetNumberOfProfiles();
  return number_of_profiles >= 1;
  }
  return HasProfileSwitchTargets_ChromiumImpl(profile);
}
}  // namespace profiles
