// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation

class HeadlineCardView: FeedCardBackgroundButton {
    let feedView = FeedItemView(layout: .brandedHeadline).then {
        // Title label slightly different
        $0.titleLabel.font = .systemFont(ofSize: 18.0, weight: .semibold)
    }
    
    override init(frame: CGRect) {
        super.init(frame: frame)
        
        addSubview(feedView)
        feedView.snp.makeConstraints {
            $0.edges.equalToSuperview()
        }
    }
}

class SmallHeadlineCardView: HeadlineCardView {
    
    override init(frame: CGRect) {
        super.init(frame: frame)
        
        feedView.titleLabel.numberOfLines = 4
    }
    
    @available(*, unavailable)
    required init(coder: NSCoder) {
        fatalError()
    }
}
