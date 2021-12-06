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
        $0.textColor = .braveLabel
        $0.font = .preferredFont(forTextStyle: .body)
    }
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        view.addSubview(label)
        label.snp.makeConstraints {
            $0.leading.trailing.top.bottom.equalToSuperview().inset(32.0)
        }
    }
    
    func setData(domain: String, trackerBlocked: String, trackerCount: Int) {
        var defaultText: String
        let uuid = UUID().uuidString.replacingOccurrences(of: "-", with: "")

        if trackerCount > 0 {
            defaultText = trackerCount == 1 ?
                String(format: Strings.Onboarding.blockedAdsOnboardingPopoverSingleTrackerDescription, "[\(uuid)]", trackerCount, domain) :
                String(format: Strings.Onboarding.blockedAdsOnboardingPopoverMultipleTrackerDescription, "[\(uuid)]", trackerCount, domain)
        } else {
            defaultText = String(format: Strings.Onboarding.blockedAdsOnboardingPopoverDescriptionTwo, "[\(uuid)]", domain)
        }
        
        if let attributedText = createBlockedDescription(trackerBlocked: trackerBlocked, uuid: uuid, defaultText: defaultText) {
            label.attributedText = attributedText
        }
    }
    
    private func createBlockedDescription(trackerBlocked: String, uuid: String, defaultText: String) -> NSAttributedString? {
        let attributedText = NSMutableAttributedString()

        let defaultTextChunks = defaultText.separatedBy("[\(uuid)]")
        guard defaultTextChunks.count == 2 else {
            label.text = defaultText
            return nil
        }
        
        attributedText.append(NSAttributedString(string: defaultTextChunks[0], attributes: [
            .foregroundColor: UIColor.braveLabel,
            .font: UIFont.preferredFont(forTextStyle: .body)
        ]))
        
        attributedText.append(NSAttributedString(string: " \(trackerBlocked) ", attributes: [
            .foregroundColor: UIColor.braveLabel,
            .font: UIFont.preferredFont(for: .body, weight: .bold)
        ]))
        
        attributedText.append(NSAttributedString(
            string: "\(defaultTextChunks[1])\n\n\(Strings.Onboarding.blockedAdsOnboardingPopoverDescriptionThree)",
            attributes: [
                .foregroundColor: UIColor.braveLabel,
                .font: UIFont.preferredFont(forTextStyle: .body)
        ]))
        
        return attributedText
    }
}
