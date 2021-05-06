// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import SnapKit
import BraveUI

/// Non-interactive contents that appear behind the New Tab Page contents
class NewTabPageBackgroundView: UIView {
    /// The image wallpaper if the user has background images enabled
    let imageView = UIImageView().then {
        $0.contentMode = .scaleAspectFit
    }
    /// Constraints generated on `imageView` for adjusting layout based on
    /// orientation
    var imageConstraints: (portraitCenter: Constraint, landscapeCenter: Constraint)?
    /// A gradient to display over background images to ensure visibility of
    /// the NTP contents and sponsored logo
    ///
    /// Only should be displayed when the user has background images enabled
    let gradientView = GradientView(
        colors: [
            UIColor(white: 0.0, alpha: 0.5),
            UIColor(white: 0.0, alpha: 0.0),
            UIColor(white: 0.0, alpha: 0.3)
        ],
        positions: [0, 0.5, 0.8],
        startPoint: .zero,
        endPoint: CGPoint(x: 0, y: 1)
    )
    
    override init(frame: CGRect) {
        super.init(frame: frame)
        
        clipsToBounds = true
        backgroundColor = .init {
            if $0.userInterfaceStyle == .dark {
                return .secondaryBraveBackground
            }
            // We use a special color here unfortunately when there is no background because
            // favorite cells have white text
            return .init(rgb: 0x3b3e4f)
        }
        
        addSubview(imageView)
        addSubview(gradientView)
        
        gradientView.snp.makeConstraints {
            $0.top.leading.trailing.equalToSuperview()
            $0.height.greaterThanOrEqualTo(700)
            $0.bottom.equalToSuperview().priority(.low)
        }
    }
    
    @available(*, unavailable)
    required init(coder: NSCoder) {
        fatalError()
    }
}
