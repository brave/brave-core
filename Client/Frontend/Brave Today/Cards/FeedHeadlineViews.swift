// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation

class HeadlineCardView: FeedCardBackgroundButton, FeedCardContent {
    var tappedHeadline: (() -> Void)?
    
    let feedView = FeedItemView(layout: .brandedHeadline).then {
        // Title label slightly different
        $0.titleLabel.font = .systemFont(ofSize: 18.0, weight: .semibold)
        $0.isUserInteractionEnabled = false
    }
    
    required init() {
        super.init(frame: .zero)
        
        addSubview(feedView)
        feedView.snp.makeConstraints {
            $0.edges.equalToSuperview()
        }
        
        addTarget(self, action: #selector(tappedSelf), for: .touchUpInside)
    }
    
    @objc private func tappedSelf() {
        tappedHeadline?()
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
        
        stackView.snp.makeConstraints {
            $0.edges.equalToSuperview()
        }
    }
    
    @available(*, unavailable)
    required init(coder: NSCoder) {
        fatalError()
    }
}
