// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import Shared
import BraveShared
import BraveRewardsUI
import BraveRewards

class ClaimRewardsNTPNotificationViewController: TranslucentBottomSheet {
    
    private let rewards: BraveRewards
    private var mainView: UIStackView?
    
    init(rewards: BraveRewards) {
        self.rewards = rewards
        super.init()
    }
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        guard let mainView = getMainView else {
            assertionFailure()
            return
        }
        
        self.mainView = mainView
        
        // Confetti background
        mainView.setCustomSpacing(0, after: mainView.header)
        mainView.body.font = .systemFont(ofSize: 14.0)
        
        let bgView = UIView().then {
            if let image = #imageLiteral(resourceName: "confetti").withAlpha(0.7) {
                $0.backgroundColor =  UIColor(patternImage: image)
            }
        }
        view.addSubview(bgView)
        bgView.snp.makeConstraints {
            $0.leading.trailing.bottom.equalToSuperview()
            // Confetti can obstruct the labels making it less readable.
            $0.height.equalTo(100)
        }
        
        if let text = mainView.body.text {
            let font = mainView.body.font ?? UIFont.systemFont(ofSize: 14, weight: .regular)
            mainView.body.attributedText =
                text.attributedText(stringToChange: Strings.NTP.goodJob, font: font, color: .white)
        }
        
        view.addSubview(mainView)
        view.bringSubviewToFront(closeButton)
    }
    
    override func viewDidLayoutSubviews() {
        updateMainViewConstraints()
        
        view.snp.remakeConstraints {
            if isPortraitPhone {
                $0.right.left.equalToSuperview()
            } else {
                let width = min(view.frame.width, 400)
                $0.width.equalTo(width)
                $0.centerX.equalToSuperview()
            }
            $0.bottom.equalToSuperview()
        }
    }
    
    private func updateMainViewConstraints() {
        guard let mainView = mainView else { return }
        
        mainView.snp.remakeConstraints {
            $0.edges.equalTo(view.safeAreaLayoutGuide).inset(16)
        }
    }
    
    private var isPortraitPhone: Bool {
        traitCollection.userInterfaceIdiom == .phone
            && UIApplication.shared.statusBarOrientation.isPortrait
    }
    
    private var getMainView: NTPNotificationView? {
        var config = NTPNotificationViewConfig(textColor: .white)
        
        guard let promo = (rewards.ledger.pendingPromotions.filter { $0.type == .ads }.first) else {
            return nil
        }
        
        let goodJob = Strings.NTP.goodJob
        let grantAmount = BATValue(promo.approximateValue).displayString
        
        let earnings = grantAmount + " " + Strings.BAT
        
        let batEarnings = String(format: Strings.NTP.earningsReport, earnings)
        
        let text = "\(goodJob) \(batEarnings)"
        
        config.bodyText = (text: text, urlInfo: [:], action: nil)
        
        config.primaryButtonConfig =
            (text: Strings.NTP.claimRewards,
             showCoinIcon: true,
             action: { [weak self] in
                Preferences.NewTabPage.attemptToShowClaimRewardsNotification.value = false
                
                self?.rewards.ledger.claimPromotion(promo) { [weak self] success in
                    if !success {
                        let alert = UIAlertController(title: Strings.genericErrorTitle, message: Strings.genericErrorBody, preferredStyle: .alert)
                        alert.addAction(UIAlertAction(title: Strings.OKString, style: .default, handler: nil))
                        self?.present(alert, animated: true)
                    }
                }

                self?.close()
            })
        
        return NTPNotificationView(config: config)
    }
}
