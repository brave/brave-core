// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveUI
import BraveShared
import Shared

class BraveTodayErrorView: UIView, FeedCardContent {
    
    var refreshButtonTapped: (() -> Void)?
    
    private let backgroundView = FeedCardBackgroundView()
    
    private let stackView = UIStackView().then {
        $0.axis = .vertical
        $0.alignment = .center
        $0.spacing = 8
    }
    
    let refreshButton = ActionButton().then {
        $0.backgroundColor = BraveUX.braveOrange
        $0.setTitle(Strings.BraveToday.refresh, for: .normal)
        $0.titleLabel?.font = .systemFont(ofSize: 15.0, weight: .semibold)
        $0.layer.borderWidth = 0
        $0.loaderView = LoaderView(size: .small).then {
            $0.tintColor = .white
        }
        $0.contentEdgeInsets = UIEdgeInsets(top: 6, left: 12, bottom: 6, right: 12)
    }
    
    let titleLabel = UILabel().then {
        $0.textAlignment = .center
        $0.appearanceTextColor = .white
        $0.font = .systemFont(ofSize: 22, weight: .semibold)
        $0.numberOfLines = 0
    }
    
    let errorMessageLabel = UILabel().then {
        $0.textAlignment = .center
        $0.appearanceTextColor = .white
        $0.font = .systemFont(ofSize: 16)
        $0.numberOfLines = 0
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
            .customSpace(12),
            .view(titleLabel),
            .view(errorMessageLabel),
            .customSpace(20),
            .view(refreshButton)
        )
        
        refreshButton.addTarget(self, action: #selector(tappedRefreshButton), for: .touchUpInside)
    }
    
    @available(*, unavailable)
    required init(coder: NSCoder) {
        fatalError()
    }
    
    @objc private func tappedRefreshButton() {
        refreshButtonTapped?()
    }
    
    // unused
    var actionHandler: ((Int, FeedItemAction) -> Void)?
    var contextMenu: FeedItemMenu?
}
