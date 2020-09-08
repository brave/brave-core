// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import BraveShared
import Shared

private let log = Logger.browserLogger

class OnboardingPrivacyConsentViewController: OnboardingViewController {
     
    private var contentView: View {
        return view as! View // swiftlint:disable:this force_cast
    }
    
    override func loadView() {
        view = View(theme: theme)
    }

    override func viewDidLoad() {
        super.viewDidLoad()
        
        contentView.yesConsentButton.addTarget(self, action: #selector(yesConsentTapped), for: .touchUpInside)
        contentView.noConsentButton.addTarget(self, action: #selector(noConsentTaapped), for: .touchUpInside)
    }
    
    @objc func yesConsentTapped() {
        presentNextScreen(withPrivacyConsent: true)
    }
    
    @objc func noConsentTaapped() {
        presentNextScreen(withPrivacyConsent: false)
    }
    
    private func presentNextScreen(withPrivacyConsent: Bool) {
        Preferences.General.basicOnboardingProgress.value = OnboardingProgress.privacyConsent.rawValue
        if let urp = UserReferralProgram.shared {
            (UIApplication.shared.delegate as? AppDelegate)?
                .handleReferralLookup(urp, checkClipboard: withPrivacyConsent)
        }
        
        delegate?.presentNextScreen(current: self)
    }
    
    override func applyTheme(_ theme: Theme) {
        styleChildren(theme: theme)
        contentView.applyTheme(theme)
    }
}
