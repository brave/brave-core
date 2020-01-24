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
    
    private var mainView: UIStackView?
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        guard let mainView = createViewFromState() else {
            assertionFailure()
            return
        }
        
        self.mainView = mainView
        
        mainView.setCustomSpacing(0, after: mainView.header)
        view.addSubview(mainView)
    }
    
    override func viewDidLayoutSubviews() {
        updateMainViewConstraints()
    }
    
    private func updateMainViewConstraints() {
        guard let mainView = mainView else { return }

        mainView.alignment = isPortraitIphone ? .fill : .center

        mainView.snp.remakeConstraints {
            $0.top.equalToSuperview().inset(28)
            $0.bottom.equalTo(view.safeAreaLayoutGuide).inset(16)

            if isPortraitIphone {
                $0.leading.trailing.equalTo(view.safeAreaLayoutGuide).inset(16)
            } else {
                let width = min(view.frame.width, 400)
                $0.width.equalTo(width)
                $0.centerX.equalToSuperview()
            }
        }
    }
    
    private var isPortraitIphone: Bool {
        traitCollection.userInterfaceIdiom == .phone && UIApplication.shared.statusBarOrientation.isPortrait
    }
    
    override func close(immediately: Bool = false) {
        Preferences.NewTabPage.brandedImageShowed.value = true
        super.close(immediately: immediately)
    }
    
    private func createViewFromState() -> NTPNotificationView? {
        var config = NTPNotificationViewConfig(textColor: .white)
        
        switch state {
        case .getPaidTurnRewardsOn, .getPaidTurnAdsOn:
            let learnMore = Strings.learnMore.withNonBreakingSpace
            config.bodyText =
                (text: "\(Strings.NTP.getPaidToSeeThisImage) \(learnMore)",
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
            let learnMore = Strings.learnMore.withNonBreakingSpace
            
            config.bodyText =
                (text: "\(Strings.NTP.youArePaidToSeeThisImage) \(learnMore)",
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
