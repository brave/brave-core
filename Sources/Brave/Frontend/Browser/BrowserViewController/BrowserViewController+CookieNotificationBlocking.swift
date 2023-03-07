// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import SwiftUI
import BraveUI
import BraveShared
import Data
import Onboarding

extension BrowserViewController {
  func presentCookieNotificationBlockingCalloutIfNeeded() {
    // Check the blockCookieConsentNotices callout can be shown
    guard shouldShowCallout(calloutType: .blockCookieConsentNotices) else {
      return
    }
    
    // Don't show this if we already enabled the setting
    guard !FilterListResourceDownloader.shared.isEnabled(for: FilterList.cookieConsentNoticesComponentID) else { return }
    // Don't show this if we are presenting another popup already
    guard !isOnboardingOrFullScreenCalloutPresented else { return }
    // We only show the popup on second launch
    guard !Preferences.General.isFirstLaunch.value else { return }
    // Ensure we successfully shown basic onboarding first
    guard Preferences.FullScreenCallout.omniboxCalloutCompleted.value else { return }

    let popover = PopoverController(
      contentController: CookieNotificationBlockingConsentViewController(),
      contentSizeBehavior: .preferredContentSize)
    popover.addsConvenientDismissalMargins = false
    popover.present(from: topToolbar.locationView.shieldsButton, on: self)
  }
}

class CookieNotificationBlockingConsentViewController: UIHostingController<CookieNotificationBlockingConsentView>, PopoverContentComponent {
  init() {
    super.init(rootView: CookieNotificationBlockingConsentView())
    
    self.preferredContentSize = CGSize(
      width: CookieNotificationBlockingConsentView.contentWidth,
      height: CookieNotificationBlockingConsentView.contentHeight
    )
  }

  required init?(coder aDecoder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }
  
  override func viewDidLoad() {
    super.viewDidLoad()
    view.backgroundColor = UIColor.braveBackground
  }
}
