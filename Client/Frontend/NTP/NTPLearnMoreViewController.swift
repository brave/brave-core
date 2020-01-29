// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import Shared
import BraveShared
import BraveRewardsUI
import BraveRewards
import SafariServices

/// A view controller that is presented after user taps on 'Learn more' on one of `NTPNotificationViewController` views.
class NTPLearnMoreViewController: BottomSheetViewController {
    
    private let state: BrandedImageCalloutState
    
    var linkHandler: ((URL) -> Void)?
    
    private var rewards: BraveRewards?
    
    init(state: BrandedImageCalloutState, rewards: BraveRewards?) {
        self.state = state
        self.rewards = rewards
        super.init()
    }
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        guard let mainView = createViewFromState() else {
            assertionFailure()
            return
        }
        contentView.addSubview(mainView)
        
        mainView.snp.remakeConstraints {
            $0.top.equalToSuperview().inset(28)
            $0.left.right.equalToSuperview().inset(16)
            $0.bottom.equalTo(view.safeAreaLayoutGuide).inset(28)
        }
    }
    
    func createViewFromState() -> NTPNotificationView? {
        var config = NTPNotificationViewConfig(textColor: .black)
        
        switch state {
        case .getPaidTurnRewardsOn:
            config.headerText = Strings.NTP.getPaidForThisImageTurnRewards
            
            let tos = Strings.termsOfService
            let tosPart = String(format: Strings.NTP.turnRewardsTos, tos)
            
            let hideImages = Strings.NTP.hideSponsoredImages
            let hideImagesPart = String(format: Strings.NTP.chooseToHideSponsoredImages, hideImages)
            
            let fullBodyText = "\(tosPart) \(hideImagesPart)"
            
            config.bodyText =
                (text: fullBodyText,
                 urlInfo: [tos: "tos", hideImages: "hide-sponsored-images"],
                 action: { [weak self] action in
                    if action.absoluteString == "tos" {
                        let urlString = "https://www.brave.com/terms_of_use"
                        guard let url = URL(string: urlString) else { return }
                        self?.showSFSafariViewController(url: url)
                    } else if action.absoluteString == "hide-sponsored-images" {
                        Preferences.NewTabPage.backgroundSponsoredImages.value = false
                        self?.close()
                    }
                })
            
            config.primaryButtonConfig =
                (text: Strings.NTP.turnOnBraveRewards,
                 showCoinIcon: false,
                 action: { [weak self] in
                    guard let rewards = self?.rewards else { return }
                    
                    if rewards.ledger.isWalletCreated {
                        rewards.ledger.isEnabled = true
                    } else {
                        rewards.ledger.createWalletAndFetchDetails { _ in }
                    }
                    
                    self?.close()
                })
            
            config.secondaryButtonConfig =
                (text: Strings.learnMore,
                 action: { [weak self] in
                    guard let url = URL(string: "https://brave.com/brave-rewards/") else { return }
                    self?.showSFSafariViewController(url: url)
                })
        case .getPaidTurnAdsOn:
            config.headerText = Strings.NTP.getPaidForThisImageTurnAds
            
            let hideImages = Strings.NTP.hideSponsoredImages
            let hideImagesPart = String(format: Strings.NTP.chooseToHideSponsoredImages, hideImages)
            
            config.bodyText =
                (text: hideImagesPart,
                 urlInfo: [hideImages: "hide-sponsored-images"],
                 action: { [weak self] action in
                    if action.absoluteString == "hide-sponsored-images" {
                        Preferences.NewTabPage.backgroundSponsoredImages.value = false
                        self?.close()
                    }
                })
            
            config.primaryButtonConfig =
                (text: Strings.NTP.turnOnBraveAds,
                 showCoinIcon: true,
                 action: { [weak self] in
                    guard let rewards = self?.rewards else { return }
                    
                    rewards.ads.isEnabled = true
                    self?.close()
                })
            
        case .gettingPaidAlready:
            config.headerText = Strings.NTP.youArePaidToSeeThisImage
            
            let learnMore = Strings.learnMore
            let learnMorePart = String(format: Strings.NTP.learnMoreAboutBrandedImages, learnMore)
            
            let hideImages = Strings.NTP.hideSponsoredImages
            let hideImagesPart = String(format: Strings.NTP.chooseToHideSponsoredImages, hideImages)
            
            config.bodyText =
                (text: "\(learnMorePart) \(hideImagesPart)",
                    urlInfo: [learnMore: "sponsored-images", hideImages: "hide-sponsored-images"],
                    action: { [weak self] action in
                        if action.absoluteString == "sponsored-images" {
                            guard let url = URL(string: "https://brave.com/brave-rewards/") else { return }
                            self?.linkHandler?(url)
                            self?.close()
                        } else if action.absoluteString == "hide-sponsored-images" {
                            Preferences.NewTabPage.backgroundSponsoredImages.value = false
                            self?.close()
                        }
                })
        case .dontShow, .youCanGetPaidTurnAdsOn:
            assertionFailure()
            return nil
        }
        
        return NTPNotificationView(config: config)
    }
    
    private func showSFSafariViewController(url: URL) {
        let config = SFSafariViewController.Configuration()
        
        let vc = SFSafariViewController(url: url, configuration: config)
        vc.modalPresentationStyle = .overFullScreen
        
        self.present(vc, animated: true)
    }
}
