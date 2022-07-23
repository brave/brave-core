/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_REWARDS_TIP_DIALOG_H_
#define BRAVE_BROWSER_BRAVE_REWARDS_TIP_DIALOG_H_

#include <memory>
#include <string>

#include "base/values.h"

namespace content {
class WebContents;
}

namespace brave_rewards {

void OpenTipDialog(content::WebContents* initiator, base::Value::Dict params);
}

#endif  // BRAVE_BROWSER_BRAVE_REWARDS_TIP_DIALOG_H_
