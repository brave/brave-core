// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Lottie

class BraveTodayWelcomeView: UIView, FeedCardContent {
    var actionHandler: ((Int, FeedItemAction) -> Void)?
    var contextMenu: FeedItemMenu?
    
    private let backgroundView = FeedCardBackgroundView()
    
    private let stackView = UIStackView().then {
        $0.axis = .vertical
        $0.alignment = .center
        $0.spacing = 16
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
            .view(UIImageView(image: UIImage(imageLiteralResourceName: "brave-today-welcome-graphic")).then {
                $0.setContentHuggingPriority(.required, for: .vertical)
                $0.contentMode = .scaleAspectFit
            }),
            .customSpace(30),
            .view(UILabel().then {
                $0.text = "Today’s top stories in a completely private feed, just for you."
                $0.textAlignment = .center
                $0.appearanceTextColor = .white
                $0.font = .systemFont(ofSize: 22, weight: .semibold)
                $0.numberOfLines = 0
            }),
            .view(UILabel().then {
                $0.text = "Brave Today matches your interests on your device so your personal information never leaves your browser. New content updated every hour."
                $0.textAlignment = .center
                $0.appearanceTextColor = .white
                $0.font = .systemFont(ofSize: 13)
                $0.numberOfLines = 0
            })
        )
    }
    
    @available(*, unavailable)
    required init(coder: NSCoder) {
        fatalError()
    }
}
