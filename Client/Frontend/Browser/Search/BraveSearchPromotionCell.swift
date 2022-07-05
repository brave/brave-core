// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import Shared
import BraveUI
import BraveShared

class BraveSearchPromotionCell: UITableViewCell {
  
  struct UX {
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
    $0.layer.borderColor = UX.contentBorderColor.cgColor
    $0.backgroundColor = UX.contentBackgroundColor
  }
  
  private let mainStackView = UIStackView().then {
    $0.axis = .vertical
    $0.alignment = .fill
  }
  
  private let buttonsStackView = UIStackView().then {
    $0.axis = .horizontal
    $0.distribution = .fillProportionally
    $0.alignment = .fill
    $0.spacing = 8.0
  }
  
  private let promotionalImageView = UIImageView(image: UIImage(named: "brave-search-promotion", in: .current, compatibleWith: nil)).then {
    $0.contentMode = .scaleAspectFill
    $0.isUserInteractionEnabled = false
  }

  private let titleLabel = UILabel().then {
    $0.text = Strings.BraveSearchPromotion.braveSearchPromotionBannerTitle
    $0.textColor = .bravePrimary
    $0.textAlignment = .left
    $0.font = .preferredFont(forTextStyle: .headline)
    $0.numberOfLines = 0
    $0.setContentHuggingPriority(.defaultHigh, for: .vertical)
    $0.setContentCompressionResistancePriority(.defaultHigh, for: .vertical)
  }

  private let bodyLabel = UILabel().then {
    $0.text = Strings.BraveSearchPromotion.braveSearchPromotionBannerDescription
    $0.textColor = .braveLabel
    $0.textAlignment = .left
    $0.font = .preferredFont(forTextStyle: .body)
    $0.numberOfLines = 0
    $0.setContentHuggingPriority(.defaultHigh, for: .vertical)
    $0.setContentCompressionResistancePriority(.defaultHigh, for: .vertical)
  }

  let tryButton = TrySearchButton().then {
    $0.titleLabel.text = Strings.BraveSearchPromotion.braveSearchPromotionBannerTryButtonTitle
  }
  
  private let dismissButton = UIButton().then {
    $0.titleLabel?.font = .preferredFont(forTextStyle: .headline)
    $0.titleLabel?.minimumScaleFactor = 0.75
    $0.titleLabel?.lineBreakMode = .byWordWrapping
    $0.titleLabel?.textAlignment = .center

    $0.setTitleColor(.braveOrange, for: .normal)
    $0.setTitle(
      Preferences.BraveSearch.braveSearchPromotionCompletionState.value != BraveSearchPromotionState.maybeLaterUpcomingSession.rawValue ?
        Strings.BraveSearchPromotion.braveSearchPromotionBannerMaybeLaterButtonTitle :
        Strings.BraveSearchPromotion.braveSearchPromotionBannerDismissButtonTitle,
      for: .normal)
    $0.backgroundColor = .clear
  }

  override init(style: UITableViewCell.CellStyle, reuseIdentifier: String?) {
    super.init(style: style, reuseIdentifier: reuseIdentifier)
    
    backgroundColor = .secondaryBraveBackground
    selectionStyle = .none

    contentView.addSubview(promotionContentView)
    promotionContentView.snp.makeConstraints {
      $0.leading.equalTo(safeArea.leading).inset(8)
      $0.trailing.equalTo(safeArea.trailing).inset(8)
      
      if #available(iOS 15, *) {
        $0.top.equalTo(safeArea.top)
        $0.bottom.equalTo(safeArea.bottom)
      } else {
        // iOS 14 table headers look different, solid color, adding small inset to make it look better.
        $0.top.equalTo(safeArea.top).inset(8)
        $0.bottom.equalTo(safeArea.bottom).inset(8)
      }
    }
    
    [tryButton, dismissButton].forEach(buttonsStackView.addArrangedSubview(_:))
    
    mainStackView.addStackViewItems(
      .view(titleLabel),
      .customSpace(8.0),
      .view(bodyLabel),
      .customSpace(16.0),
      .view(buttonsStackView)
    )
    
    promotionContentView.addSubview(promotionalImageView)
    promotionContentView.addSubview(mainStackView)

    mainStackView.snp.makeConstraints {
      $0.leading.top.equalToSuperview().offset(16)
      $0.bottom.equalToSuperview().offset(-16)
      $0.trailing.equalTo(promotionalImageView.snp.leading).offset(-16)
    }
    
    promotionContentView.snp.makeConstraints {
      $0.edges.equalToSuperview()
    }
    
    promotionalImageView.snp.makeConstraints {
      $0.width.equalTo(promotionContentView.snp.width).multipliedBy(0.25)
      $0.top.bottom.equalToSuperview()
      $0.trailing.equalToSuperview()
    }

    tryButton.addTarget(self, action: #selector(tryAction), for: .touchUpInside)
    dismissButton.addTarget(self, action: #selector(dismissAction), for: .touchUpInside)
  }

  @available(*, unavailable)
  required init(coder: NSCoder) { fatalError() }

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
    $0.font = .preferredFont(forTextStyle: .headline)
    $0.numberOfLines = 0
    $0.lineBreakMode = .byWordWrapping
    $0.textAlignment = .center
  }

  private let backgroundView: UIVisualEffectView = {
    let backgroundView = UIVisualEffectView(effect: UIBlurEffect(style: .systemThinMaterial))
    backgroundView.isUserInteractionEnabled = false
    backgroundView.contentView.backgroundColor = .braveOrange
    backgroundView.layer.cornerRadius = 16
    backgroundView.layer.cornerCurve = .continuous
    backgroundView.layer.masksToBounds = true
    return backgroundView
  }()

  override public init(frame: CGRect) {
    super.init(frame: frame)

    let stackView = UIStackView().then {
      $0.axis = .vertical
      $0.alignment = .center
      $0.isUserInteractionEnabled = false
    }

    addSubview(backgroundView)
    addSubview(stackView)
    stackView.addArrangedSubview(titleLabel)

    backgroundView.snp.makeConstraints {
      $0.edges.equalTo(self)
    }

    stackView.snp.makeConstraints {
      $0.edges.equalTo(self).inset(UIEdgeInsets(top: 8, left: 8, bottom: 8, right: 8))
    }

    backgroundColor = .clear

    layer.borderColor = UIColor.black.withAlphaComponent(0.15).cgColor
    layer.borderWidth = 1.0 / UIScreen.main.scale
    layer.cornerRadius = bounds.height / 2.0
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
