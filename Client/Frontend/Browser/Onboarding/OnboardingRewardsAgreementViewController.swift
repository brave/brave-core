// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Shared
import BraveShared
import BraveRewards

class OnboardingRewardsAgreementViewController: OnboardingViewController {

    private var contentView: View {
        return view as! View // swiftlint:disable:this force_cast
    }
    
    override func loadView() {
        view = View(theme: theme)
    }

    override func viewDidLoad() {
        super.viewDidLoad()
        
        let isAdsRegionSupported = BraveAds.isCurrentRegionSupported()
        
        let adSupportedRegionText = Locale.current.isJapan ? Strings.OBRewardsDetailInAdRegionJapan : Strings.OBRewardsDetailInAdRegion
        contentView.updateSubtitleText(isAdsRegionSupported ? adSupportedRegionText : Strings.OBRewardsDetailOutsideAdRegion, boldWords: isAdsRegionSupported ? 2 : 1)
        
        contentView.turnOnButton.addTarget(self, action: #selector(onTurnOn), for: .touchUpInside)
        contentView.skipButton.addTarget(self, action: #selector(skipTapped), for: .touchUpInside)
        
        (view as! View).onTermsOfServicePressed = { [weak self] in  // swiftlint:disable:this force_cast
            guard let self = self else { return }
            
            self.present(OnboardingWebViewController(), animated: true, completion: nil)
        }
    }
    
    override func continueTapped() {
        Preferences.General.basicOnboardingProgress.value = OnboardingProgress.rewards.rawValue
            
        super.continueTapped()
    }
    
    @objc
    private func onTurnOn() {
        rewards?.ledger.createWalletAndFetchDetails { _ in
            //TODO: Handle errors at a later date and possibly elsewhere..
        }
        
        self.continueTapped()
    }
    
    override func applyTheme(_ theme: Theme) {
        styleChildren(theme: theme)
        contentView.applyTheme(theme)
    }
}
