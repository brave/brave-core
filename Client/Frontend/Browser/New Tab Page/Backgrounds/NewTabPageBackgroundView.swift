// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import SnapKit

/// Non-interactive contents that appear behind the New Tab Page contents
class NewTabPageBackgroundView: UIView, Themeable {
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
    
    func applyTheme(_ theme: Theme) {
        if theme.isDark {
            backgroundColor = theme.colors.home
        } else {
            backgroundColor = UIColor(red: 59.0/255.0, green: 62.0/255.0, blue: 79.0/255.0, alpha: 1.0)
        }
    }
}
