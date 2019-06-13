#include "../../../../../../chrome/browser/profiles/profile.cc"

#include "brave/common/tor/pref_names.h"

bool Profile::IsTorProfile() const {
  return GetPrefs()->GetBoolean(tor::prefs::kProfileUsingTor);
}

bool IsGuestProfile(Profile* profile) {
  DCHECK(profile);
  return (profile->HasOffTheRecordProfile() &&
          profile->GetOffTheRecordProfile() == profile &&
          profile->GetOriginalProfile()->IsGuestSession());
}
