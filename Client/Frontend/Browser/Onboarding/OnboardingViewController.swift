// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import BraveShared
import Shared
import BraveRewards

/// A base class to provide common implementations needed for user onboarding screens.
class OnboardingViewController: UIViewController, Themeable {
    weak var delegate: Onboardable?
    var profile: Profile
    var rewards: BraveRewards?
    var theme: Theme
    
    /// Whether the on-boarding is dark or not, based solely on passed in theme
    ///  Added explicitly to make removal easier in the future if just `theme` is used
    var dark: Bool {
        return theme.isDark
    }
    
    static func colorForTheme(_ theme: Theme) -> UIColor {
        return theme.isDark ? UIColor(rgb: 0x343A40) : UIColor(rgb: 0xFFFFFF)
    }
    
    static func getUpdatedTheme() -> Theme {
        let tabManager = (UIApplication.shared.delegate as? AppDelegate)?.browserViewController.tabManager
        return Theme.of(tabManager?.selectedTab)
    }
    
    init(profile: Profile, rewards: BraveRewards?, theme: Theme) {
        self.profile = profile
        self.rewards = rewards
        self.theme = theme
        super.init(nibName: nil, bundle: nil)
    }
    
    @available(*, unavailable)
    required init(coder: NSCoder) { fatalError() }
    
    override func viewDidLoad() {
        super.viewDidLoad()
    }
    
    /// Default behavior to present next onboarding screen.
    /// Override it to add custom behavior.
    @objc func continueTapped() {
        delegate?.presentNextScreen(current: self)
    }
    
    /// Default behavior to present previous onboarding screen.
    /// Override it to add custom behavior.
    @objc func backTapped() {
        delegate?.presentPreviousScreen(current: self)
    }
    
    /// Default behavior if skip onboarding is tapped.
    /// Override it to add custom behavior.
    @objc func skipTapped() {
        delegate?.skip()
    }
    
    struct CommonViews {
        
        static func primaryButton(text: String = Strings.OBContinueButton) -> UIButton {
            let button = RoundInterfaceButton().then {
                $0.setTitle(text, for: .normal)
                $0.backgroundColor = BraveUX.BraveOrange
                $0.contentEdgeInsets = UIEdgeInsets(top: 12, left: 25, bottom: 12, right: 25)
            }
            
            return button
        }
        
        static func secondaryButton(text: String = Strings.OBSkipButton) -> UIButton {
            let button = UIButton().then {
                $0.setTitle(text, for: .normal)
                let titleColor = #colorLiteral(red: 0.5176470588, green: 0.5333333333, blue: 0.6117647059, alpha: 1)
                $0.setTitleColor(titleColor, for: .normal)
            }
            
            return button
        }
        
        static func primaryText(_ text: String) -> UILabel {
            let label = UILabel().then {
                $0.text = text
                $0.font = UIFont.systemFont(ofSize: 16, weight: UIFont.Weight.bold)
                $0.textAlignment = .center
            }
            
            return label
        }
        
        static func secondaryText(_ text: String) -> UILabel {
            let label = UILabel().then {
                $0.text = text
                $0.font = UIFont.systemFont(ofSize: 16, weight: UIFont.Weight.regular)
                $0.textAlignment = .center
                $0.numberOfLines = 0
            }
            
            return label
        }
    }
    
    override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
        if #available(iOS 13.0, *) {
            if UITraitCollection.current.userInterfaceStyle != previousTraitCollection?.userInterfaceStyle {
                theme = OnboardingViewController.getUpdatedTheme()
                applyTheme(theme)
            }
        }
    }
    
    func applyTheme(_ theme: Theme) {
        styleChildren(theme: theme)
    }
}
