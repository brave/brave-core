// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Lottie
import BraveUI
import Shared

enum OptInCardAction {
    case closedButtonTapped
    case turnOnBraveNewsButtonTapped
    case learnMoreButtonTapped
}

class BraveNewsOptInView: UIView, FeedCardContent {
    private let backgroundView = FeedCardBackgroundView()
    
    private let stackView = UIStackView().then {
        $0.axis = .vertical
        $0.alignment = .center
        $0.spacing = 16
    }
    
    let graphicAnimationView = AnimationView(name: "brave-today-welcome-graphic").then {
        $0.contentMode = .scaleAspectFit
        $0.loopMode = .loop
    }
    
    var optInCardActionHandler: ((OptInCardAction) -> Void)?
    
    private let closeButton = UIButton(type: .system).then {
        $0.setImage(#imageLiteral(resourceName: "card_close").withRenderingMode(.alwaysOriginal), for: .normal)
        $0.accessibilityLabel = Strings.close
    }
    
    let turnOnBraveNewsButton = ActionButton().then {
        $0.layer.borderWidth = 0
        $0.titleLabel?.font = .systemFont(ofSize: 16.0, weight: .semibold)
        $0.setTitleColor(.white, for: .normal)
        $0.setTitle(Strings.BraveNews.turnOnBraveNews, for: .normal)
        $0.contentEdgeInsets = UIEdgeInsets(top: 10, left: 20, bottom: 10, right: 20)
        $0.backgroundColor = .braveLighterBlurple
        $0.loaderView = LoaderView(size: .small).then {
            $0.tintColor = .white
        }
    }
    
    private let learnMoreButton = UIButton(type: .system).then {
        $0.setTitle(Strings.BraveNews.learnMoreTitle, for: .normal)
        $0.titleLabel?.font = .systemFont(ofSize: 15.0, weight: .semibold)
        $0.setTitleColor(.white, for: .normal)
    }
    
    required init() {
        super.init(frame: .zero)
        
        addSubview(backgroundView)
        addSubview(stackView)
        addSubview(closeButton)
        
        backgroundView.snp.makeConstraints {
            $0.edges.equalToSuperview()
        }
        stackView.snp.makeConstraints {
            $0.edges.equalToSuperview().inset(24)
        }
        stackView.addStackViewItems(
            .view(graphicAnimationView),
            .customSpace(30),
            .view(UILabel().then {
                $0.text = Strings.BraveNews.introCardTitle
                $0.textAlignment = .center
                $0.textColor = .white
                $0.font = .systemFont(ofSize: 18, weight: .semibold)
                $0.numberOfLines = 0
            }),
            .view(UILabel().then {
                $0.text = Strings.BraveNews.introCardBody
                $0.textAlignment = .center
                $0.textColor = .white
                $0.font = .systemFont(ofSize: 14)
                $0.numberOfLines = 0
            }),
            .view(UIStackView().then {
                $0.spacing = 4
                $0.axis = .vertical
                $0.alignment = .center
                $0.addStackViewItems(
                    .view(MaskedNewLabel()),
                    .view(UILabel().then {
                        $0.text = Strings.BraveNews.introCardNewTextBody
                        $0.textAlignment = .center
                        $0.textColor = .white
                        $0.font = .systemFont(ofSize: 13)
                        $0.numberOfLines = 0
                    })
                )
            }),
            .customSpace(24),
            .view(turnOnBraveNewsButton),
            .view(learnMoreButton)
        )
        
        closeButton.snp.makeConstraints {
            $0.top.right.equalToSuperview().inset(8)
        }
        
        closeButton.addTarget(self, action: #selector(tappedCloseButton), for: .touchUpInside)
        learnMoreButton.addTarget(self, action: #selector(tappedLearnMoreButton), for: .touchUpInside)
        turnOnBraveNewsButton.addTarget(self, action: #selector(tappedTurnOnBraveButton), for: .touchUpInside)
    }
    
    // MARK: - Actions
    
    @objc private func tappedCloseButton() {
        optInCardActionHandler?(.closedButtonTapped)
    }
    
    @objc private func tappedLearnMoreButton() {
        optInCardActionHandler?(.learnMoreButtonTapped)
    }
    
    @objc private func tappedTurnOnBraveButton() {
        optInCardActionHandler?(.turnOnBraveNewsButtonTapped)
    }
    
    @available(*, unavailable)
    required init(coder: NSCoder) {
        fatalError()
    }
    
    // MARK: - FeedCardContent
    
    var actionHandler: ((Int, FeedItemAction) -> Void)? {
        didSet {
            assertionFailure("Unused for welcome card")
        }
    }
    var contextMenu: FeedItemMenu? {
        didSet {
            assertionFailure("Unused for welcome card")
        }
    }
}

/// Displays the word "New" with a gradient mask
private class MaskedNewLabel: UIView {
    private let gradientView = BraveGradientView.gradient02.then {
        // New has a white background always behind it, so only light
        $0.overrideUserInterfaceStyle = .light
    }
    private let label = UILabel().then {
        $0.text = Strings.BraveNews.introCardNew.uppercased()
        $0.textColor = .black
        $0.font = .systemFont(ofSize: 12, weight: .bold)
    }
    override init(frame: CGRect) {
        super.init(frame: frame)
        
        backgroundColor = .white
        layer.cornerRadius = 8
        layer.cornerCurve = .continuous
        
        gradientView.mask = label
        clipsToBounds = true
        
        addSubview(gradientView)
        label.sizeToFit()
        gradientView.snp.makeConstraints {
            $0.edges.equalToSuperview().inset(UIEdgeInsets(top: 3, left: 6, bottom: 3, right: 6))
            $0.size.equalTo(label.bounds.size)
        }
        isAccessibilityElement = true
        accessibilityTraits = [.staticText]
    }
    override func layoutSubviews() {
        super.layoutSubviews()
        label.frame = gradientView.bounds
    }
    @available(*, unavailable)
    required init(coder: NSCoder) {
        fatalError()
    }
    override var accessibilityLabel: String? {
        get { label.accessibilityLabel }
        set { } // swiftlint:disable:this unused_setter_value
    }
}
