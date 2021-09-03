/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/settings/brave_relaunch_handler_mac.h"

#include "base/bind.h"
#import "brave/browser/mac/sparkle_glue.h"

void BraveRelaunchHandler::RegisterMessages() {
  web_ui()->RegisterDeprecatedMessageCallback(
      "relaunchOnMac", base::BindRepeating(&BraveRelaunchHandler::Relaunch,
                                           base::Unretained(this)));
}

void BraveRelaunchHandler::Relaunch(const base::ListValue* args) {
  [[SparkleGlue sharedSparkleGlue] relaunch];
}
