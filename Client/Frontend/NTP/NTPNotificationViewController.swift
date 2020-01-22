// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import Shared
import BraveShared
import BraveRewardsUI
import BraveRewards

class NTPNotificationViewController: TranslucentBottomSheet {
    
    private let state: BrandedImageCalloutState
    
    var learnMoreHandler: (() -> Void)?
    
    private var rewards: BraveRewards?
    
    init?(state: BrandedImageCalloutState, rewards: BraveRewards?) {
        self.state = state
        self.rewards = rewards
        super.init()
        
        if state == .dontShow { return nil }
    }
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        guard let mainView = createViewFromState() else {
            assertionFailure()
            return
        }
        
        mainView.setCustomSpacing(0, after: mainView.header)
        
        view.addSubview(mainView)
        mainView.snp.remakeConstraints {
            $0.top.equalToSuperview().inset(28)
            $0.leading.trailing.bottom.equalTo(view.safeAreaLayoutGuide).inset(16)
        }
    }
    
    override func close(immediately: Bool = false) {
        Preferences.NewTabPage.brandedImageShowed.value = true
        super.close(immediately: immediately)
    }
    
    private func createViewFromState() -> NTPNotificationView? {
        var config = NTPNotificationViewConfig(textColor: .white)
        
        switch state {
        case .getPaidTurnRewardsOn, .getPaidTurnAdsOn:
            let learnMore = Strings.learnMore
            config.bodyText =
                (text: "\(Strings.NTP.getPaidToSeeThisImage)\n\(learnMore)",
                    urlInfo: [learnMore: "learn-more"],
                    action: { [weak self] action in
                        self?.learnMoreHandler?()
                        self?.close()
                })
            
        case .youCanGetPaidTurnAdsOn:
            config.headerText = Strings.NTP.supportWebCreatorsWithTokens
            config.bodyText = (text: Strings.NTP.earnTokensByViewingAds, urlInfo: [:], action: nil)
            
            config.primaryButtonConfig =
                (text: Strings.NTP.turnOnBraveAds,
                 showCoinIcon: true,
                 action: { [weak self] in
                    guard let rewards = self?.rewards else { return }
                    
                    rewards.ads.isEnabled = true
                    self?.close()
                })
        case .gettingPaidAlready:
            let learnMore = Strings.learnMore
            
            config.bodyText =
                (text: "\(Strings.NTP.youArePaidToSeeThisImage)\n\(learnMore)",
                    urlInfo: [learnMore: "learn-more"],
                    action: { [weak self] action in
                        self?.learnMoreHandler?()
                })
        case .dontShow:
            return nil
        }
        
        return NTPNotificationView(config: config)
    }
}
