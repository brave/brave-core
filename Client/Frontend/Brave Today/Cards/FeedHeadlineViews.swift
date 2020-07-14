// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation

class HeadlineCardView: FeedCardBackgroundButton, FeedCardContent {
    var actionHandler: ((Int, FeedItemAction) -> Void)?
    var contextMenu: FeedItemMenu?
    
    let feedView = FeedItemView(layout: .brandedHeadline).then {
        // Title label slightly different
        $0.titleLabel.font = .systemFont(ofSize: 18.0, weight: .semibold)
        $0.isUserInteractionEnabled = false
    }
    
    private var contextMenuDelegate: NSObject?
    
    required init() {
        super.init(frame: .zero)
        
        addSubview(feedView)
        feedView.snp.makeConstraints {
            $0.edges.equalToSuperview()
        }
        
        addTarget(self, action: #selector(tappedSelf), for: .touchUpInside)
        
        if #available(iOS 13.0, *) {
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
        }
    }
    
    func prepareForReuse() {
        print(self.interactions)
    }
    
    @objc private func tappedSelf() {
        actionHandler?(0, .opened())
    }
}

class SmallHeadlineCardView: HeadlineCardView {
    
    required init() {
        super.init()
        
        feedView.titleLabel.font = .systemFont(ofSize: 14, weight: .semibold)
        feedView.titleLabel.numberOfLines = 4
    }
    
    @available(*, unavailable)
    required init(coder: NSCoder) {
        fatalError()
    }
}

class SmallHeadlinePairCardView: UIView, FeedCardContent {
    var actionHandler: ((Int, FeedItemAction) -> Void)?
    var contextMenu: FeedItemMenu?
    
    private let stackView = UIStackView().then {
        $0.distribution = .fillEqually
        $0.spacing = 20
    }
    
    let smallHeadelineCardViews: (left: SmallHeadlineCardView, right: SmallHeadlineCardView) = (SmallHeadlineCardView(), SmallHeadlineCardView())
    
    required init() {
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
        if #available(iOS 13.0, *) {
            smallHeadelineCardViews.left.contextMenu = FeedItemMenu({ [weak self] _ -> UIMenu? in
                return self?.contextMenu?.menu?(0)
            })
            smallHeadelineCardViews.right.contextMenu = FeedItemMenu({ [weak self] _ -> UIMenu? in
                return self?.contextMenu?.menu?(1)
            })
        } else {
            smallHeadelineCardViews.left.contextMenu = FeedItemMenu({ [weak self] _ -> FeedItemMenu.LegacyContext? in
                return self?.contextMenu?.legacyMenu?(0)
            })
            smallHeadelineCardViews.right.contextMenu = FeedItemMenu({ [weak self] _ -> FeedItemMenu.LegacyContext? in
                return self?.contextMenu?.legacyMenu?(1)
            })
        }
        
        stackView.snp.makeConstraints {
            $0.edges.equalToSuperview()
        }
    }
    
    @available(*, unavailable)
    required init(coder: NSCoder) {
        fatalError()
    }
}
