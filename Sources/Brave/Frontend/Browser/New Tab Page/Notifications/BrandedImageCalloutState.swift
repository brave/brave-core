// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Shared
import Preferences

enum BrandedImageCalloutState {
  /// User is supporting content creators
  case brandedImageSupport
  /// Don't show any callout
  case dontShow

  static func getState(
    adsEnabled: Bool, adsAvailableInRegion: Bool,
    isSponsoredImage: Bool
  ) -> BrandedImageCalloutState {
    if adsEnabled && isSponsoredImage {
      return brandedImageSupport
    }
    // If any of remaining callouts were shown once, we skip showing any other state.
    let wasCalloutShowed = Preferences.NewTabPage.brandedImageShowed.value

    if wasCalloutShowed { return .dontShow }

    if !adsAvailableInRegion { return .dontShow }

    if adsEnabled && isSponsoredImage { return .brandedImageSupport }

    return .dontShow
  }

  /// Not all initial views have a followup view, this computed property tracks it.
  var hasDetailViewController: Bool {
    switch self {
    case .brandedImageSupport: return true
    case .dontShow: return false
    }
  }
}
