// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation

class SponsorCardView: FeedCardBackgroundButton, FeedCardContent {
    var actionHandler: ((Int, FeedItemAction) -> Void)?
    
    let feedView = FeedItemView(layout: .bannerThumbnail).then {
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
            let contextMenuDelegate = FeedContextMenu(handler: { [weak self] action in
                self?.actionHandler?(0, action)
            })
            addInteraction(UIContextMenuInteraction(delegate: contextMenuDelegate))
            self.contextMenuDelegate = contextMenuDelegate
        }
    }
    
    @objc private func tappedSelf() {
        actionHandler?(0, .opened())
    }
}
