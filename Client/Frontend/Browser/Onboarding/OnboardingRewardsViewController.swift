// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveRewards
import BraveShared
import Shared

class OnboardingRewardsViewController: OnboardingViewController {
    private var contentView: View {
        return view as! View // swiftlint:disable:this force_cast
    }
    
    override func loadView() {
        view = View(theme: theme)
    }

    override func viewDidLoad() {
        super.viewDidLoad()
        
        let isRewardsEnabled = rewards?.ledger.isEnabled == true
        let isAdsRegionSupported = BraveAds.isSupportedRegion(Locale.current.identifier)
        
        // The user is not new..
        if Preferences.General.basicOnboardingProgress.value != OnboardingProgress.none.rawValue || isRewardsEnabled {
            contentView.updateDetailsText(Strings.OBRewardsDetailInAdRegion, boldWords: 2)
        } else {
            // The user is new..
            contentView.updateDetailsText(isAdsRegionSupported ? Strings.OBRewardsDetailInAdRegion : Strings.OBRewardsDetailOutsideAdRegion, boldWords: isAdsRegionSupported ? 2 : 1)
        }
        
        // Last flow has been modified to show "Show Me" instead of "Join"
        if isRewardsEnabled && isAdsRegionSupported {
            contentView.continueButton.setTitle(Strings.OBShowMeButton, for: .normal)
        }
        
        contentView.continueButton.addTarget(self, action: #selector(continueTapped), for: .touchUpInside)
        contentView.skipButton.addTarget(self, action: #selector(skipTapped), for: .touchUpInside)
    }
    
    override func applyTheme(_ theme: Theme) {
        styleChildren(theme: theme)
        contentView.applyTheme(theme)
    }
}
