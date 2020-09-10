// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import BraveUI

/// The background component for each card
class FeedCardBackgroundView: UIVisualEffectView {
    init() {
        if #available(iOS 13.0, *) {
            super.init(effect: UIBlurEffect(style: .systemThinMaterialDark))
        } else {
            super.init(effect: UIBlurEffect(style: .dark))
        }
        
        layer.cornerRadius = 10
        if #available(iOS 13.0, *) {
            layer.cornerCurve = .continuous
        }
        clipsToBounds = true
        isUserInteractionEnabled = false
        isAccessibilityElement = false
        contentView.backgroundColor = UIColor.white.withAlphaComponent(0.17)
    }
    @available(*, unavailable)
    required init(coder: NSCoder) {
        fatalError()
    }
}

/// The card background to use when the entire card is tappable
class FeedCardBackgroundButton: SpringButton {
    /// The blurred background view
    private let backgroundView = FeedCardBackgroundView()
    
    override init(frame: CGRect) {
        super.init(frame: frame)
        
        highlightScale = 0.98
        
        addSubview(backgroundView)
        backgroundView.snp.makeConstraints {
            $0.edges.equalToSuperview()
        }
        
        layer.cornerRadius = backgroundView.layer.cornerRadius
        if #available(iOS 13.0, *) {
            layer.cornerCurve = backgroundView.layer.cornerCurve
        }
        clipsToBounds = true
    }
}
