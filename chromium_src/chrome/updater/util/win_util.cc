/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string>

#include "base/functional/bind.h"
#include "chrome/updater/registration_data.h"
#include "chrome/updater/updater_scope.h"
#include "chrome/updater/util/util.h"

#define MigrateLegacyUpdaters MigrateLegacyUpdaters_ChromiumImpl
#include <chrome/updater/util/win_util.cc>
#undef MigrateLegacyUpdaters

namespace updater {

// In Omaha 3, we used to store the channel in the `ap` value. As of this
// writing, Omaha 4 (a.k.a. Chromium Updater) still supports it. However its
// docs (//docs/updater) do not mention it. Instead, the docs exclusively
// mention the `cohort` value. In our migration from Omaha 3 to Omaha 4, we
// therefore migrate `ap` to `cohort`. We have many different `ap` values such
// as "release", "release-test", "a64-r-test" and (when a delta update failed)
// "release-full", etc. But the only information they contain that isn't yet
// present in the application ID and the OS's architecture (which is included in
// update requests) is whether the update channel is a test channel. The
// function below therefore migrates `ap` to `cohort` by setting `cohort` to
// "private" if (and only if) `ap` contains "-test", which is the pattern our
// channel names use.
// TODO(mherrmann): Remove this once our migration to Omaha 4 on Windows is
// complete.
bool MigrateLegacyUpdaters(
    UpdaterScope scope,
    base::RepeatingCallback<void(const RegistrationRequest&)>
        register_callback) {
  return MigrateLegacyUpdaters_ChromiumImpl(
      scope,
      base::BindRepeating(
          [](const base::RepeatingCallback<void(const RegistrationRequest&)>&
                 callback,
             const RegistrationRequest& registration) {
            bool is_test_channel =
                registration.ap.find("-test") != std::string::npos;
            // We don't expect there to be an existing cohort, but just in case
            // there is, it should take precedence.
            bool has_cohort =
                registration.cohort && !registration.cohort->empty();
            if (is_test_channel && !has_cohort) {
              // We must .Clone() because `registration` is const.
              mojom::RegistrationRequestPtr modified = registration.Clone();
              modified->cohort = "private";
              callback.Run(*modified);
            } else {
              callback.Run(registration);
            }
          },
          register_callback));
}

}  // namespace updater
