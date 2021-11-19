// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import UIKit
import SnapKit
import BraveUI

class WelcomeNTPOnboardingController: UIViewController & PopoverContentComponent {
    private let stackView = UIStackView().then {
        $0.spacing = 8.0
        $0.alignment = .top
        $0.layoutMargins = UIEdgeInsets(equalInset: 20.0)
        $0.isLayoutMarginsRelativeArrangement = true
    }
    
    private let iconView = UIImageView().then {
        $0.contentMode = .scaleAspectFit
        $0.image = #imageLiteral(resourceName: "welcome-view-ntp-logo")
        $0.snp.makeConstraints {
            $0.size.equalTo(40)
        }
        $0.setContentHuggingPriority(.required, for: .horizontal)
        $0.setContentCompressionResistancePriority(.required, for: .horizontal)
    }
    
    private let textLabel = UILabel().then {
        $0.numberOfLines = 0
        $0.setContentHuggingPriority(.defaultLow, for: .horizontal)
        $0.setContentCompressionResistancePriority(.required, for: .horizontal)
        $0.setContentHuggingPriority(.defaultLow, for: .vertical)
        $0.setContentCompressionResistancePriority(.required, for: .vertical)
    }
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        view.addSubview(stackView)
        stackView.addArrangedSubview(iconView)
        stackView.addArrangedSubview(textLabel)
        stackView.snp.makeConstraints {
            $0.edges.equalToSuperview()
        }
    }
    
    func setText(title: String? = nil, details: String) {
        let attributedString = NSMutableAttributedString()
        if let title = title {
            attributedString.append(NSAttributedString(string: "\(title)\n", attributes: [
                .font: UIFont.preferredFont(forTextStyle: .headline)
            ]))
        }
        
        attributedString.append(NSAttributedString(string: details, attributes: [
            .font: UIFont.preferredFont(forTextStyle: .body)
        ]))
        
        textLabel.attributedText = attributedString
    }
    
    func maskedPointerView(icon: UIImage, tint: UIColor?) -> UIView {
        let view = UIView().then {
            $0.backgroundColor = .braveBackground
            $0.layer.masksToBounds = true
            $0.layer.cornerCurve = .continuous
        }
        
        let imageView = UIImageView().then {
            $0.image = icon
            $0.contentMode = .center
            $0.tintColor = tint
        }
        
        view.addSubview(imageView)
        imageView.snp.makeConstraints {
            $0.center.equalToSuperview()
            $0.width.equalTo(view)
            $0.height.equalTo(view)
        }
        
        return view
    }
}
