/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/webui/help/version_updater_mac.h"

#import "brave/browser/sparkle_glue_mac.h"
#include "brave/browser/update_util.h"

VersionUpdater* VersionUpdater::Create(
    content::WebContents* web_contents) {
  return new VersionUpdaterMac;
}

VersionUpdaterMac::VersionUpdaterMac() {
}

VersionUpdaterMac::~VersionUpdaterMac() {
}

void VersionUpdaterMac::CheckForUpdate(
    const StatusCallback& status_callback,
    const PromoteCallback& promote_callback) {
  if (brave::UpdateEnabled()) {
    [[SparkleGlue sharedSparkleGlue] checkForUpdates:nil];

    // TODO(simonhong): Update status from sparkle.
    status_callback.Run(DISABLED, 0, std::string(), 0, base::string16());
    NOTIMPLEMENTED();
  } else {
    status_callback.Run(DISABLED, 0, std::string(), 0, base::string16());
  }
}

void VersionUpdaterMac::PromoteUpdater() const {
  NOTIMPLEMENTED();
}
