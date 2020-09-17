//
//  user_prefs.cpp
//  Sources
//
//  Created by brandon on 2020-05-08.
//

#include "brave/vendor/brave-ios/components/user_prefs/user_prefs.h"

#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/memory/singleton.h"
#include "components/prefs/pref_service.h"

namespace user_prefs {

namespace {

void* UserDataKey() {
  static int data_key = 0;
  return reinterpret_cast<void*>(&data_key);
}

}  // namespace

// static
PrefService* UserPrefs::Get(base::SupportsUserData* context) {
  DCHECK(context);
  DCHECK(context->GetUserData(UserDataKey()));
  return static_cast<UserPrefs*>(
      context->GetUserData(UserDataKey()))->prefs_;
}

// static
void UserPrefs::Set(base::SupportsUserData* context, PrefService* prefs) {
  DCHECK(context);
  DCHECK(prefs);
  DCHECK(!context->GetUserData(UserDataKey()));
  context->SetUserData(UserDataKey(), base::WrapUnique(new UserPrefs(prefs)));
}

UserPrefs::UserPrefs(PrefService* prefs) : prefs_(prefs) {
}

UserPrefs::~UserPrefs() {
}

}  // namespace user_prefs
