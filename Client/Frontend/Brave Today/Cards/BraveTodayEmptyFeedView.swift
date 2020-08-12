// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveUI
import BraveShared
import Shared

class BraveTodayEmptyFeedView: UIView, FeedCardContent {
    
    var sourcesAndSettingsButtonTapped: (() -> Void)?
    
    private let backgroundView = FeedCardBackgroundView()
    
    private let stackView = UIStackView().then {
        $0.axis = .vertical
        $0.alignment = .center
        $0.spacing = 8
    }
    
    private let sourcesAndSettingsButton = ActionButton(type: .system).then {
        $0.layer.borderWidth = 0
        $0.titleLabel?.font = .systemFont(ofSize: 16.0, weight: .semibold)
        $0.setTitle(Strings.BraveToday.sourcesAndSettings, for: .normal)
        $0.contentEdgeInsets = UIEdgeInsets(top: 10, left: 20, bottom: 10, right: 20)
        $0.backgroundColor = UIColor.white.withAlphaComponent(0.2)
    }
    
    private let titleLabel = UILabel().then {
        $0.textAlignment = .center
        $0.appearanceTextColor = .white
        $0.font = .systemFont(ofSize: 22, weight: .semibold)
        $0.numberOfLines = 0
        $0.text = Strings.BraveToday.emptyFeedTitle
    }
    
    private let messageLabel = UILabel().then {
        $0.textAlignment = .center
        $0.appearanceTextColor = .white
        $0.font = .systemFont(ofSize: 16)
        $0.numberOfLines = 0
        $0.text = Strings.BraveToday.emptyFeedBody
    }
    
    required init() {
        super.init(frame: .zero)
        
        addSubview(backgroundView)
        addSubview(stackView)
        
        backgroundView.snp.makeConstraints {
            $0.edges.equalToSuperview()
        }
        stackView.snp.makeConstraints {
            $0.edges.equalToSuperview().inset(24)
        }
        
        stackView.addStackViewItems(
            .view(UIImageView(image: UIImage(imageLiteralResourceName: "brave-today-error"))),
            .customSpace(16),
            .view(titleLabel),
            .view(messageLabel),
            .customSpace(20),
            .view(sourcesAndSettingsButton)
        )
        
        sourcesAndSettingsButton.addTarget(self, action: #selector(tappedSettingsButton), for: .touchUpInside)
    }
    
    @available(*, unavailable)
    required init(coder: NSCoder) {
        fatalError()
    }
    
    @objc private func tappedSettingsButton() {
        sourcesAndSettingsButtonTapped?()
    }
    
    // unused
    var actionHandler: ((Int, FeedItemAction) -> Void)?
    var contextMenu: FeedItemMenu?
}
