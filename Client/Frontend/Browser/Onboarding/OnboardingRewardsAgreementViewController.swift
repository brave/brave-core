// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Shared
import BraveShared
import BraveRewards

class OnboardingRewardsAgreementViewController: OnboardingViewController {

    private var loadingView = UIActivityIndicatorView(style: .white)

    private var contentView: View {
        return view as! View // swiftlint:disable:this force_cast
    }
    
    override func loadView() {
        view = View(theme: theme)
    }

    override func viewDidLoad() {
        super.viewDidLoad()
        
        let isRewardsEnabled = rewards?.ledger.isEnabled == true
        let isAdsRegionSupported = BraveAds.isCurrentRegionSupported()
        
        // The user is not new..
        if Preferences.General.basicOnboardingProgress.value != OnboardingProgress.none.rawValue || isRewardsEnabled {
            contentView.updateSubtitleText(Strings.OBRewardsDetailInAdRegion, boldWords: 2)
        } else {
            // The user is new..
            contentView.updateSubtitleText(isAdsRegionSupported ? Strings.OBRewardsDetailInAdRegion : Strings.OBRewardsDetailOutsideAdRegion, boldWords: isAdsRegionSupported ? 2 : 1)
        }
        
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
        if loadingView.superview != nil || loadingView.isAnimating {
            return
        }
        
        let titleColour = contentView.turnOnButton.titleColor(for: .normal)
        contentView.turnOnButton.setTitleColor(.clear, for: .normal)
        contentView.turnOnButton.isUserInteractionEnabled = false
        contentView.skipButton.isUserInteractionEnabled = false
        contentView.turnOnButton.addSubview(loadingView)
        loadingView.snp.makeConstraints {
            $0.center.equalToSuperview()
        }
        
        loadingView.startAnimating()
        rewards?.ledger.createWalletAndFetchDetails { [weak self] success in
            guard let self = self else { return }

            self.loadingView.stopAnimating()
            self.loadingView.removeFromSuperview()
            self.contentView.turnOnButton.setTitleColor(titleColour, for: .normal)
            self.contentView.turnOnButton.isUserInteractionEnabled = true
            self.contentView.skipButton.isUserInteractionEnabled = true
            
            if success {
                self.continueTapped()
            } else {
                let alert = UIAlertController(title: Strings.OBErrorTitle, message: Strings.OBErrorDetails, preferredStyle: .alert)
                alert.addAction(UIAlertAction(title: Strings.OBErrorOkay, style: .default, handler: nil))
                self.present(alert, animated: true, completion: nil)
            }
        }
    }
    
    override func applyTheme(_ theme: Theme) {
        styleChildren(theme: theme)
        contentView.applyTheme(theme)
    }
}
