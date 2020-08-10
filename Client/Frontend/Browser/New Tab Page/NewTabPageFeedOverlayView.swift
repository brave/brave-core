// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveUI

/// A set of feed related components that overlay the New Tab Page when Brave Today is enabled
class NewTabPageFeedOverlayView: UIView {
    
    let headerView = FeedSectionHeaderView().then {
        $0.alpha = 0.0
        $0.setContentHuggingPriority(.required, for: .vertical)
    }
    
    let loaderView = LoaderView(size: .small).then {
        $0.tintColor = .white
        $0.isHidden = true
        $0.isUserInteractionEnabled = false
    }
    
    let newContentAvailableButton = NewContentAvailableButton().then {
        $0.isHidden = true
    }
    
    override init(frame: CGRect) {
        super.init(frame: frame)
        
        addSubview(headerView)
        addSubview(loaderView)
        addSubview(newContentAvailableButton)
        
        headerView.snp.makeConstraints {
            $0.top.leading.trailing.equalToSuperview()
        }
        
        newContentAvailableButton.snp.makeConstraints {
            $0.top.equalTo(headerView.snp.bottom).offset(16)
            $0.centerX.equalToSuperview()
        }
        
        loaderView.snp.makeConstraints {
            $0.centerX.equalToSuperview()
            $0.bottom.equalToSuperview().inset(16)
        }
    }
    
    @available(*, unavailable)
    required init(coder: NSCoder) {
        fatalError()
    }
    
    override func hitTest(_ point: CGPoint, with event: UIEvent?) -> UIView? {
        // Disable taps on this view
        if let view = super.hitTest(point, with: event), view != self, view.isDescendant(of: self) {
            return view
        }
        return nil
    }
}

class NewContentAvailableButton: SpringButton {
    
}
