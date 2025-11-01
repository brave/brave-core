// Copyright 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import DesignSystem
import SnapKit
import UIKit

class RichNewTabTakeoverBackgroundView: UIView {
  var webUIController: ChromeWebUIController?

  func setupRichNewTabTakeoverLayer(
    braveProileController: BraveProfileController,
    richNewTabTakeoverURL: URL
  ) {
    webUIController = ChromeWebUIController(
      braveCore: braveProileController,
      isPrivateBrowsing: false
    ).then {
      $0.webView.load(URLRequest(url: richNewTabTakeoverURL))
      addSubview($0.view)
      $0.view.snp.makeConstraints {
        $0.edges.equalToSuperview()
      }
    }
  }

  override init(frame: CGRect) {
    super.init(frame: frame)

    clipsToBounds = true
    backgroundColor = .init {
      if $0.userInterfaceStyle == .dark {
        return .secondaryBraveBackground
      }
      // We use a special color here unfortunately when there is no background because
      // favorite cells have white text
      return .init(rgb: 0x3b3e4f)
    }
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
}
