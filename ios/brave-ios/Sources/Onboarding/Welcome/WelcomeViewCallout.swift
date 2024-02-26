// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveUI
import Foundation
import Shared
import SnapKit
import UIKit

public enum WelcomeViewCalloutState {
  public struct WelcomeViewDefaultBrowserDetails {
    var title: String
    var toggleTitle: String?
    var toggleStatus: Bool
    var details: String
    var secondaryDetails: String?
    var linkDescription: String?
    var primaryButtonTitle: String
    var secondaryButtonTitle: String?
    var toggleAction: ((Bool) -> Void)?
    var linkAction: ((URL) -> Void)?
    var primaryButtonAction: (() -> Void)
    var secondaryButtonAction: (() -> Void)?

    public init(
      title: String,
      toggleTitle: String? = nil,
      toggleStatus: Bool = true,
      details: String,
      secondaryDetails: String? = nil,
      linkDescription: String? = nil,
      primaryButtonTitle: String,
      secondaryButtonTitle: String? = nil,
      toggleAction: ((Bool) -> Void)? = nil,
      linkAction: ((URL) -> Void)? = nil,
      primaryButtonAction: @escaping () -> Void,
      secondaryButtonAction: (() -> Void)? = nil
    ) {
      self.title = title
      self.toggleTitle = toggleTitle
      self.toggleStatus = toggleStatus
      self.details = details
      self.secondaryDetails = secondaryDetails
      self.linkDescription = linkDescription
      self.primaryButtonTitle = primaryButtonTitle
      self.secondaryButtonTitle = secondaryButtonTitle
      self.toggleAction = toggleAction
      self.linkAction = linkAction
      self.primaryButtonAction = primaryButtonAction
      self.secondaryButtonAction = secondaryButtonAction
    }
  }

  case loading
  case welcome(title: String)
  case defaultBrowser(info: WelcomeViewDefaultBrowserDetails)
  case settings(title: String, details: String)
  case p3a(info: WelcomeViewDefaultBrowserDetails)
  case defaultBrowserCallout(info: WelcomeViewDefaultBrowserDetails)
}

class WelcomeViewCallout: UIView {
  private struct UX {
    static let padding = 7.0
    static let contentPadding = 24.0
    static let cornerRadius = 16.0
    static let verticalLayoutMargin = 15.0
    static let horizontalLayoutMargin = 8.0
  }

  private let backgroundView = RoundedBackgroundView(cornerRadius: UX.cornerRadius)

  private let arrowView = CalloutArrowView().then {
    $0.backgroundColor = .secondaryBraveBackground
  }

  private let contentStackView = UIStackView().then {
    $0.axis = .vertical
    $0.layoutMargins = UIEdgeInsets(equalInset: UX.contentPadding)
    $0.isLayoutMarginsRelativeArrangement = true
  }

  // MARK: - Content

  private let titleLabel = UILabel().then {
    $0.textColor = .bravePrimary
    $0.textAlignment = .center
    $0.numberOfLines = 0
    $0.minimumScaleFactor = 0.5
    $0.adjustsFontSizeToFitWidth = true
    $0.setContentHuggingPriority(.required, for: .horizontal)
    $0.setContentCompressionResistancePriority(.required, for: .horizontal)
  }

  private let actionToggle = WelcomeShareActionToggle()

  private let detailsLabel = UILabel().then {
    $0.textColor = .bravePrimary
    $0.textAlignment = .left
    $0.numberOfLines = 0
    $0.setContentCompressionResistancePriority(.defaultLow, for: .vertical)
    $0.setContentHuggingPriority(.required, for: .horizontal)
    $0.setContentCompressionResistancePriority(.required, for: .horizontal)
  }

  private let secondaryDetailsLabel = UILabel().then {
    $0.textColor = .bravePrimary
    $0.textAlignment = .left
    $0.numberOfLines = 0
    $0.setContentCompressionResistancePriority(.defaultLow, for: .vertical)
    $0.setContentHuggingPriority(.required, for: .horizontal)
    $0.setContentCompressionResistancePriority(.required, for: .horizontal)
  }

  private let actionDescriptionLabel = LinkLabel().then {
    $0.textColor = .bravePrimary
    $0.textAlignment = .left
    $0.textContainer.lineFragmentPadding = 0
    $0.textContainerInset = .zero
    $0.setContentCompressionResistancePriority(.defaultLow, for: .vertical)
    $0.setContentHuggingPriority(.required, for: .horizontal)
    $0.setContentCompressionResistancePriority(.required, for: .horizontal)
  }

  private let primaryButton = RoundInterfaceButton(type: .custom).then {
    $0.configuration = .filled()
    $0.configuration?.showsActivityIndicator = false
    $0.configuration?.imagePadding = 5
    $0.configuration?.activityIndicatorColorTransformer = UIConfigurationColorTransformer({ _ in
      .white
    })
    $0.configuration?.baseForegroundColor = .white
    $0.configuration?.baseBackgroundColor = .braveBlurpleTint

    $0.titleLabel?.numberOfLines = 0
    $0.titleLabel?.minimumScaleFactor = 0.7
    $0.titleLabel?.adjustsFontSizeToFitWidth = true
  }

  private let secondaryButtonContentView = UIStackView().then {
    $0.axis = .horizontal
    $0.spacing = 15.0
    $0.isHidden = true
    $0.alpha = 0.0
    $0.layoutMargins = UIEdgeInsets(top: 0.0, left: 15.0, bottom: 0.0, right: 15.0)
    $0.isLayoutMarginsRelativeArrangement = true
  }

  private let secondaryLabel = UILabel().then {
    $0.textColor = .bravePrimary
    $0.textAlignment = .center
    $0.numberOfLines = 0
    $0.setContentHuggingPriority(.required, for: .horizontal)
    $0.setContentHuggingPriority(.required, for: .vertical)
    $0.setContentCompressionResistancePriority(.required, for: .vertical)
  }

  private let secondaryButton = RoundInterfaceButton(type: .custom).then {
    $0.setTitleColor(.braveBlurpleTint, for: .normal)
    $0.backgroundColor = .clear
    $0.setContentHuggingPriority(.required, for: .horizontal)
    $0.setContentCompressionResistancePriority(.required, for: .horizontal)
    $0.titleLabel?.numberOfLines = 0
    $0.titleLabel?.minimumScaleFactor = 0.7
    $0.titleLabel?.adjustsFontSizeToFitWidth = true
  }

  private var verticalLayoutMargin = UX.verticalLayoutMargin
  private var horizontalLayoutMargin = UX.horizontalLayoutMargin

  private(set) var state: WelcomeViewCalloutState?

  var isBottomArrowHidden: Bool = false {
    didSet {
      arrowView.isHidden = isBottomArrowHidden
    }
  }

  var isLoading = false {
    didSet {
      primaryButton.setNeedsUpdateConfiguration()
    }
  }

  init() {
    super.init(frame: .zero)
    doLayout()

    [
      titleLabel, actionToggle, detailsLabel, secondaryDetailsLabel, actionDescriptionLabel,
      primaryButton, secondaryButtonContentView,
    ].forEach {
      contentStackView.addArrangedSubview($0)

      $0.alpha = 0.0
      $0.isHidden = true
    }

    [primaryButton, secondaryButton].forEach {
      $0.contentMode = .top
      $0.snp.makeConstraints {
        $0.height.equalTo(44.0)
      }
    }

    [secondaryLabel, secondaryButton].forEach {
      secondaryButtonContentView.addArrangedSubview($0)
    }

    [titleLabel, actionToggle, detailsLabel, secondaryDetailsLabel, actionDescriptionLabel].forEach
    {
      $0.contentMode = .top
    }

    if traitCollection.horizontalSizeClass == .regular
      && traitCollection.verticalSizeClass == .regular
    {
      verticalLayoutMargin = 3 * UX.verticalLayoutMargin / 2
      horizontalLayoutMargin = 3 * UX.horizontalLayoutMargin / 2
    }
  }

  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  private func doLayout() {
    arrowView.removeFromSuperview()
    contentStackView.removeFromSuperview()

    addSubview(backgroundView)
    addSubview(contentStackView)
    addSubview(arrowView)
    arrowView.transform = CGAffineTransform(rotationAngle: .pi)

    contentStackView.snp.makeConstraints {
      if traitCollection.horizontalSizeClass == .compact
        && traitCollection.verticalSizeClass == .regular
      {
        $0.leading.trailing.equalToSuperview().inset(UX.padding)
      } else {
        $0.centerX.equalToSuperview()
        $0.leading.trailing.equalToSuperview().priority(.high)
        $0.width.equalToSuperview().multipliedBy(0.5)
      }
      $0.top.equalToSuperview()
      $0.bottom.equalTo(arrowView.snp.top)
    }

    arrowView.snp.makeConstraints {
      $0.centerX.equalToSuperview()
      $0.bottom.equalToSuperview().inset(8)
      $0.width.equalTo(20.0)
      $0.height.equalTo(13.0)
    }

    backgroundView.snp.makeConstraints {
      $0.edges.equalTo(contentStackView.snp.edges)
    }
  }

  func setState(state: WelcomeViewCalloutState) {
    self.state = state

    primaryButton.removeAction(
      identifiedBy: .init(rawValue: "primary.action"),
      for: .primaryActionTriggered
    )
    secondaryButton.removeAction(
      identifiedBy: .init(rawValue: "secondary.action"),
      for: .primaryActionTriggered
    )

    switch state {
    case .loading:
      backgroundView.isHidden = true
      arrowView.isHidden = true

      titleLabel.do {
        $0.isHidden = true
      }

      detailsLabel.do {
        $0.alpha = 0.0
        $0.isHidden = true
      }

      primaryButton.do {
        $0.alpha = 0.0
        $0.isHidden = true
      }

      secondaryLabel.do {
        $0.alpha = 0.0
        $0.isHidden = true
      }

      secondaryButton.do {
        $0.alpha = 0.0
        $0.isHidden = true
      }

      secondaryButtonContentView.do {
        $0.alpha = 0.0
        $0.isHidden = true
      }
    case .welcome(let title):
      contentStackView.do {
        $0.layoutMargins = UIEdgeInsets(top: 0, left: -15, bottom: 0, right: -15)
      }
      backgroundView.isHidden = true
      arrowView.isHidden = true

      titleLabel.do {
        $0.text = title
        $0.textAlignment = .center
        $0.font = .preferredFont(for: .largeTitle, weight: .semibold)
        $0.textColor = .bravePrimary.resolvedColor(with: .init(userInterfaceStyle: .dark))
        $0.alpha = 1.0
        $0.isHidden = false
      }

      detailsLabel.do {
        $0.alpha = 0.0
        $0.isHidden = true
      }

      secondaryDetailsLabel.do {
        $0.alpha = 0.0
        $0.isHidden = true
      }

      primaryButton.do {
        $0.alpha = 0.0
        $0.isHidden = true
      }

      secondaryLabel.do {
        $0.alpha = 0.0
        $0.isHidden = true
      }

      secondaryButton.do {
        $0.alpha = 0.0
        $0.isHidden = true
      }

      secondaryButtonContentView.do {
        $0.alpha = 0.0
        $0.isHidden = true
      }
    case .defaultBrowser(let info):
      contentStackView.do {
        $0.layoutMargins = UIEdgeInsets(
          top: 2 * verticalLayoutMargin,
          left: 30,
          bottom: verticalLayoutMargin,
          right: 30
        )
      }
      titleLabel.do {
        $0.text = info.title
        $0.textAlignment = .left
        $0.textColor = .bravePrimary
        $0.font = .preferredFont(for: .title3, weight: .bold)
        $0.alpha = 1.0
        $0.isHidden = false
      }

      detailsLabel.do {
        $0.text = info.details
        $0.font = .preferredFont(for: .body, weight: .regular)
        $0.alpha = 1.0
        $0.isHidden = false
      }

      secondaryDetailsLabel.do {
        $0.text = info.secondaryDetails
        $0.font = .preferredFont(for: .body, weight: .bold)
        $0.alpha = 1.0
        $0.isHidden = false
      }

      primaryButton.do {
        $0.setTitle(info.primaryButtonTitle, for: .normal)
        $0.titleLabel?.font = .preferredFont(for: .body, weight: .regular)
        $0.addAction(
          UIAction(
            identifier: .init(rawValue: "primary.action"),
            handler: { _ in
              info.primaryButtonAction()
            }
          ),
          for: .touchUpInside
        )
        $0.alpha = 1.0
        $0.isHidden = false
      }

      secondaryLabel.do {
        $0.alpha = 0.0
        $0.isHidden = true
      }

      secondaryButton.do {
        $0.setTitle(info.secondaryButtonTitle, for: .normal)
        $0.titleLabel?.font = .preferredFont(for: .title3, weight: .bold)
        $0.addAction(
          UIAction(
            identifier: .init(rawValue: "secondary.action"),
            handler: { _ in
              info.secondaryButtonAction?()
            }
          ),
          for: .touchUpInside
        )
        $0.alpha = 1.0
        $0.isHidden = false
      }

      secondaryButtonContentView.do {
        $0.alpha = 1.0
        $0.isHidden = false
      }

      contentStackView.setCustomSpacing(horizontalLayoutMargin, after: titleLabel)
      contentStackView.setCustomSpacing(horizontalLayoutMargin, after: detailsLabel)
      contentStackView.setCustomSpacing(2 * horizontalLayoutMargin, after: secondaryDetailsLabel)
      contentStackView.setCustomSpacing(horizontalLayoutMargin, after: primaryButton)
    case .settings(let title, let details):
      contentStackView.do {
        $0.layoutMargins = UIEdgeInsets(top: 120, left: -30, bottom: -20, right: -30)
      }

      backgroundView.isHidden = true
      arrowView.isHidden = true

      titleLabel.do {
        $0.text = title
        $0.textColor = .bravePrimary.resolvedColor(with: .init(userInterfaceStyle: .dark))
        $0.textAlignment = .center
        $0.font = .preferredFont(for: .title1, weight: .semibold)
        $0.alpha = 1.0
        $0.isHidden = false
      }

      detailsLabel.do {
        $0.text = details
        $0.textColor = .bravePrimary.resolvedColor(with: .init(userInterfaceStyle: .dark))
        $0.textAlignment = .center
        $0.font = .preferredFont(for: .title3, weight: .regular)
        $0.alpha = 0.0
        $0.isHidden = false
      }

      secondaryDetailsLabel.do {
        $0.alpha = 0.0
        $0.isHidden = true
      }

      primaryButton.do {
        $0.alpha = 0.0
        $0.isHidden = true
      }

      secondaryLabel.do {
        $0.alpha = 0.0
        $0.isHidden = true
      }

      secondaryButton.do {
        $0.alpha = 0.0
        $0.isHidden = true
      }

      secondaryButtonContentView.do {
        $0.alpha = 0.0
        $0.isHidden = true
      }

      contentStackView.setCustomSpacing(20.0, after: titleLabel)
    case .p3a(let info):
      contentStackView.do {
        $0.layoutMargins = UIEdgeInsets(
          top: 2 * verticalLayoutMargin,
          left: 30,
          bottom: verticalLayoutMargin,
          right: 30
        )
      }
      titleLabel.do {
        $0.text = info.title
        $0.textAlignment = .left
        $0.textColor = .bravePrimary
        $0.font = .preferredFont(for: .title3, weight: .bold)
        $0.alpha = 1.0
        $0.isHidden = false
      }

      actionToggle.do {
        $0.text = info.toggleTitle
        $0.font = .preferredFont(for: .body, weight: .regular)
        $0.isOn = info.toggleStatus
        $0.onToggleChanged = info.toggleAction
        $0.alpha = 1.0
        $0.isHidden = false
      }

      detailsLabel.do {
        $0.text = info.details
        $0.font = .preferredFont(for: .footnote, weight: .regular)
        $0.alpha = 1.0
        $0.isHidden = false
      }

      secondaryDetailsLabel.do {
        $0.alpha = 0.0
        $0.isHidden = true
      }

      actionDescriptionLabel.do {
        $0.font = .preferredFont(for: .footnote, weight: .regular)
        $0.onLinkedTapped = info.linkAction
        if let linkDescription = info.linkDescription {
          $0.text = String(format: linkDescription)
          $0.setURLInfo([linkDescription: "p3a"])
        }
        $0.alpha = 1.0
        $0.isHidden = false
      }

      primaryButton.do {
        $0.configuration?.title = info.primaryButtonTitle
        $0.titleLabel?.font = .preferredFont(for: .body, weight: .regular)
        $0.addAction(
          UIAction(
            identifier: .init(rawValue: "primary.action"),
            handler: { _ in
              info.primaryButtonAction()
            }
          ),
          for: .touchUpInside
        )
        $0.alpha = 1.0
        $0.isHidden = false
        $0.configurationUpdateHandler = { button in
          button.configuration?.title = self.isLoading ? "" : info.primaryButtonTitle
          button.configuration?.showsActivityIndicator = self.isLoading
        }
      }

      secondaryLabel.do {
        $0.alpha = 0.0
        $0.isHidden = true
      }

      secondaryButton.do {
        $0.alpha = 0.0
        $0.isHidden = true
      }

      secondaryButtonContentView.do {
        $0.alpha = 0.0
        $0.isHidden = true
      }

      contentStackView.setCustomSpacing(horizontalLayoutMargin, after: titleLabel)
      contentStackView.setCustomSpacing(horizontalLayoutMargin, after: actionToggle)
      contentStackView.setCustomSpacing(horizontalLayoutMargin, after: detailsLabel)
      contentStackView.setCustomSpacing(3 * horizontalLayoutMargin, after: actionDescriptionLabel)
      contentStackView.setCustomSpacing(horizontalLayoutMargin, after: primaryButton)
    case .defaultBrowserCallout(let info):
      contentStackView.do {
        $0.layoutMargins = UIEdgeInsets(
          top: 2 * UX.verticalLayoutMargin,
          left: 20,
          bottom: UX.verticalLayoutMargin,
          right: 20
        )
      }

      titleLabel.do {
        $0.text = info.title
        $0.textAlignment = .left
        $0.font = .preferredFont(for: .title3, weight: .bold)
        $0.alpha = 1.0
        $0.isHidden = false
      }

      detailsLabel.do {
        $0.text = info.details
        $0.font = .preferredFont(for: .body, weight: .regular)
        $0.alpha = 1.0
        $0.isHidden = false
      }

      secondaryDetailsLabel.do {
        $0.text = info.details
        $0.font = .preferredFont(for: .body, weight: .bold)
        $0.alpha = 1.0
        $0.isHidden = false
      }

      primaryButton.do {
        $0.setTitle(info.primaryButtonTitle, for: .normal)
        $0.titleLabel?.font = .preferredFont(for: .body, weight: .regular)
        $0.addAction(
          UIAction(
            identifier: .init(rawValue: "primary.action"),
            handler: { _ in
              info.primaryButtonAction()
            }
          ),
          for: .touchUpInside
        )
        $0.alpha = 1.0
        $0.isHidden = false
      }

      secondaryLabel.do {
        $0.text = Strings.Callout.defaultBrowserCalloutSecondaryButtonDescription
        $0.textAlignment = .right
        $0.font = .preferredFont(for: .body, weight: .regular)
        $0.alpha = 1.0
        $0.isHidden = false
        $0.numberOfLines = 1
        $0.minimumScaleFactor = 0.7
        $0.adjustsFontSizeToFitWidth = true
      }

      secondaryButton.do {
        $0.setTitle(info.secondaryButtonTitle, for: .normal)
        $0.titleLabel?.font = .preferredFont(for: .title3, weight: .bold)
        $0.addAction(
          UIAction(
            identifier: .init(rawValue: "secondary.action"),
            handler: { _ in
              info.secondaryButtonAction?()
            }
          ),
          for: .touchUpInside
        )
        $0.alpha = 1.0
        $0.isHidden = false
      }

      secondaryButtonContentView.do {
        $0.alpha = 1.0
        $0.isHidden = false
      }

      contentStackView.setCustomSpacing(horizontalLayoutMargin, after: titleLabel)
      contentStackView.setCustomSpacing(horizontalLayoutMargin, after: detailsLabel)
      contentStackView.setCustomSpacing(2 * horizontalLayoutMargin, after: secondaryDetailsLabel)
      contentStackView.setCustomSpacing(horizontalLayoutMargin, after: primaryButton)
    }
  }

  func animateTitleViewVisibility(alpha: CGFloat, duration: TimeInterval) {
    UIView.animate(withDuration: duration) {
      self.detailsLabel.alpha = alpha
    }
  }
}

private class CalloutArrowView: UIView {
  private let maskLayer = CAShapeLayer()

  override init(frame: CGRect) {
    super.init(frame: frame)
  }

  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  override func layoutSubviews() {
    super.layoutSubviews()

    maskLayer.frame = bounds
    maskLayer.path = createTrianglePath(rect: bounds).cgPath
    layer.mask = maskLayer
  }

  private func createTrianglePath(rect: CGRect) -> UIBezierPath {
    let path = UIBezierPath()

    // Middle Top
    path.move(to: CGPoint(x: rect.midX, y: rect.minY))

    // Bottom Left
    path.addLine(to: CGPoint(x: rect.minX, y: rect.maxY))

    // Bottom Right
    path.addLine(to: CGPoint(x: rect.maxX, y: rect.maxY))

    // Middle Top
    path.addLine(to: CGPoint(x: rect.midX, y: rect.minY))
    return path
  }
}

private class RoundedBackgroundView: UIView {
  private let cornerRadius: CGFloat
  private let roundedLayer = CALayer()

  init(cornerRadius: CGFloat) {
    self.cornerRadius = cornerRadius
    super.init(frame: .zero)

    backgroundColor = .clear
  }

  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  override func layoutSubviews() {
    super.layoutSubviews()

    roundedLayer.do {
      $0.backgroundColor = UIColor.secondaryBraveBackground.cgColor
      $0.frame = bounds
      $0.cornerCurve = .continuous
      $0.mask = CAShapeLayer().then {
        $0.frame = bounds
        $0.path =
          UIBezierPath(
            roundedRect: bounds,
            byRoundingCorners: .allCorners,
            cornerRadii: CGSize(width: cornerRadius, height: cornerRadius)
          ).cgPath
      }
    }

    layer.insertSublayer(roundedLayer, at: 0)
    backgroundColor = .clear
    layer.shadowColor = UIColor.black.cgColor
    layer.shadowRadius = cornerRadius
    layer.shadowOpacity = 0.36
    layer.shadowPath =
      UIBezierPath(
        roundedRect: bounds,
        cornerRadius: cornerRadius
      ).cgPath
  }
}
