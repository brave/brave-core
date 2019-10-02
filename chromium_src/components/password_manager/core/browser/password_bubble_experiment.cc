/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

class PrefService;
namespace syncer {
class SyncService;
}  // namespace syncer

namespace password_bubble_experiment {

bool ShouldShowChromeSignInPasswordPromo(
    PrefService* prefs,
    const syncer::SyncService* sync_service) {
  return false;
}

}  // namespace password_bubble_experiment

#define ShouldShowChromeSignInPasswordPromo \
  ShouldShowChromeSignInPasswordPromo_ChromiumImpl
#include "../../../../../../components/password_manager/core/browser/password_bubble_experiment.cc" // NOLINT
#undef ShouldShowChromeSignInPasswordPromo
