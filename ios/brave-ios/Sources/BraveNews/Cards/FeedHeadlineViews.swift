// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import UIKit

public class HeadlineCardView: FeedCardBackgroundButton, FeedCardContent {
  public var actionHandler: ((Int, FeedItemAction) -> Void)?
  public var contextMenu: FeedItemMenu?

  public let feedView = FeedItemView(layout: .brandedHeadline).then {
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

public class SmallHeadlineCardView: HeadlineCardView {

  public required init() {
    super.init()

    feedView.titleLabel.font = .systemFont(ofSize: 14, weight: .semibold)
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
}

public class SmallHeadlinePairCardView: UIView, FeedCardContent {
  public var actionHandler: ((Int, FeedItemAction) -> Void)?
  public var contextMenu: FeedItemMenu?

  private let stackView = UIStackView().then {
    $0.distribution = .fillEqually
    $0.spacing = 20
  }

  public let smallHeadelineCardViews: (left: SmallHeadlineCardView, right: SmallHeadlineCardView) = (SmallHeadlineCardView(), SmallHeadlineCardView())

  public required init() {
    super.init(frame: .zero)

    addSubview(stackView)
    stackView.addArrangedSubview(smallHeadelineCardViews.left)
    stackView.addArrangedSubview(smallHeadelineCardViews.right)

    smallHeadelineCardViews.left.actionHandler = { [weak self] _, action in
      self?.actionHandler?(0, action)
    }
    smallHeadelineCardViews.right.actionHandler = { [weak self] _, action in
      self?.actionHandler?(1, action)
    }
    smallHeadelineCardViews.left.contextMenu = FeedItemMenu({ [weak self] _ -> UIMenu? in
      return self?.contextMenu?.menu?(0)
    })
    smallHeadelineCardViews.right.contextMenu = FeedItemMenu({ [weak self] _ -> UIMenu? in
      return self?.contextMenu?.menu?(1)
    })

    stackView.snp.makeConstraints {
      $0.edges.equalToSuperview()
    }
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
}
