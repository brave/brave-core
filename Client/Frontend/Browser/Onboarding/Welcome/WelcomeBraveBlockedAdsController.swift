// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import UIKit
import SnapKit
import BraveUI
import BraveShared
import Shared

class WelcomeBraveBlockedAdsController: UIViewController, PopoverContentComponent {
    private let label = UILabel().then {
        $0.numberOfLines = 0
    }
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        view.addSubview(label)
        label.snp.makeConstraints {
            $0.leading.trailing.top.bottom.equalToSuperview().inset(32.0)
        }
    }
    
    func setData(domain: String, trackerBlocked: String, trackerCount: Int) {
        let text = NSMutableAttributedString()
        
        if trackerCount > 0 {
            let uuid = UUID().uuidString.replacingOccurrences(of: "-", with: "")
            let string = trackerCount == 1 ?
            String(format: Strings.Onboarding.blockedAdsOnboardingPopoverSingleTrackerDescription, "[\(uuid)]", trackerCount, domain) :
            String(format: Strings.Onboarding.blockedAdsOnboardingPopoverMultipleTrackerDescription, "[\(uuid)]", trackerCount, domain)
            let strings = string.separatedBy("[\(uuid)]")
            guard strings.count == 2 else {
                label.text = string
                return
            }
            
            text.append(NSAttributedString(string: strings[0], attributes: [
                .foregroundColor: UIColor.braveLabel,
                .font: UIFont.preferredFont(forTextStyle: .body)
            ]))
            
            text.append(NSAttributedString(string: " \(trackerBlocked) ", attributes: [
                .foregroundColor: UIColor.braveLabel,
                .font: UIFont.preferredFont(for: .body, weight: .bold)
            ]))
            
            text.append(NSAttributedString(string: "\(strings[1])\n\n\(Strings.Onboarding.blockedAdsOnboardingPopoverDescriptionThree)", attributes: [
                .foregroundColor: UIColor.braveLabel,
                .font: UIFont.preferredFont(forTextStyle: .body)
            ]))
        } else {
            let string = String(format: Strings.Onboarding.blockedAdsOnboardingPopoverDescriptionTwo, trackerBlocked, domain)
            
            text.append(NSAttributedString(string: "\(string)\n\n\(Strings.Onboarding.blockedAdsOnboardingPopoverDescriptionThree)", attributes: [
                .foregroundColor: UIColor.braveLabel,
                .font: UIFont.preferredFont(forTextStyle: .body)
            ]))
        }
        label.attributedText = text
    }
}
