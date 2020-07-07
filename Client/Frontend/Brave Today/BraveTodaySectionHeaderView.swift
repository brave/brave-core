// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import BraveUI

class BraveTodaySectionHeaderView: UICollectionReusableView, CollectionViewReusable {
    private let backgroundView = UIVisualEffectView(effect: UIBlurEffect(style: .dark))
    private let label = UILabel().then {
        $0.text = "Brave Today" // TODO(kyle): localize
        $0.appearanceTextColor = .white
        $0.font = .systemFont(ofSize: 18, weight: .semibold)
    }
    let settingsButton = UIButton(type: .system).then {
        $0.setImage(UIImage(imageLiteralResourceName: "brave-today-settings").withRenderingMode(.alwaysOriginal), for: .normal)
        $0.setContentHuggingPriority(.required, for: .horizontal)
    }
    
    override init(frame: CGRect) {
        super.init(frame: frame)
        
        addSubview(backgroundView)
        let stackView = UIStackView(arrangedSubviews: [label, settingsButton]).then {
            $0.spacing = 20
            $0.alignment = .center
        }
        backgroundView.contentView.addSubview(stackView)
        stackView.snp.makeConstraints {
            $0.edges.equalToSuperview().inset(UIEdgeInsets(top: 8, left: 16, bottom: 8, right: 16))
        }
        
        backgroundView.snp.makeConstraints {
            $0.edges.equalToSuperview()
        }
    }
    @available(*, unavailable)
    required init(coder: NSCoder) {
        fatalError()
    }
}
