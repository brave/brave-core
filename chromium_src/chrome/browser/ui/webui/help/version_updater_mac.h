/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef CHROME_BROWSER_UI_WEBUI_HELP_VERSION_UPDATER_MAC_H_
#define CHROME_BROWSER_UI_WEBUI_HELP_VERSION_UPDATER_MAC_H_

#include "base/macros.h"
#include "chrome/browser/ui/webui/help/version_updater.h"

// OS X implementation of version update functionality, used by the WebUI
// About/Help page.
class VersionUpdaterMac : public VersionUpdater {
 public:
  // VersionUpdater implementation.
  void CheckForUpdate(const StatusCallback& status_callback,
                      const PromoteCallback& promote_callback) override;
  void PromoteUpdater() const override;

 protected:
  friend class VersionUpdater;

  // Clients must use VersionUpdater::Create().
  VersionUpdaterMac();
  ~VersionUpdaterMac() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(VersionUpdaterMac);
};

#endif  // CHROME_BROWSER_UI_WEBUI_HELP_VERSION_UPDATER_MAC_H_

