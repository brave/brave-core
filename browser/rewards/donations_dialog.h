// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_REWARDS_DONATIONS_DIALOG_CONTROLLER_H_
#define BRAVE_BROWSER_REWARDS_DONATIONS_DIALOG_CONTROLLER_H_

namespace content {
  class WebContents;
}

namespace donations {
  void OpenDonationDialog(content::WebContents* initiator);
}


#endif