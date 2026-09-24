// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveStrings
import SwiftUI

class SearchResultAdClickedInfoBar: InfoBar {
  init(
    onLinkPressed: @escaping (URL) -> Void
  ) {
    super.init(
      labelText: Strings.Ads.searchResultAdClickedInfoBarMessage,
      linkText: Strings.Ads.searchResultAdClickedInfoBarLearnMoreOptOutChoices,
      linkUrl: "https://search.brave.com/help/conversion-reporting",
      onLinkPressed: onLinkPressed
    )
  }

  required init?(coder aDecoder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }
}
