/* Copyright (c) 2018 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_WEBUI_HELP_VERSION_UPDATER_MAC_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_WEBUI_HELP_VERSION_UPDATER_MAC_H_

#import <AppKit/AppKit.h>
#include <string.h>

#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/browser/sparkle_buildflags.h"
#include "chrome/browser/ui/webui/help/version_updater.h"
#include "chrome/updater/update_service.h"
#include "chrome/updater/updater_scope.h"

@class KeystoneObserver;

// macOS implementation of version update functionality, used by the WebUI
// About/Help page.
class SparkleVersionUpdater : public VersionUpdater {
 public:
  SparkleVersionUpdater();
  SparkleVersionUpdater(const SparkleVersionUpdater&) = delete;
  SparkleVersionUpdater& operator=(const SparkleVersionUpdater&) = delete;
  ~SparkleVersionUpdater() override;

  // VersionUpdater implementation.
  void CheckForUpdate(StatusCallback status_callback,
                      PromoteCallback promote_callback) override;
  void PromoteUpdater() override;

  // Process status updates received from Keystone. The dictionary will contain
  // an AutoupdateStatus value as an intValue at key kAutoupdateStatusStatus. If
  // a version is available (see AutoupdateStatus), it will be present at key
  // kAutoupdateStatusVersion.
  void UpdateStatus(NSDictionary* status);

#if BUILDFLAG(ENABLE_SPARKLE)
  void GetIsSparkleForTesting(bool& result) const override;
#endif

 private:
  // Update the visibility state of promote button.
  void UpdateShowPromoteButton();

  // Updates the status from the Chromium Updater.
  void UpdateStatusFromChromiumUpdater(
      VersionUpdater::StatusCallback status_callback,
      const updater::UpdateService::UpdateState& update_state);

  // Callback used to communicate update status to the client.
  StatusCallback status_callback_;

  // Callback used to show or hide the promote UI elements.
  PromoteCallback promote_callback_;

  // The visible state of the promote button.
  bool show_promote_button_;

  // The observer that will receive Keystone status updates.
  KeystoneObserver* __strong keystone_observer_;

  base::WeakPtrFactory<SparkleVersionUpdater> weak_factory_{this};
};

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_WEBUI_HELP_VERSION_UPDATER_MAC_H_
