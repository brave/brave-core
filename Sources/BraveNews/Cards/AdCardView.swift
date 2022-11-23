// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import UIKit
import BraveUI

public class AdCardView: FeedCardBackgroundButton, FeedCardContent {
  public var actionHandler: ((Int, FeedItemAction) -> Void)?
  public var contextMenu: FeedItemMenu?

  public let feedView = FeedItemView(layout: .ad).then {
    $0.thumbnailImageView.contentMode = .scaleAspectFit
  }

  private let adCalloutView = BraveAdCalloutView()
  private var contextMenuDelegate: NSObject?

  public required init() {
    super.init(frame: .zero)

    addSubview(feedView)
    feedView.snp.makeConstraints {
      $0.edges.equalToSuperview()
    }

    addTarget(self, action: #selector(tappedSelf), for: .touchUpInside)
    feedView.callToActionButton.addTarget(self, action: #selector(tappedSelf), for: .touchUpInside)

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

    isAccessibilityElement = false
    accessibilityElements = [feedView, feedView.callToActionButton]
    feedView.accessibilityTraits.insert(.button)
    shouldGroupAccessibilityChildren = true

    addSubview(adCalloutView)
    adCalloutView.snp.makeConstraints {
      $0.top.trailing.equalToSuperview().inset(8)
      $0.leading.greaterThanOrEqualToSuperview().inset(8)
      $0.bottom.lessThanOrEqualToSuperview().inset(8)
    }
  }

  public override var accessibilityLabel: String? {
    get { feedView.accessibilityLabel }
    set { assertionFailure("Accessibility label is inherited from a subview: \(newValue ?? "nil") ignored") }
  }

  @objc private func tappedSelf() {
    actionHandler?(0, .opened())
  }
}

private class BraveAdCalloutView: UIView {
  override init(frame: CGRect) {
    super.init(frame: frame)

    backgroundColor = .white
    layer.cornerRadius = 4
    layer.cornerCurve = .continuous
    layer.borderColor = UIColor.braveLighterBlurple.cgColor
    layer.borderWidth = 1
    layer.masksToBounds = true

    let stackView = UIStackView()
    stackView.spacing = 3
    stackView.alignment = .center
    stackView.layoutMargins = .init(top: 5, left: 5, bottom: 5, right: 5)
    stackView.isLayoutMarginsRelativeArrangement = true
    stackView.addStackViewItems(
      .view(
        UIImageView(image: UIImage(sharedNamed: "bat-small")!).then {
          $0.contentMode = .scaleAspectFit
          $0.snp.makeConstraints {
            $0.size.equalTo(14)
          }
        }),
      .view(
        UILabel().then {
          $0.text = "Ad"
          $0.textColor = .braveBlurpleTint
          $0.font = {
            let metrics = UIFontMetrics(forTextStyle: .footnote)
            let desc = UIFontDescriptor.preferredFontDescriptor(withTextStyle: .footnote)
            let font = UIFont.systemFont(ofSize: desc.pointSize, weight: .semibold)
            return metrics.scaledFont(for: font)
          }()
          $0.adjustsFontForContentSizeCategory = true
        })
    )
    addSubview(stackView)
    stackView.snp.makeConstraints {
      $0.edges.equalToSuperview()
    }
  }
  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
}
