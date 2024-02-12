// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import Shared
import BraveUI
import Preferences

class BraveSearchPromotionCell: UITableViewCell {
  private struct DesignUX {
    static let paddingX: CGFloat = 15.0
    static let paddingY: CGFloat = 10.0
    static let contentBorderColor = UIColor.borderColor
    static let contentBackgroundColor = UIColor.backgroundColor
  }

  static let identifier = "BraveSearchPromotionCell"

  var trySearchEngineTapped: (() -> Void)?
  var dismissTapped: (() -> Void)?

  private let promotionContentView = UIView().then {
    $0.clipsToBounds = true
    $0.layer.cornerRadius = 16
    $0.layer.cornerCurve = .continuous
    $0.layer.borderWidth = 1.0
  }
  
  private let titleLabel = UILabel().then {
    $0.text = Strings.BraveSearchPromotion.braveSearchPromotionBannerTitle
    $0.font = .preferredFont(forTextStyle: .headline)
    $0.textColor = .bravePrimary
    $0.numberOfLines = 0
    $0.lineBreakMode = .byWordWrapping
  }

  private let subtitleLabel = UILabel().then {
    $0.text = Strings.BraveSearchPromotion.braveSearchPromotionBannerDescription
    $0.font = .preferredFont(forTextStyle: .body)
    $0.textColor = .braveLabel
    $0.numberOfLines = 0
    $0.lineBreakMode = .byWordWrapping
    $0.setContentHuggingPriority(.required, for: .vertical)
    $0.setContentCompressionResistancePriority(.required, for: .vertical)
  }
  
  private let promotionalImageView = UIImageView(
    image: UIImage(named: "brave-search-promotion", in: .module, compatibleWith: nil)).then {
    $0.contentMode = .scaleAspectFill
    $0.isUserInteractionEnabled = false
    $0.clipsToBounds = false
  }

  let tryButton = TrySearchButton()
  let dismissButton = UIButton()
  
  private let vStackView = UIStackView().then {
    $0.axis = .vertical
  }

  private let hStackView = UIStackView().then {
    $0.spacing = 9.0
    $0.distribution = .equalCentering
    $0.alignment = .fill
    $0.setContentHuggingPriority(.required, for: .horizontal)
  }

  override init(style: UITableViewCell.CellStyle, reuseIdentifier: String?) {
    super.init(style: style, reuseIdentifier: reuseIdentifier)
    
    backgroundColor = .clear
    selectionStyle = .none
    
    contentView.addSubview(promotionContentView)
    promotionContentView.snp.makeConstraints {
      $0.leading.equalTo(contentView.safeArea.leading).inset(8)
      $0.trailing.equalTo(contentView.safeArea.trailing).inset(8)
      $0.top.equalTo(contentView.safeArea.top)
      $0.bottom.equalTo(contentView.safeArea.bottom)
    }
    
    promotionContentView.addSubview(vStackView)
    promotionContentView.addSubview(promotionalImageView)

    vStackView.snp.makeConstraints {
      $0.leading.equalToSuperview().inset(12.0)
      $0.trailing.equalTo(promotionalImageView.snp.leading)
      $0.top.bottom.equalToSuperview().inset(8.0)
    }
    
    promotionalImageView.snp.makeConstraints {
      $0.width.equalTo(promotionContentView.snp.width).multipliedBy(0.20)
      $0.top.bottom.equalToSuperview()
      $0.trailing.equalToSuperview()
    }

    tryButton.addTarget(self, action: #selector(tryAction), for: .touchUpInside)
    dismissButton.addTarget(self, action: #selector(dismissAction), for: .touchUpInside)

    resetLayout()
  }

  @available(*, unavailable)
  required init(coder: NSCoder) { fatalError() }

  func resetLayout() {
    themeViews()
    doLayout()
  }

  private func doLayout() {
    vStackView.arrangedSubviews.forEach({ $0.removeFromSuperview() })
    hStackView.arrangedSubviews.forEach({ $0.removeFromSuperview() })

    let spacer = UIView().then {
      $0.setContentHuggingPriority(.defaultLow, for: .horizontal)
      $0.setContentCompressionResistancePriority(.required, for: .horizontal)
    }

    [titleLabel, subtitleLabel, hStackView].forEach({
      self.vStackView.addArrangedSubview($0)
    })

    vStackView.setCustomSpacing(8.0, after: titleLabel)
    vStackView.setCustomSpacing(22.0, after: subtitleLabel)

    [tryButton, dismissButton, spacer].forEach({
      self.hStackView.addArrangedSubview($0)
    })
  }

  private func themeViews() {
    let titleEdgeInsets = UIEdgeInsets(
      top: -DesignUX.paddingY,
      left: -DesignUX.paddingX,
      bottom: -DesignUX.paddingY,
      right: -DesignUX.paddingX)

    let contentEdgeInsets = UIEdgeInsets(
      top: DesignUX.paddingY,
      left: DesignUX.paddingX,
      bottom: DesignUX.paddingY,
      right: DesignUX.paddingX)

    tryButton.do {
      $0.titleLabel.snp.makeConstraints {
        $0.edges.equalToSuperview().inset(8.0)
      }
      $0.backgroundColor = .braveBlurpleTint
    }

    dismissButton.do {
      $0.setTitle(
        Preferences.BraveSearch.braveSearchPromotionCompletionState.value != BraveSearchPromotionState.maybeLaterUpcomingSession.rawValue ?
          Strings.BraveSearchPromotion.braveSearchPromotionBannerMaybeLaterButtonTitle :
          Strings.BraveSearchPromotion.braveSearchPromotionBannerDismissButtonTitle,
        for: .normal)
      $0.setTitleColor(.braveBlurpleTint, for: .normal)
      $0.titleLabel?.font = .preferredFont(for: .subheadline, weight: .semibold)
      $0.titleLabel?.minimumScaleFactor = 0.5
      $0.titleEdgeInsets = titleEdgeInsets
      $0.contentEdgeInsets = contentEdgeInsets
      $0.backgroundColor = .clear
    }
    
    promotionContentView.do {
     $0.layer.borderColor = DesignUX.contentBorderColor.cgColor
     $0.backgroundColor = DesignUX.contentBackgroundColor
    }
  }

  override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
    super.traitCollectionDidChange(previousTraitCollection)

    themeViews()
  }
  
  @objc func tryAction() {
    trySearchEngineTapped?()
  }

  @objc func dismissAction() {
    dismissTapped?()
  }
}

private extension UIColor {
  static var borderColor: UIColor {
    return UIColor { $0.userInterfaceStyle == .dark ?
      UIColor(rgb: 0x1f257a) :
      UIColor(rgb: 0xe2e3f8) }
  }
  
  static var backgroundColor: UIColor {
    return UIColor { $0.userInterfaceStyle == .dark ?
      UIColor(rgb: 0x1f257a).withAlphaComponent(0.4) :
      UIColor(rgb: 0xefeffb).withAlphaComponent(0.4) }
  }
}

class TrySearchButton: UIControl {
  
  let titleLabel = UILabel().then {
    $0.textColor = .white
    $0.text = Strings.BraveSearchPromotion.braveSearchPromotionBannerTryButtonTitle
    $0.font = .preferredFont(for: .subheadline, weight: .semibold)
    $0.numberOfLines = 0
    $0.lineBreakMode = .byWordWrapping
    $0.textAlignment = .center
  }

  private let backgroundView: UIVisualEffectView = {
    let backgroundView = UIVisualEffectView(effect: UIBlurEffect(style: .systemThinMaterial))
    backgroundView.isUserInteractionEnabled = false
    backgroundView.contentView.backgroundColor = .braveBlurpleTint
    backgroundView.layer.cornerRadius = 16
    backgroundView.layer.cornerCurve = .continuous
    backgroundView.layer.masksToBounds = true
    return backgroundView
  }()

  override public init(frame: CGRect) {
    super.init(frame: frame)

    addSubview(backgroundView)
    addSubview(titleLabel)

    backgroundView.snp.makeConstraints {
      $0.edges.equalTo(self)
    }

    titleLabel.snp.makeConstraints {
      $0.top.equalTo(safeArea.top).inset(8)
      $0.leading.equalTo(safeArea.leading)
      $0.bottom.equalTo(safeArea.bottom)
      $0.trailing.equalTo(safeArea.trailing)
    }

    layer.borderColor = UIColor.black.withAlphaComponent(0.15).cgColor
    layer.borderWidth = 1.0 / UIScreen.main.scale
    layer.cornerRadius = 16
    layer.shadowColor = UIColor.black.cgColor
    layer.shadowOpacity = 0.25
    layer.shadowOffset = CGSize(width: 0, height: 1)
    layer.shadowRadius = 2
  }

  public override func layoutSubviews() {
    super.layoutSubviews()

    layer.shadowPath = UIBezierPath(roundedRect: bounds, cornerRadius: layer.cornerRadius).cgPath
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }

  public override var isHighlighted: Bool {
    didSet {
      setNeedsLayout()
      let scale: CGFloat = self.isHighlighted ? 1.025 : 1
      UIViewPropertyAnimator(duration: 0.3, dampingRatio: 0.8) { [self] in
        // Only adjust scale since tx/ty could be altered
        transform.a = scale
        transform.d = scale
        layoutIfNeeded()
      }
      .startAnimation()
    }
  }
}
