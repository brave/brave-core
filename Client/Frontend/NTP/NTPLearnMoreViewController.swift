// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import Shared
import BraveShared
import BraveRewardsUI
import BraveRewards
import SafariServices

protocol NTPLearnMoreViewDelegate: AnyObject {
    func buttonTapped(type: NTPLearnMoreViewController.NTPButtonType)
    func learnMoreTapped()
    func hideSponsoredImagesTapped()
    func tosTapped()
}

/// A view controller that is presented after user taps on 'Learn more' on one of `NTPNotificationViewController` views.
class NTPLearnMoreViewController: BottomSheetViewController {
    
    var linkHandler: ((URL) -> Void)?
    
    private let state: BrandedImageCalloutState
    private let rewards: BraveRewards
    
    private let termsOfServiceUrl = "https://www.brave.com/terms_of_use"
    private let learnMoreAboutBraveRewardsUrl = "https://brave.com/brave-rewards/"
    
    init(state: BrandedImageCalloutState, rewards: BraveRewards) {
        self.state = state
        self.rewards = rewards
        super.init()
    }
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        guard let mainView = mainView else {
            assertionFailure()
            return
        }
        
        mainView.delegate = self
        
        contentView.addSubview(mainView)
        mainView.snp.remakeConstraints {
            $0.top.equalToSuperview().inset(28)
            $0.leading.trailing.equalToSuperview()
            $0.bottom.equalTo(view.safeAreaLayoutGuide).inset(16)
        }
    }
    
    // MARK: - View setup
    
    private var mainView: NTPLearnMoreContentView? {
        var config: NTPNotificationLearnMoreViewConfig?
        
        switch state {
        case .getPaidTurnRewardsOn:
            config = NTPNotificationLearnMoreViewConfig(
                headerText: Strings.NTP.getPaidForThisImageTurnRewards,
                buttonType: .rewards,
                tosText: true,
                learnMoreButtonText: Strings.NTP.learnMoreAboutRewards)
        case .getPaidTurnAdsOn:
            config = NTPNotificationLearnMoreViewConfig(
                headerText: Strings.NTP.getPaidForThisImageTurnAds,
                buttonType: .ads,
                tosText: false,
                learnMoreButtonText: Strings.NTP.learnMoreAboutSI)
            
        case .gettingPaidAlready:
            config = NTPNotificationLearnMoreViewConfig(
                headerText: Strings.NTP.youArePaidToSeeThisImage,
                buttonType: nil,
                tosText: false,
                learnMoreButtonText: Strings.NTP.learnMoreAboutSI,
                headerBodySpacing: 8)
        case .dontShow, .youCanGetPaidTurnAdsOn:
            assertionFailure()
            return nil
        }
        
        guard let viewConfig = config else { return nil }
        
        return NTPLearnMoreContentView(config: viewConfig)
    }
}

// MARK: - NTPLearnMoreDelegate
extension NTPLearnMoreViewController: NTPLearnMoreViewDelegate {
    func buttonTapped(type: NTPButtonType) {
        switch type {
        case .rewards:
            if rewards.ledger.isWalletCreated {
                rewards.ledger.isEnabled = true
            } else {
                rewards.ledger.createWalletAndFetchDetails { _ in }
            }
        case .ads:
            rewards.ads.isEnabled = true
        }
        
        self.close()
    }
    
    func learnMoreTapped() {
        guard let url = URL(string: learnMoreAboutBraveRewardsUrl) else { return }
        
        // When a view with 'Turn on Rewards/Ads' button is shown, tapping on 'learn more'
        // opens the website modally and doesn't close the NTP view.
        // This is so the user can go back and enable Rewards/Ads after reading the learn more url.
        if state == .getPaidTurnAdsOn || state == .getPaidTurnRewardsOn {
            self.showSFSafariViewController(url: url)
        } else {
            // Normal case, open link in current tab and close the modal.
            linkHandler?(url)
            self.close()
        }
    }
    
    func hideSponsoredImagesTapped() {
        Preferences.NewTabPage.backgroundSponsoredImages.value = false
        self.close()
    }
    
    func tosTapped() {
        guard let url = URL(string: termsOfServiceUrl) else { return }
        self.showSFSafariViewController(url: url)
    }
    
    private func showSFSafariViewController(url: URL) {
        let config = SFSafariViewController.Configuration()
        
        let vc = SFSafariViewController(url: url, configuration: config)
        vc.modalPresentationStyle = .overFullScreen
        
        self.present(vc, animated: true)
    }
}
