// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "base/memory/raw_ptr.h"
#include "brave/ios/browser/api/prefs/browser_prefs+private.h"
#include "components/prefs/pref_service.h"
#include "ios/chrome/browser/shared/model/prefs/pref_names.h"

@implementation BrowserPrefs {
  raw_ptr<PrefService> _prefs;
}

- (instancetype)initWithPrefService:(PrefService*)prefs {
  if ((self = [super init])) {
    _prefs = prefs;
  }
  return self;
}

- (BOOL)httpsUpgradesEnabled {
  return _prefs->GetBoolean(prefs::kHttpsUpgradesEnabled);
}

- (void)setHttpsUpgradesEnabled:(BOOL)httpsUpgradesEnabled {
  _prefs->SetBoolean(prefs::kHttpsUpgradesEnabled, httpsUpgradesEnabled);
}

- (BOOL)httpsOnlyModeEnabled {
  return _prefs->GetBoolean(prefs::kHttpsOnlyModeEnabled);
}

- (void)setHttpsOnlyModeEnabled:(BOOL)httpsOnlyModeEnabled {
  _prefs->SetBoolean(prefs::kHttpsOnlyModeEnabled, httpsOnlyModeEnabled);
}

@end
