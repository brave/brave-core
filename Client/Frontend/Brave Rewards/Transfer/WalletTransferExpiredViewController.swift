// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveUI
import Shared

class WalletTransferExpiredViewController: UIViewController, PopoverContentComponent {
    
    private let gradientBackgroundView = GradientView(
        colors: [
            .init(rgb: 0xA1A8F2),
            .init(rgb: 0x4C54D2)
        ],
        positions: [0.0, 1.0],
        startPoint: .init(),
        endPoint: .init(x: 1, y: 1)
    )
    
    private let textLabel = UILabel().then {
        $0.textColor = .white
        $0.numberOfLines = 0
        let image = #imageLiteral(resourceName: "warning-triangle").template
        $0.attributedText = {
            let imageAttachment = NSTextAttachment().then {
                $0.image = image
                var r = CGRect(size: image.size)
                r.origin.y = -4
                $0.bounds = r
            }
            let string = NSMutableAttributedString(attachment: imageAttachment)
            string.append(NSMutableAttributedString(string: " \(Strings.Rewards.transferNoLongerAvailableWarningMessage)"))
            string.addAttributes([
                    .font: UIFont.systemFont(ofSize: 16.0, weight: .medium),
                ], range: .init(location: 0, length: string.string.count))
            return string.withLineSpacing(3)
        }()
    }
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        view.addSubview(gradientBackgroundView)
        view.addSubview(textLabel)
        
        gradientBackgroundView.snp.makeConstraints {
            $0.edges.equalToSuperview()
        }
        
        textLabel.snp.makeConstraints {
            $0.edges.equalToSuperview().inset(16)
        }
        
        view.snp.makeConstraints {
            $0.width.equalTo(260)
        }
    }
    
    // MARK: - PopoverContentComponent
    
    var extendEdgeIntoArrow: Bool {
        true
    }
}
