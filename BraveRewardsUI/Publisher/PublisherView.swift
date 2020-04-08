/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import BraveRewards
import BraveUI

class PublisherView: UIStackView {
  
  func setVerificationStatusHidden(_ hidden: Bool) {
    verifiedLabelStackView.isHidden = hidden
  }
  
  func setStatus(_ status: PublisherStatus, externalWalletStatus: WalletStatus, hasBraveFunds: Bool) {
    if status != .notVerified {
      verificationSymbolImageView.image = UIImage(frameworkResourceNamed: "icn-verify")
      verifiedLabel.text = Strings.verified
    } else {
      verificationSymbolImageView.image = UIImage(frameworkResourceNamed: "icn-unverified")
      verifiedLabel.text = Strings.notYetVerified
    }
    if hasBraveFunds {
      // Use that balance first, therefore not showing any differently
      unverifiedDisclaimerView.isHidden = status != .notVerified
    } else {
      if externalWalletStatus == .notConnected {
        unverifiedDisclaimerView.isHidden = status != .notVerified
      } else {
        unverifiedDisclaimerView.isHidden = status == .verified
        if status == .connected {
          unverifiedDisclaimerView.text = "\(Strings.connectedPublisherDisclaimer) \(Strings.disclaimerLearnMore)"
          unverifiedDisclaimerView.setURLInfo([Strings.disclaimerLearnMore: "learn-more"])
        }
      }
    }
  }
  
  func updatePublisherName(_ name: String, provider: String) {
    publisherNameLabel.attributedText = {
      let paragraphStyle = NSMutableParagraphStyle()
      paragraphStyle.lineBreakMode = .byWordWrapping
      
      let text = NSMutableAttributedString(string: name.trimmingCharacters(in: .whitespacesAndNewlines), attributes: [
        .font: UIFont.systemFont(ofSize: 18.0, weight: .medium),
        .foregroundColor: UX.publisherNameColor
      ])
      
      if !provider.isEmpty {
        text.append(NSMutableAttributedString(string: " " + provider.trimmingCharacters(in: .whitespacesAndNewlines), attributes: [
          .font: UIFont.systemFont(ofSize: 18.0),
          .foregroundColor: UX.publisherNameColor
        ]))
      }
      
      text.addAttribute(.paragraphStyle, value: paragraphStyle, range: NSRange(location: 0, length: text.length))
      return text
    }()
  }
  
  let faviconImageView = PublisherIconCircleImageView(size: UX.faviconSize)
  
  private let publisherNameLabel = UILabel().then {
    $0.appearanceTextColor = UX.publisherNameColor
    $0.font = .systemFont(ofSize: 18.0, weight: .medium)
    $0.numberOfLines = 2
  }
  
  /// The learn more button on the unverified publisher disclaimer was tapped
  var learnMoreTapped: (() -> Void)? {
    didSet {
      unverifiedDisclaimerView.onLinkedTapped = { [weak self] _ in
        self?.learnMoreTapped?()
      }
    }
  }
  
  /// Refresh Publisher List
  var onCheckAgainTapped: (() -> Void)?
  
  // MARK: -
  
  private struct UX {
    static let faviconSize: CGFloat = 48
    static let publisherNameColor = Colors.grey900
    static let verifiedStatusColor = Colors.grey700
  }
  
  // For containing the favicon and publisherStackView (Always visible)
  private let containerStackView = UIStackView().then {
    $0.spacing = 10.0
    $0.alignment = .center
  }
  
  // For containing the publisherNameLabel and verifiedLabelStackView (Always visible)
  private let publisherStackView = UIStackView().then {
    $0.axis = .vertical
    $0.spacing = 4.0
  }
  
  // For containing verificationSymbolImageView and verifiedCheckAgainStackView
  private let verifiedLabelStackView = UIStackView().then {
    $0.spacing = 4.0
  }
  
  // ✓ or ?
  private let verificationSymbolImageView = UIImageView().then {
    $0.contentMode = .scaleAspectFit
    $0.setContentHuggingPriority(.required, for: .horizontal)
  }
  
  private let verifiedCheckAgainStackView = UIStackView().then {
    $0.spacing = 4.0
  }
  
  // "Brave Verified Publisher" / "Not yet verified"
  private let verifiedLabel = UILabel().then {
    $0.appearanceTextColor = UX.verifiedStatusColor
    $0.font = .systemFont(ofSize: 12.0)
    $0.adjustsFontSizeToFitWidth = true
  }
  
  let checkAgainButton = Button(type: .system).then {
    $0.appearanceTextColor = Colors.blue400
    $0.titleLabel?.font = .systemFont(ofSize: 12.0)
    $0.setTitle(Strings.checkAgain, for: .normal)
    $0.setContentHuggingPriority(.required, for: .horizontal)
  }
  
  private let checkAgainLoaderView = LoaderView(size: .small).then {
    $0.alpha = 0.0
  }
  
  // Only shown when unverified
  private let unverifiedDisclaimerView = LinkLabel().then {
    $0.appearanceTextColor = Colors.grey700
    $0.font = UIFont.systemFont(ofSize: 12.0)
    $0.textContainerInset = UIEdgeInsets(top: 8.0, left: 8.0, bottom: 8.0, right: 8.0)
    $0.text = "\(Strings.unverifiedPublisherDisclaimer) \(Strings.disclaimerLearnMore)"
    $0.setURLInfo([Strings.disclaimerLearnMore: "learn-more"])
    $0.backgroundColor = UIColor(white: 0.0, alpha: 0.04)
    $0.layer.cornerRadius = 4.0
  }
  
  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
  
  override init(frame: CGRect) {
    super.init(frame: frame)
    
    axis = .vertical
    spacing = 10.0
    
    addArrangedSubview(containerStackView)
    addArrangedSubview(unverifiedDisclaimerView)
    containerStackView.addArrangedSubview(faviconImageView)
    containerStackView.addArrangedSubview(publisherStackView)
    publisherStackView.addArrangedSubview(publisherNameLabel)
    publisherStackView.addArrangedSubview(verifiedLabelStackView)
    verifiedLabelStackView.addArrangedSubview(verificationSymbolImageView)
    verifiedLabelStackView.addArrangedSubview(verifiedCheckAgainStackView)
    verifiedCheckAgainStackView.addArrangedSubview(verifiedLabel)
    verifiedCheckAgainStackView.addArrangedSubview(checkAgainButton)
    verifiedCheckAgainStackView.addSubview(checkAgainLoaderView)
    
    faviconImageView.snp.makeConstraints {
      $0.size.equalTo(UX.faviconSize)
    }
    
    checkAgainButton.snp.makeConstraints {
      $0.height.equalTo(20)
    }
    
    checkAgainLoaderView.snp.makeConstraints {
      $0.centerY.equalTo(checkAgainButton)
      $0.right.equalTo(checkAgainButton.snp.right)
    }
    
    checkAgainButton.addTarget(self, action: #selector(onCheckAgainPressed(_:)), for: .touchUpInside)
  }
  
  @objc
  private func onCheckAgainPressed(_ button: Button) {
    onCheckAgainTapped?()
  }
  
  func setCheckAgainIsLoading(_ loading: Bool) {
    let animatingOutView = loading ? checkAgainButton : checkAgainLoaderView
    let animatingInView = loading ? checkAgainLoaderView : checkAgainButton
    
    if loading {
      checkAgainLoaderView.start()
    }
    
    UIView.animateKeyframes(withDuration: 0.45, delay: 0, options: [], animations: {
      UIView.addKeyframe(withRelativeStartTime: 0, relativeDuration: 0.2, animations: {
        animatingOutView.alpha = 0.0
      })
      UIView.addKeyframe(withRelativeStartTime: 0.25, relativeDuration: 0.2, animations: {
        animatingInView.alpha = 1.0
      })
    }, completion: { _ in
      if !loading {
        self.checkAgainLoaderView.stop()
        self.checkAgainLoaderView.alpha = 0.0
      }
    })
  }
}
