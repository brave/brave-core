// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Shared

class EmptyStateOverlayView: UIView {
    
    private let logoImageView = UIImageView().then {
        $0.tintColor = .braveLabel
    }
    
    private let informationLabel = UILabel().then {
        $0.textAlignment = .center
        $0.font = .preferredFont(for: .title3, weight: .light)
        $0.textColor = .braveLabel
        $0.numberOfLines = 0
        $0.adjustsFontSizeToFitWidth = true
    }
    
    required init(description: String? = nil, icon: UIImage? = nil) {
        super.init(frame: .zero)
        
        backgroundColor = .secondaryBraveBackground
        
        if let icon = icon {
            logoImageView.image = icon.template
        }
        
        addSubview(logoImageView)

        logoImageView.snp.makeConstraints { make in
            make.centerX.equalToSuperview()
            make.size.equalTo(60)
            // Sets proper top constraint for iPhone 6 in portrait and for iPad.
            make.centerY.equalToSuperview().offset(-180).priority(100)
            // Sets proper top constraint for iPhone 4, 5 in portrait.
            make.top.greaterThanOrEqualToSuperview().offset(50)
        }

        if let description = description {
            informationLabel.text = description
        }
        
        addSubview(informationLabel)
        
        informationLabel.snp.makeConstraints { make in
            make.centerX.equalToSuperview()
            make.top.equalTo(logoImageView.snp.bottom).offset(15)
            make.left.right.equalToSuperview().inset(15)
        }
    }
    
    @available(*, unavailable)
    required init(coder: NSCoder) {
        fatalError()
    }
    
    func updateInfoLabel(with text: String) {
        informationLabel.text = text
    }
}
