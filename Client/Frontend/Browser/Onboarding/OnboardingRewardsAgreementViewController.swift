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
        
        contentView.agreeButton.addTarget(self, action: #selector(onAgreed), for: .touchUpInside)
        contentView.cancelButton.addTarget(self, action: #selector(backTapped), for: .touchUpInside)
        
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
    private func onAgreed() {
        if loadingView.superview != nil || loadingView.isAnimating {
            return
        }
        
        let titleColour = contentView.agreeButton.titleColor(for: .normal)
        contentView.agreeButton.setTitleColor(.clear, for: .normal)
        contentView.agreeButton.isUserInteractionEnabled = false
        contentView.cancelButton.isUserInteractionEnabled = false
        contentView.agreeButton.addSubview(loadingView)
        loadingView.snp.makeConstraints {
            $0.center.equalToSuperview()
        }
        
        loadingView.startAnimating()
        rewards?.ledger.createWalletAndFetchDetails { [weak self] success in
            guard let self = self else { return }

            self.loadingView.stopAnimating()
            self.loadingView.removeFromSuperview()
            self.contentView.agreeButton.setTitleColor(titleColour, for: .normal)
            self.contentView.agreeButton.isUserInteractionEnabled = true
            self.contentView.cancelButton.isUserInteractionEnabled = true
            
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
