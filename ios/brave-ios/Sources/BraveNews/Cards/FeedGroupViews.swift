// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import BraveUI
import Shared

/// Displays a group of feed items under a title, and optionally a brand under
/// the feeds.
public class FeedGroupView: UIView {
  /// The user has tapped the feed that exists at a specific index
  public var actionHandler: ((Int, FeedItemAction) -> Void)?

  public var contextMenu: FeedItemMenu?

  /// The containing stack view that can be used to prepend or append views to the group card
  public let containerStackView = UIStackView().then {
    $0.axis = .vertical
    $0.spacing = 0
    $0.shouldGroupAccessibilityChildren = true
  }

  /// The title label appearing above the list of feeds
  public let titleLabel = UILabel().then {
    $0.font = .systemFont(ofSize: 21, weight: .bold)
    $0.textColor = .white
    $0.numberOfLines = 0
  }
  /// The buttons that contain each feed view
  private let buttons: [SpringButton]
  /// The feeds that are displayed within the list
  public let feedViews: [FeedItemView]
  /// The context menu delegate if on iOS 13
  private var contextMenuDelegates: [NSObject] = []
  /// The blurred background view
  private let backgroundView = FeedCardBackgroundView()
  /// Create a card that contains a list of feeds within it
  /// - parameters:
  ///     - axis: Controls whether or not feeds are distributed horizontally
  ///             or vertically
  ///     - feedLayout: Controls how each individual feed item is laid out
  ///     - numberOfFeeds: The number of feeds views to create and to the list
  ///     - transformItems: A block to transform the group item views to
  ///       another view in case it needs to be altered, padded, etc.
  init(
    axis: NSLayoutConstraint.Axis,
    feedLayout: FeedItemView.Layout,
    numberOfFeeds: Int = 3,
    transformItems: (([UIView]) -> [UIView])? = nil
  ) {
    feedViews = (0..<numberOfFeeds).map { _ in
      FeedItemView(layout: feedLayout).then {
        $0.thumbnailImageView.layer.cornerRadius = 4
        $0.thumbnailImageView.layer.cornerCurve = .continuous
        $0.isUserInteractionEnabled = false
      }
    }
    buttons = feedViews.map(FeedSpringButton.init)

    super.init(frame: .zero)

    zip(buttons.indices, buttons).forEach { (index, button) in
      button.addTarget(self, action: #selector(tappedButton(_:)), for: .touchUpInside)
      let contextMenuDelegate = FeedContextMenuDelegate(
        performedPreviewAction: { [weak self] in
          self?.actionHandler?(index, .opened())
        },
        menu: { [weak self] in
          return self?.contextMenu?.menu?(index)
        },
        padPreview: true
      )
      button.addInteraction(UIContextMenuInteraction(delegate: contextMenuDelegate))
      contextMenuDelegates.append(contextMenuDelegate)
    }
    let stackView = UIStackView().then {
      $0.axis = .vertical
      $0.spacing = 16
      $0.isLayoutMarginsRelativeArrangement = true
      $0.layoutMargins = UIEdgeInsets(top: 20, left: 20, bottom: 20, right: 20)
    }
    stackView.addStackViewItems(
      .view(titleLabel),
      .view(
        UIStackView().then {
          $0.spacing = 16
          $0.axis = axis
          if axis == .horizontal {
            $0.distribution = .fillEqually
            $0.alignment = .top
          }
          let transform: ([UIView]) -> [UIView] = transformItems ?? { views in views }
          let groupViews = transform(buttons)
          groupViews.forEach($0.addArrangedSubview)
        })
    )

    addSubview(backgroundView)
    addSubview(containerStackView)
    containerStackView.addArrangedSubview(stackView)

    backgroundView.snp.makeConstraints {
      $0.edges.equalToSuperview()
    }
    containerStackView.snp.makeConstraints {
      $0.edges.equalToSuperview()
    }

    layer.cornerRadius = backgroundView.layer.cornerRadius
    layer.masksToBounds = true
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }

  @objc private func tappedButton(_ sender: SpringButton) {
    if let index = buttons.firstIndex(where: { sender === $0 }) {
      actionHandler?(index, .opened())
    }
  }
}

/// A group of feed items placed horizontally in a card
public class HorizontalFeedGroupView: FeedGroupView, FeedCardContent {
  public required init() {
    super.init(axis: .horizontal, feedLayout: .vertical)
  }
}

/// A group of deal feed items placed horizontally in a card
public class DealsFeedGroupView: FeedGroupView, FeedCardContent {
  public var moreOffersButtonTapped: (() -> Void)?

  public required init() {
    super.init(axis: .horizontal, feedLayout: .offer)

    containerStackView.addStackViewItems(
      .view(
        UIView().then {
          $0.backgroundColor = UIColor(white: 1.0, alpha: 0.15)
          $0.snp.makeConstraints {
            $0.height.equalTo(1.0 / UIScreen.main.scale)
          }
          $0.isUserInteractionEnabled = false
          $0.isAccessibilityElement = false
        }),
      .view(
        FeedCardFooterButton().then {
          $0.label.text = Strings.BraveNews.moreBraveOffers
          $0.addTarget(self, action: #selector(tappedMoreOffers), for: .touchUpInside)
        })
    )
  }

  @objc private func tappedMoreOffers() {
    moreOffersButtonTapped?()
  }
}

/// A group of feed items placed vertically in a card
public class VerticalFeedGroupView: FeedGroupView, FeedCardContent {
  public required init() {
    super.init(axis: .vertical, feedLayout: .horizontal)
  }
}

/// A group of feed items numbered and placed vertically in a card
public class NumberedFeedGroupView: FeedGroupView, FeedCardContent {
  public required init() {
    super.init(
      axis: .vertical, feedLayout: .basic,
      transformItems: { views in
        // Turn the usual feed group item into a numbered item
        views.enumerated().map { view in
          UIStackView().then {
            $0.spacing = 16
            $0.alignment = .center
            $0.addStackViewItems(
              .view(
                UILabel().then {
                  $0.text = "\(view.offset + 1)"
                  $0.font = .systemFont(ofSize: 16, weight: .bold)
                  $0.textColor = UIColor(white: 1.0, alpha: 0.4)
                  $0.setContentHuggingPriority(.required, for: .horizontal)
                  $0.isAccessibilityElement = false
                }),
              .view(view.element)
            )
          }
        }
      })
  }
}

private class FeedSpringButton: SpringButton {
  let feedItemView: FeedItemView

  init(itemView: FeedItemView) {
    feedItemView = itemView

    super.init(frame: .zero)

    addSubview(itemView)
    itemView.snp.makeConstraints {
      $0.edges.equalToSuperview()
    }
  }

  override var accessibilityLabel: String? {
    get { feedItemView.accessibilityLabel }
    set { assertionFailure("Accessibility label is inherited from a subview: \(newValue ?? "nil") ignored") }
  }
}
