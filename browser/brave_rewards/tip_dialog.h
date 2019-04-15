/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_REWARDS_TIP_DIALOG_H_
#define BRAVE_BROWSER_BRAVE_REWARDS_TIP_DIALOG_H_

#include <string>

namespace content {
class WebContents;
}

namespace brave_rewards {

void OpenTipDialog(content::WebContents* initiator,
                   std::unique_ptr<base::DictionaryValue> params);

}

#endif  // BRAVE_BROWSER_BRAVE_REWARDS_TIP_DIALOG_H_
