#include "brave/browser/profiles/brave_profile_manager.h"
#include "chrome/browser/profiles/profiles_state.h"
#include "ui/gfx/text_elider.h"
#define SetActiveProfileToGuestIfLocked SetActiveProfileToGuestIfLocked_ChromiumImpl
#define GetAvatarButtonTextForProfile GetAvatarButtonTextForProfile_ChromiumImpl
#define GetAvatarNameForProfile GetAvatarNameForProfile_ChromiumImpl
#include "../../../../../../chrome/browser/profiles/profiles_state.cc"
#undef GetAvatarNameForProfile
#undef GetAvatarButtonTextForProfile
#undef SetActiveProfileToGuestIfLocked

namespace profiles {

#if !defined(OS_ANDROID)
base::string16 GetAvatarNameForProfile(const base::FilePath& profile_path) {
  if (profile_path == BraveProfileManager::GetTorProfilePath())
    return l10n_util::GetStringUTF16(IDS_TOR_PROFILE_NAME);
  return GetAvatarNameForProfile_ChromiumImpl(profile_path);
}

#if !defined(OS_CHROMEOS)
base::string16 GetAvatarButtonTextForProfile(Profile* profile) {
  const int kMaxCharactersToDisplay = 15;
  base::string16 name = GetAvatarNameForProfile(profile->GetPath());
  name = gfx::TruncateString(name,
                             kMaxCharactersToDisplay,
                             gfx::CHARACTER_BREAK);
  if (profile->IsLegacySupervised()) {
    name = l10n_util::GetStringFUTF16(
        IDS_LEGACY_SUPERVISED_USER_NEW_AVATAR_LABEL, name);
  }
  return name;
}

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
#endif  // !defined(OS_ANDROID)

}  // namespace profiles
