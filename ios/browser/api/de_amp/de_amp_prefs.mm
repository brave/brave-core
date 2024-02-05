// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/api/de_amp/de_amp_prefs+private.h"

#include "brave/components/de_amp/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "ios/chrome/browser/shared/model/application_context/application_context.h"
#include "ios/chrome/browser/shared/model/browser_state/chrome_browser_state.h"

@implementation DeAmpPrefs {
  PrefService* _profileState;
}

- (instancetype)initWithProfileState:(PrefService*)profileState {
  if ((self = [super init])) {
    _profileState = profileState;
  }
  return self;
}

- (bool)isDeAmpEnabled {
  return _profileState->GetBoolean(de_amp::kDeAmpPrefEnabled);
}

- (void)setIsDeAmpEnabled:(bool)isDeAmpEnabled {
  _profileState->SetBoolean(de_amp::kDeAmpPrefEnabled, isDeAmpEnabled);
  _profileState->CommitPendingWrite();
}

@end
