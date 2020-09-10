// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import BraveUI
import Shared

/// The header which is shown when the user scrolls down into the Brave Today feed
class FeedSectionHeaderView: UIView {
    private let backgroundView: UIVisualEffectView
    private let label = UILabel().then {
        $0.text = Strings.BraveToday.braveToday
        $0.appearanceTextColor = .white
        $0.font = .systemFont(ofSize: 18, weight: .semibold)
    }
    let settingsButton = UIButton(type: .system).then {
        $0.setImage(UIImage(imageLiteralResourceName: "brave-today-settings"), for: .normal)
        $0.tintColor = .white
        $0.setContentHuggingPriority(.required, for: .horizontal)
        $0.accessibilityLabel = Strings.BraveToday.sourcesAndSettings
    }
    
    private let shadowLine = UIView().then {
        $0.backgroundColor = UIColor(white: 0.0, alpha: 0.5)
    }
    
    override init(frame: CGRect) {
        if #available(iOS 13.0, *) {
            backgroundView = UIVisualEffectView(effect: UIBlurEffect(style: .systemThinMaterialDark))
        } else {
            backgroundView = UIVisualEffectView(effect: UIBlurEffect(style: .dark))
        }
        
        super.init(frame: frame)
        
        addSubview(backgroundView)
        addSubview(shadowLine)
        
        let stackView = UIStackView(arrangedSubviews: [label, settingsButton]).then {
            $0.spacing = 20
        }
        addSubview(stackView)
        stackView.snp.makeConstraints {
            $0.edges.equalTo(safeAreaLayoutGuide).inset(UIEdgeInsets(top: 8, left: 16, bottom: 8, right: 16))
        }
        
        backgroundView.snp.makeConstraints {
            $0.edges.equalToSuperview()
        }
        shadowLine.snp.makeConstraints {
            $0.top.equalTo(backgroundView.snp.bottom)
            $0.leading.trailing.equalToSuperview()
            $0.height.equalTo(1.0 / UIScreen.main.scale)
        }
    }
    
    @available(*, unavailable)
    required init(coder: NSCoder) {
        fatalError()
    }
}
