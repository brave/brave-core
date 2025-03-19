// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Shared
import SwiftUI

class NewTabTakeoverInfoBar: InfoBar {
  static let learnMoreOptOutChoicesUrl = "https://brave.com"

  init(tabManager: TabManager) {
    super.init(
      tabManager: tabManager,
      labelText: Strings.newTabTakeoverInfoBarMessage,
      linkText: Strings.newTabTakeoverInfoBarLearnMoreOptOutChoices,
      linkUrl: NewTabTakeoverInfoBar.learnMoreOptOutChoicesUrl
    )
  }

  required init?(coder aDecoder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }
}
