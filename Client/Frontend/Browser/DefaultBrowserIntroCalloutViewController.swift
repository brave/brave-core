// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import Shared
import BraveShared
import BraveUI

private let log = Logger.browserLogger

class DefaultBrowserIntroCalloutViewController: UIViewController, Themeable {
    
    private let openSettingsButton = Button(type: .system).then {
        $0.setTitle(Strings.DefaultBrowserCallout.introOpenSettingsButtonText, for: .normal)
        $0.backgroundColor = BraveUX.braveOrange
        $0.titleLabel?.font = .systemFont(ofSize: 17, weight: .semibold)
        $0.appearanceTextColor = .white
        $0.snp.makeConstraints { make in
            make.height.equalTo(44)
        }
        $0.contentEdgeInsets = .init(top: 0, left: 25, bottom: 0, right: 25)
        $0.layer.cornerRadius = 22
        if #available(iOS 13.0, *) {
            $0.layer.cornerCurve = .continuous
        }
    }
    
    private let cancelButton = UIButton(type: .system).then {
        $0.setTitle(Strings.DefaultBrowserCallout.introSkipButtonText, for: .normal)
        $0.titleLabel?.font = .systemFont(ofSize: 17, weight: .semibold)
        $0.appearanceTextColor = Colors.grey600
    }
    
    private let image = UIImageView()
    
    private let theme: Theme
    
    init(theme: Theme) {
        self.theme = theme
        super.init(nibName: nil, bundle: nil)
    }
    
    @available(*, unavailable)
    required init(coder: NSCoder) { fatalError() }

    override func viewDidLoad() {
        super.viewDidLoad()
        view.backgroundColor = .white
        
        setupViews()
        applyTheme(theme)
        
        cancelButton.addTarget(self, action: #selector(cancelAction), for: .touchUpInside)
        openSettingsButton.addTarget(self, action: #selector(openSettingsAction), for: .touchUpInside)
    }
    
    @objc private func cancelAction() {
        dismiss(animated: true)
    }
    
    @objc private func openSettingsAction() {
        guard let settingsUrl = URL(string: UIApplication.openSettingsURLString) else {
            log.error("Failed to unwrap iOS settings URL")
            return
        }
        UIApplication.shared.open(settingsUrl)
        
        cancelAction()
    }

    private func setupViews() {
        let textStackView = UIStackView().then {
            $0.axis = .vertical
            $0.spacing = 16
            
            $0.addStackViewItems(
                .view(UILabel().then {
                    $0.text = Strings.DefaultBrowserCallout.introPrimaryText
                    $0.numberOfLines = 0
                    $0.font = .systemFont(ofSize: 28)
                    $0.textAlignment = .center
                }),
                .view(UILabel().then {
                    $0.text = Strings.DefaultBrowserCallout.introSecondaryText
                    $0.numberOfLines = 0
                    $0.textAlignment = .center
                    $0.font = .systemFont(ofSize: 17)
                }),
                .view(UILabel().then {
                    $0.text = Strings.DefaultBrowserCallout.introTertiaryText
                    $0.numberOfLines = 0
                    $0.textAlignment = .center
                    $0.font = .systemFont(ofSize: 17)
                    $0.appearanceTextColor = #colorLiteral(red: 0.5254901961, green: 0.5568627451, blue: 0.5882352941, alpha: 1)
                })
            )
        }
        
        let buttonsStackView = UIStackView().then {
            $0.axis = .vertical
            
            $0.addStackViewItems(
                .view(openSettingsButton),
                .customSpace(8),
                .view(cancelButton)
            )
        }
        
        let contentStackView = UIStackView().then {
            $0.axis = .vertical
            
            $0.addStackViewItems(
                .view(textStackView),
                .customSpace(24),
                .view(buttonsStackView))
        }
        
        let mainStackView = UIStackView().then {
            $0.axis = .vertical
            $0.distribution = .equalSpacing
            
            $0.addStackViewItems(
                .view(image.then {
                    $0.contentMode = .scaleAspectFit
                    $0.setContentCompressionResistancePriority(.defaultLow, for: .vertical)
                }),
                .view(contentStackView)
            )
        }
        
        view.addSubview(mainStackView)
        mainStackView.snp.makeConstraints {
            $0.edges.equalToSuperview().inset(36)
        }
    }
    
    // MARK: - Themeable
    func applyTheme(_ theme: Theme) {
        view.appearanceBackgroundColor = theme.isDark ? BraveUX.popoverDarkBackground : .white
        image.image = theme.isDark ? #imageLiteral(resourceName: "default_browser_intro_dark") : #imageLiteral(resourceName: "default_browser_intro")
    }
}
