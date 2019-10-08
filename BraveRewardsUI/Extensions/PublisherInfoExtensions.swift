// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveRewards

extension PublisherInfo {
  /// A properly capitalized string to use when displaying the provider.
  ///
  /// Example: `youtube` becomes `YouTube`, `github` becomes `GitHub`, etc.
  var providerDisplayString: String {
    let providers = [
      "github": "GitHub",
      "reddit": "Reddit",
      "twitter": "Twitter",
      "twitch": "Twitch",
      "vimeo": "Vimeo",
      "youtube": "YouTube"
    ]
    return providers[provider.lowercased()] ?? provider
  }
}
