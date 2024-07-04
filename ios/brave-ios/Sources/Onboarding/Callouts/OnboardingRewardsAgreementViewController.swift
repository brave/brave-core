// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShared
import BraveUI
import Foundation
import Lottie
import Preferences
import SafariServices
import Shared
import SnapKit
import Storage
import UIKit

public enum OnboardingRewardsState {
  case skipped
  case complete
}

public class OnboardingRewardsAgreementViewController: UIViewController {
  public var onOnboardingStateChanged:
    ((OnboardingRewardsAgreementViewController, _ state: OnboardingRewardsState) -> Void)?
  public var onRewardsStatusChanged: ((Bool) -> Void)?

  public init() {
    super.init(nibName: nil, bundle: nil)

    modalPresentationStyle =
      UIDevice.current.userInterfaceIdiom == .phone ? .fullScreen : .formSheet

    // Prevent dismissing the modal by swipe
    isModalInPresentation = true
    preferredContentSize = OnboardingCommon.UX.preferredModalSize
  }

  @available(*, unavailable)
  required init(coder: NSCoder) { fatalError() }

  private var contentView: View {
    return view as! View
  }

  public override func loadView() {
    view = View()
  }

  public override func viewDidLoad() {
    super.viewDidLoad()

    view.backgroundColor = .black

    contentView.turnOnButton.addTarget(self, action: #selector(turnOnTapped), for: .touchUpInside)
    contentView.skipButton.addTarget(self, action: #selector(skipTapped), for: .touchUpInside)

    contentView.onTermsOfServicePressed = { [weak self] in
      let vc = SFSafariViewController(url: .brave.termsOfUse, configuration: .init())
      vc.modalPresentationStyle = .currentContext

      self?.present(vc, animated: true)
    }

    contentView.onPrivacyPolicyPressed = { [weak self] in
      let vc = SFSafariViewController(url: .brave.privacy, configuration: .init())
      vc.modalPresentationStyle = .currentContext

      self?.present(vc, animated: true)
    }
  }

  @objc
  private func skipTapped() {
    onOnboardingStateChanged?(self, .skipped)
  }

  @objc
  private func turnOnTapped() {
    Preferences.Onboarding.basicOnboardingProgress.value = OnboardingProgress.rewards.rawValue
    onRewardsStatusChanged?(true)
    onOnboardingStateChanged?(self, .complete)
  }
}

extension OnboardingRewardsAgreementViewController {
  class View: UIView {

    var onTermsOfServicePressed: (() -> Void)?
    var onPrivacyPolicyPressed: (() -> Void)?

    let turnOnButton = OnboardingCommon.Views.primaryButton(text: Strings.yes).then {
      $0.accessibilityIdentifier = "OnboardingRewardsAgreementViewController.OBTurnOnButton"
      $0.backgroundColor = .braveBlurpleTint
      $0.titleLabel?.minimumScaleFactor = 0.75
    }

    let skipButton = OnboardingCommon.Views.secondaryButton(text: Strings.OBSkipButton).then {
      $0.accessibilityIdentifier = "OnboardingRewardsAgreementViewController.OBSkipButton"
      $0.titleLabel?.minimumScaleFactor = 0.75
    }

    private let mainStackView = UIStackView().then {
      $0.axis = .vertical
      $0.spacing = OnboardingCommon.UX.negativeSpacing
    }

    private let imageView = LottieAnimationView(name: "onboarding-rewards", bundle: .module).then {
      $0.contentMode = .scaleAspectFit
      $0.setContentCompressionResistancePriority(.defaultLow, for: .vertical)
      $0.backgroundBehavior = .pauseAndRestore
      $0.loopMode = .loop
      $0.play()
    }

    private let descriptionView = UIView().then {
      $0.backgroundColor = .braveBackground
      $0.layer.cornerRadius = 12
      $0.layer.cornerCurve = .continuous
      $0.layer.maskedCorners = [.layerMinXMinYCorner, .layerMaxXMinYCorner]
    }

    private let descriptionStackView = UIStackView().then {
      $0.axis = .vertical
      $0.spacing = 32
    }

    private let titleLabel = OnboardingCommon.Views.primaryText(Strings.OBRewardsAgreementTitle)
      .then {
        $0.numberOfLines = 0
        $0.textColor = .braveLabel
      }

    private let subtitleLabel = OnboardingCommon.Views.secondaryText(Strings.OBRewardsDetail).then {
      $0.numberOfLines = 0
      $0.textColor = .braveLabel
    }

    private lazy var descriptionLabel = UITextView().then {
      $0.delaysContentTouches = false
      $0.isEditable = false
      $0.isScrollEnabled = false
      $0.isSelectable = true
      $0.backgroundColor = .clear
      $0.textDragInteraction?.isEnabled = false
      $0.textContainerInset = .zero
      $0.font = UIFont.systemFont(ofSize: 16, weight: UIFont.Weight.regular)
      $0.delegate = self
      $0.textAlignment = .center
    }

    private lazy var textStackView = UIStackView().then { stackView in
      stackView.axis = .vertical
      stackView.spacing = 8
      stackView.layoutMargins = UIEdgeInsets(top: 0.0, left: 20.0, bottom: 0.0, right: 20.0)
      stackView.isLayoutMarginsRelativeArrangement = true

      [titleLabel, subtitleLabel, descriptionLabel].forEach {
        stackView.addArrangedSubview($0)
      }
    }

    private let buttonsStackView = UIStackView().then {
      $0.axis = .horizontal
      $0.alignment = .center
      $0.spacing = 15.0
    }

    private func updateDescriptionLabel() {
      descriptionLabel.attributedText = {
        let fontSize: CGFloat = 14.0

        let text = NSMutableAttributedString(
          string: Strings.OBRewardsAgreementDetail,
          attributes: [
            .font: UIFont.systemFont(ofSize: fontSize, weight: UIFont.Weight.regular),
            .foregroundColor: UIColor.braveLabel,
          ]
        )

        text.append(
          NSAttributedString(
            string: " ",
            attributes: [
              .font: UIFont.systemFont(ofSize: fontSize, weight: UIFont.Weight.regular),
              .foregroundColor: UIColor.braveLabel,
            ]
          )
        )

        text.append(
          NSAttributedString(
            string: Strings.OBRewardsAgreementDetailLink,
            attributes: [
              .font: UIFont.systemFont(ofSize: fontSize, weight: UIFont.Weight.regular),
              .foregroundColor: OnboardingCommon.UX.linkColor,
              .link: "brave_terms_of_service",
            ]
          )
        )

        text.append(
          NSAttributedString(
            string: " \(Strings.OBRewardsAgreementDetailsAnd) ",
            attributes: [
              .font: UIFont.systemFont(ofSize: fontSize, weight: UIFont.Weight.regular),
              .foregroundColor: UIColor.braveLabel,
            ]
          )
        )

        text.append(
          NSAttributedString(
            string: Strings.OBRewardsPrivacyPolicyDetailLink,
            attributes: [
              .font: UIFont.systemFont(ofSize: fontSize, weight: UIFont.Weight.regular),
              .foregroundColor: OnboardingCommon.UX.linkColor,
              .link: "brave_privacy_policy",
            ]
          )
        )

        let paragraphStyle = NSMutableParagraphStyle()
        paragraphStyle.lineBreakMode = .byWordWrapping
        paragraphStyle.alignment = .center

        text.addAttribute(
          .paragraphStyle,
          value: paragraphStyle,
          range: NSRange(location: 0, length: text.length)
        )

        return text
      }()

      descriptionLabel.accessibilityLabel = descriptionLabel.text
      descriptionLabel.accessibilityTraits = [.staticText, .link]
      descriptionLabel.accessibilityValue = nil
      descriptionLabel.isAccessibilityElement = true
    }

    func updateSubtitleText(_ text: String, boldWords: Int) {
      let subTitle = text.boldWords(with: self.subtitleLabel.font, amount: boldWords)
      subTitle.addAttribute(
        .foregroundColor,
        value: UIColor.braveLabel,
        range: NSRange(location: 0, length: subTitle.length)
      )
      self.subtitleLabel.attributedText = subTitle
    }

    override init(frame: CGRect) {
      super.init(frame: frame)

      addSubview(imageView)
      addSubview(mainStackView)
      mainStackView.snp.makeConstraints {
        $0.leading.trailing.bottom.equalToSuperview()
      }

      descriptionView.addSubview(descriptionStackView)
      descriptionStackView.snp.makeConstraints {
        $0.edges.equalTo(descriptionView.safeAreaLayoutGuide.snp.edges).inset(
          OnboardingCommon.UX.descriptionContentInset
        )
      }

      mainStackView.addArrangedSubview(descriptionView)

      [skipButton, turnOnButton]
        .forEach(buttonsStackView.addArrangedSubview(_:))

      [textStackView, buttonsStackView].forEach(descriptionStackView.addArrangedSubview(_:))

      skipButton.snp.makeConstraints {
        $0.width.equalTo(turnOnButton.snp.width).priority(.low)
      }

      updateDescriptionLabel()
    }

    override func layoutSubviews() {
      super.layoutSubviews()

      let size = imageView.intrinsicContentSize
      let scaleFactor = bounds.width / size.width
      let newSize = CGSize(width: size.width * scaleFactor, height: size.height * scaleFactor)

      // Design wants LESS offset on iPhone 8 than on iPhone X
      let offset = self.safeAreaInsets.top > 30 ? 0 : -OnboardingCommon.UX.animationContentInset
      imageView.frame = CGRect(
        x: 0.0,
        y: OnboardingCommon.UX.animationContentInset + offset,
        width: newSize.width,
        height: newSize.height
      )
    }

    @available(*, unavailable)
    required init(coder: NSCoder) {
      fatalError()
    }
  }
}

extension OnboardingRewardsAgreementViewController.View: UITextViewDelegate {
  @objc
  func textView(
    _ textView: UITextView,
    shouldInteractWith url: URL,
    in characterRange: NSRange,
    interaction: UITextItemInteraction
  ) -> Bool {
    if url.absoluteString == "brave_terms_of_service" {
      onTermsOfServicePressed?()
    } else if url.absoluteString == "brave_privacy_policy" {
      onPrivacyPolicyPressed?()
    }
    return false
  }

  @objc
  func textViewDidChangeSelection(_ textView: UITextView) {
    textView.delegate = nil
    textView.selectedTextRange = nil
    textView.delegate = self
  }
}

// Disabling orientation changes
extension OnboardingRewardsAgreementViewController {
  public override var preferredStatusBarStyle: UIStatusBarStyle {
    return .default
  }

  public override var supportedInterfaceOrientations: UIInterfaceOrientationMask {
    return .portrait
  }

  public override var preferredInterfaceOrientationForPresentation: UIInterfaceOrientation {
    return .portrait
  }

  public override var shouldAutorotate: Bool {
    return false
  }
}

extension String {
  fileprivate func boldWords(with font: UIFont, amount: Int) -> NSMutableAttributedString {
    let mutableDescriptionText = NSMutableAttributedString(string: self)

    let components = self.components(separatedBy: " ")
    for i in 0..<min(amount, components.count) {
      if let range = self.range(of: components[i]) {
        let nsRange = NSRange(range, in: self)
        let font = UIFont.systemFont(ofSize: font.pointSize, weight: UIFont.Weight.bold)

        mutableDescriptionText.addAttribute(
          NSAttributedString.Key.font,
          value: font,
          range: nsRange
        )
      }
    }

    return mutableDescriptionText
  }
}
