// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import BraveUI
import Shared

/// Displays a group of feed items under a title, and optionally a brand under
/// the feeds.
class FeedGroupView: UIView {
    /// The user has tapped the feed that exists at a specific index
    var actionHandler: ((Int, FeedItemAction) -> Void)?
    
    var contextMenu: FeedItemMenu?
    
    /// The containing stack view that can be used to prepend or append views to the group card
    let containerStackView = UIStackView().then {
        $0.axis = .vertical
        $0.spacing = 0
        $0.shouldGroupAccessibilityChildren = true
    }
    
    /// The title label appearing above the list of feeds
    let titleLabel = UILabel().then {
        $0.font = .systemFont(ofSize: 21, weight: .bold)
        $0.appearanceTextColor = .white
        $0.numberOfLines = 0
    }
    /// The buttons that contain each feed view
    private let buttons: [SpringButton]
    /// The feeds that are displayed within the list
    let feedViews: [FeedItemView]
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
        buttons = feedViews.map(FeedSpringButton.init)
        
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
            $0.isLayoutMarginsRelativeArrangement = true
            $0.layoutMargins = UIEdgeInsets(equalInset: 20)
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

/// A group of deal feed items placed horizontally in a card
class DealsFeedGroupView: FeedGroupView, FeedCardContent {
    var moreOffersButtonTapped: (() -> Void)?
    
    required init() {
        super.init(axis: .horizontal, feedLayout: .offer)
        
        containerStackView.addStackViewItems(
            .view(UIView().then {
                $0.backgroundColor = UIColor(white: 1.0, alpha: 0.15)
                $0.snp.makeConstraints {
                    $0.height.equalTo(1.0 / UIScreen.main.scale)
                }
                $0.isUserInteractionEnabled = false
                $0.isAccessibilityElement = false
            }),
            .view(MoreOffersButton().then {
                $0.addTarget(self, action: #selector(tappedMoreOffers), for: .touchUpInside)
            })
        )
    }
    
    @objc private func tappedMoreOffers() {
        moreOffersButtonTapped?()
    }
    
    private class MoreOffersButton: UIControl {
        private let label = UILabel().then {
            $0.appearanceTextColor = .white
            $0.text = Strings.BraveToday.moreBraveOffers
            $0.font = .systemFont(ofSize: 14, weight: .semibold)
            $0.isAccessibilityElement = false
        }
        private let disclosureIcon = UIImageView(image: UIImage(imageLiteralResourceName: "disclosure-arrow").template).then {
            $0.tintColor = .white
            $0.setContentHuggingPriority(.required, for: .horizontal)
        }
        
        override init(frame: CGRect) {
            super.init(frame: frame)
            
            let stackView = UIStackView(arrangedSubviews: [label, disclosureIcon])
            stackView.alignment = .center
            stackView.isUserInteractionEnabled = false
            addSubview(stackView)
            stackView.snp.makeConstraints {
                $0.edges.equalToSuperview().inset(UIEdgeInsets(top: 0, left: 20, bottom: 0, right: 20))
            }
            snp.makeConstraints {
                $0.height.equalTo(44)
            }
            accessibilityLabel = label.text
            accessibilityTraits.insert(.button)
            isAccessibilityElement = true
        }
        
        @available(*, unavailable)
        required init(coder: NSCoder) {
            fatalError()
        }
        
        override var isHighlighted: Bool {
            didSet {
                UIViewPropertyAnimator(duration: 0.3, dampingRatio: 1.0) {
                    self.backgroundColor = self.isHighlighted ?
                        UIColor(white: 1.0, alpha: 0.1) :
                        UIColor.clear
                }
                .startAnimation()
            }
        }
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
        set { assertionFailure("Accessibility label is inherited from a subview: \(String(describing: newValue)) ignored") }
    }
}
