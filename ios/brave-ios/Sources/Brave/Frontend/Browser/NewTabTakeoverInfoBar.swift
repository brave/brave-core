// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveStrings
import SwiftUI

class NewTabTakeoverInfoBar: InfoBar {
  init(
    tabManager: TabManager,
    onLinkPressed: @escaping (() -> Void),
    onClosePressed: @escaping (() -> Void)
  ) {
    super.init(
      tabManager: tabManager,
      labelText: Strings.Ads.newTabTakeoverInfoBarMessage,
      linkText: Strings.Ads.newTabTakeoverInfoBarLearnMoreOptOutChoices,
      linkUrl: "https://support.brave.com/hc/en-us/articles/35182999599501",
      onLinkPressed: onLinkPressed,
      onClosePressed: onClosePressed
    )
  }

  required init?(coder aDecoder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }
}
