// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import UIKit

public class SponsorCardView: FeedCardBackgroundButton, FeedCardContent {
  public var actionHandler: ((Int, FeedItemAction) -> Void)?
  public var contextMenu: FeedItemMenu?

  public let feedView = FeedItemView(layout: .bannerThumbnail).then {
    $0.isUserInteractionEnabled = false
  }

  private var contextMenuDelegate: NSObject?

  public required init() {
    super.init(frame: .zero)

    addSubview(feedView)
    feedView.snp.makeConstraints {
      $0.edges.equalToSuperview()
    }

    addTarget(self, action: #selector(tappedSelf), for: .touchUpInside)

    let contextMenuDelegate = FeedContextMenuDelegate(
      performedPreviewAction: { [weak self] in
        self?.actionHandler?(0, .opened())
      },
      menu: { [weak self] in
        return self?.contextMenu?.menu?(0)
      }
    )
    addInteraction(UIContextMenuInteraction(delegate: contextMenuDelegate))
    self.contextMenuDelegate = contextMenuDelegate

    isAccessibilityElement = true
  }

  public override var accessibilityLabel: String? {
    get { feedView.accessibilityLabel }
    set { assertionFailure("Accessibility label is inherited from a subview: \(String(describing: newValue)) ignored") }
  }

  @objc private func tappedSelf() {
    actionHandler?(0, .opened())
  }
}
