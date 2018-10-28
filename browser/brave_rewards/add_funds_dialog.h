// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_REWARDS_ADD_FUNDS_DIALOG_H_
#define BRAVE_BROWSER_REWARDS_ADD_FUNDS_DIALOG_H_

#include <map>
#include <string>

#include "ui/gfx/native_widget_types.h"

class Profile;

namespace content {
class WebContents;
}

namespace brave_rewards {

void OpenAddFundsDialog(content::WebContents* initiator,
                        const std::map<std::string, std::string>& addresses);

void OpenAddFundsExtensionDialog(gfx::NativeWindow parent_window,
                                 Profile* profile,
                                 content::WebContents* initiator,
                                 const std::map<std::string, std::string>& addresses);
}

#endif  // BRAVE_BROWSER_REWARDS_ADD_FUNDS_DIALOG_H_
