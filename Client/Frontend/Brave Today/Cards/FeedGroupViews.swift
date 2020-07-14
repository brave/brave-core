// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import BraveUI

/// Displays a group of feed items under a title, and optionally a brand under
/// the feeds.
class FeedGroupView: UIView {
    /// The user has tapped the feed that exists at a specific index
    var actionHandler: ((Int, FeedItemAction) -> Void)?
    
    var contextMenu: FeedItemMenu?
    
    /// The title label appearing above the list of feeds
    let titleLabel = UILabel().then {
        $0.font = .systemFont(ofSize: 21, weight: .bold)
        $0.appearanceTextColor = .white
    }
    /// The buttons that contain each feed view
    private let buttons: [SpringButton]
    /// The feeds that are displayed within the list
    let feedViews: [FeedItemView]
    /// The brand image at the bottom of each card.
    ///
    /// Set `isHidden` to true if you want to hide the brand image for a given
    /// card
    let groupBrandImageView = UIImageView().then {
        $0.contentMode = .left
        $0.snp.makeConstraints {
            $0.height.equalTo(20)
        }
    }
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
    init(axis: NSLayoutConstraint.Axis,
         feedLayout: FeedItemView.Layout,
         numberOfFeeds: Int = 3,
         transformItems: (([UIView]) -> [UIView])? = nil) {
        feedViews = (0..<numberOfFeeds).map { _ in
            FeedItemView(layout: feedLayout).then {
                $0.thumbnailImageView.layer.cornerRadius = 4
                if #available(iOS 13.0, *) {
                    $0.thumbnailImageView.layer.cornerCurve = .continuous
                }
                $0.isUserInteractionEnabled = false
            }
        }
        buttons = feedViews.map {
            let button = SpringButton()
            button.addSubview($0)
            $0.snp.makeConstraints {
                $0.edges.equalToSuperview()
            }
            return button
        }
        
        super.init(frame: .zero)
        
        zip(buttons.indices, buttons).forEach { (index, button) in
            button.addTarget(self, action: #selector(tappedButton(_:)), for: .touchUpInside)
            if #available(iOS 13.0, *) {
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
            } else {
                let longPress = UILongPressGestureRecognizer(target: self, action: #selector(longPressed(_:)))
                button.addGestureRecognizer(longPress)
            }
        }
        let stackView = UIStackView().then {
            $0.axis = .vertical
            $0.spacing = 16
        }
        stackView.addStackViewItems(
            .view(titleLabel),
            .view(UIStackView().then {
                $0.spacing = 16
                $0.axis = axis
                if axis == .horizontal {
                    $0.distribution = .fillEqually
                    $0.alignment = .top
                }
                let transform: ([UIView]) -> [UIView] = transformItems ?? { views in views }
                let groupViews = transform(buttons)
                groupViews.forEach($0.addArrangedSubview)
            }),
            .view(groupBrandImageView)
        )
        
        addSubview(backgroundView)
        addSubview(stackView)
        
        backgroundView.snp.makeConstraints {
            $0.edges.equalToSuperview()
        }
        stackView.snp.makeConstraints {
            $0.edges.equalToSuperview().inset(20)
        }
    }
    
    @available(*, unavailable)
    required init(coder: NSCoder) {
        fatalError()
    }
    
    @objc private func longPressed(_ gesture: UILongPressGestureRecognizer) {
        if gesture.state == .began {
            if let index = buttons.firstIndex(where: { gesture.view === $0 }),
                let legacyContext = contextMenu?.legacyMenu?(index) {
                actionHandler?(index, .longPressed(legacyContext))
            }
        }
    }
    
    @objc private func tappedButton(_ sender: SpringButton) {
        if let index = buttons.firstIndex(where: { sender === $0 }) {
            actionHandler?(index, .opened())
        }
    }
}

/// A group of feed items placed horizontally in a card
class HorizontalFeedGroupView: FeedGroupView, FeedCardContent {
    required init() {
        super.init(axis: .horizontal, feedLayout: .vertical)
    }
}

/// A group of feed items placed vertically in a card
class VerticalFeedGroupView: FeedGroupView, FeedCardContent {
    required init() {
        super.init(axis: .vertical, feedLayout: .horizontal)
    }
}

/// A group of feed items numbered and placed vertically in a card
class NumberedFeedGroupView: FeedGroupView, FeedCardContent {
    required init() {
        super.init(axis: .vertical, feedLayout: .basic, transformItems: { views in
            // Turn the usual feed group item into a numbered item
            views.enumerated().map { view in
                UIStackView().then {
                    $0.spacing = 16
                    $0.alignment = .center
                    $0.addStackViewItems(
                        .view(UILabel().then {
                            $0.text = "\(view.offset + 1)"
                            $0.font = .systemFont(ofSize: 16, weight: .bold)
                            $0.appearanceTextColor = UIColor(white: 1.0, alpha: 0.4)
                            $0.setContentHuggingPriority(.required, for: .horizontal)
                        }),
                        .view(view.element)
                    )
                }
            }
        })
    }
}
