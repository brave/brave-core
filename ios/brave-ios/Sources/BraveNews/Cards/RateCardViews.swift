// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import UIKit
import BraveStrings

/// The actions you can perform on a rate card item
public enum RatingCardAction: Equatable {
  // The user choose to navigate Appstore for app rating
  case rateBrave
  // The user choose to hide card for app rating
  case hideCard
}

public class SmallRateCardView: FeedCardBackgroundButton, FeedCardContent {
  public var rateCardActionHandler: ((RatingCardAction) -> Void)?
  public var contextMenu: FeedItemMenu?

  public let feedView = FeedItemView(layout: .rateCard).then {
    $0.isUserInteractionEnabled = false
    $0.callToActionButton.setTitle(Strings.BraveNews.rateBraveCardRateActionTitle, for: .normal)
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
        self?.rateCardActionHandler?(.rateBrave)
      },
      menu: { [weak self] in
        return self?.contextMenu?.menu?(0)
      }
    )
    addInteraction(UIContextMenuInteraction(delegate: contextMenuDelegate))
    self.contextMenuDelegate = contextMenuDelegate

    isAccessibilityElement = true
    
    feedView.titleLabel.text = Strings.BraveNews.rateBraveCardTitle
    feedView.descriptionLabel.do {
      $0.text = Strings.BraveNews.rateBraveCardSubtitle
      $0.textColor = .white
    }
    feedView.thumbnailImageView.do {
      $0.image = UIImage(sharedNamed: "brave.logo")
      $0.contentMode = .scaleAspectFit
      $0.backgroundColor = .clear
    }
  }

  public override var accessibilityLabel: String? {
    get { feedView.accessibilityLabel }
    set { assertionFailure("Accessibility label is inherited from a subview: \(String(describing: newValue)) ignored") }
  }

  @objc private func tappedSelf() {
    rateCardActionHandler?(.rateBrave)
  }
  
  // MARK: - FeedCardContent

  public var actionHandler: ((Int, FeedItemAction) -> Void)? {
    didSet {
      assertionFailure("Unused for welcome card")
    }
  }
}

public class SmallHeadlineRatePairCardView: UIView, FeedCardContent {
  public var actionHandler: ((Int, FeedItemAction) -> Void)?
  public var rateCardActionHandler: ((RatingCardAction) -> Void)?

  public var contextMenu: FeedItemMenu?

  private let stackView = UIStackView().then {
    $0.distribution = .fillEqually
    $0.spacing = 20
  }

  public let smallHeadlineRateCardViews: (smallHeadline: SmallHeadlineCardView, ratingCard: SmallRateCardView) = (SmallHeadlineCardView(), SmallRateCardView())

  public required init() {
    super.init(frame: .zero)

    addSubview(stackView)
    stackView.addArrangedSubview(smallHeadlineRateCardViews.smallHeadline)
    stackView.addArrangedSubview(smallHeadlineRateCardViews.ratingCard)

    smallHeadlineRateCardViews.smallHeadline.actionHandler = { [weak self] _, action in
      self?.actionHandler?(0, action)
    }
    smallHeadlineRateCardViews.ratingCard.rateCardActionHandler = { [weak self] action in
      self?.rateCardActionHandler?(action)
    }
    
    smallHeadlineRateCardViews.smallHeadline.contextMenu = FeedItemMenu({ [weak self] _ -> UIMenu? in
      return self?.contextMenu?.menu?(0)
    })
    smallHeadlineRateCardViews.ratingCard.contextMenu = FeedItemMenu({ [weak self] _ -> UIMenu? in
      typealias MenuActionHandler = () -> Void

      let appRatingCardPressedHandler: MenuActionHandler = { [weak self] in
        self?.rateCardActionHandler?(.rateBrave)
      }
      let openInNewPrivateTabHandler: MenuActionHandler = { [weak self] in
        self?.rateCardActionHandler?(.hideCard)
      }
      
      func mapDeferredHandler(_ handler: @escaping MenuActionHandler) -> UIActionHandler {
        return UIAction.deferredActionHandler { _ in
          handler()
        }
      }
      var openInAppStore: UIAction {
        .init(title: Strings.BraveNews.rateBraveCardRateActionTitle,
              image: UIImage(braveSystemNamed: "leo.thumb.up"),
              handler: mapDeferredHandler(appRatingCardPressedHandler))
      }

      var dismissCardRate: UIAction {
        .init(title: Strings.BraveNews.rateBraveCardHideActionTitle,
              image: UIImage(braveSystemNamed: "leo.eye.off"),
              attributes: .destructive,
              handler: mapDeferredHandler(openInNewPrivateTabHandler))
      }
      let openActions: [UIAction] = [
        openInAppStore,
        dismissCardRate
      ].compactMap { $0 }
      
      let children: [UIMenu] = [
        UIMenu(title: "", options: [.displayInline], children: openActions)
      ]
      
      return UIMenu(title: Strings.BraveNews.rateBraveCardActionSheetTitle, children: children)
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
