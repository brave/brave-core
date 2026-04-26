/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/core/browser/utils.h"

#include "base/check.h"
#include "base/feature_list.h"
#include "brave/components/playlist/core/common/features.h"
#include "brave/components/playlist/core/common/pref_names.h"
#include "components/prefs/pref_service.h"

namespace playlist {

namespace {

bool IsDisabledByPolicy(PrefService* prefs) {
  return prefs->IsManagedPreference(kPlaylistEnabledPref) &&
         !prefs->GetBoolean(kPlaylistEnabledPref);
}

}  // namespace

bool IsPlaylistAllowed(PrefService* prefs) {
  DCHECK(prefs);
  return base::FeatureList::IsEnabled(features::kPlaylist) &&
         !IsDisabledByPolicy(prefs);
}

}  // namespace playlist
