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
    $0.alignment = .leading
  }
  
  private let buttonsStackView = UIStackView().then {
    $0.axis = .horizontal
    $0.alignment = .lastBaseline
    $0.spacing = 16.0
    $0.setContentHuggingPriority(.defaultHigh, for: .vertical)
  }
  
  private let promotionalImageView = UIImageView(image: UIImage(named: "brave-search-promotion", in: .current, compatibleWith: nil)).then {
    $0.contentMode = .scaleAspectFill
    $0.snp.makeConstraints { make in
      make.width.greaterThanOrEqualTo(88)
    }
    $0.isUserInteractionEnabled = false
    $0.clipsToBounds = false
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

  private let tryButton = RoundInterfaceButton(type: .roundedRect).then {
    $0.titleLabel?.font = .preferredFont(forTextStyle: .headline)
    $0.titleLabel?.minimumScaleFactor = 0.75
    $0.setTitleColor(.white, for: .normal)
    $0.setTitle(Strings.BraveSearchPromotion.braveSearchPromotionBannerTryButtonTitle, for: .normal)
    $0.backgroundColor = .braveOrange
    $0.snp.makeConstraints { make in
      make.width.greaterThanOrEqualTo(120)
    }
    $0.contentEdgeInsets = UIEdgeInsets(top: 8, left: 12, bottom: 8, right: 12)
    $0.setContentHuggingPriority(.defaultHigh, for: .vertical)
    $0.setContentCompressionResistancePriority(.required, for: .vertical)
  }
  
  private let dismissButton = UIButton().then {
    $0.titleLabel?.font = .preferredFont(forTextStyle: .headline)
    $0.titleLabel?.minimumScaleFactor = 0.75
    $0.setTitleColor(.braveOrange, for: .normal)
    $0.setTitle(
      Preferences.BraveSearch.braveSearchPromotionCompletionState.value != BraveSearchPromotionState.maybeLaterUpcomingSession.rawValue ?
        Strings.BraveSearchPromotion.braveSearchPromotionBannerMaybeLaterButtonTitle :
        Strings.BraveSearchPromotion.braveSearchPromotionBannerDismissButtonTitle,
      for: .normal)
    $0.backgroundColor = .clear
    $0.setContentHuggingPriority(.defaultHigh, for: .vertical)
    $0.setContentCompressionResistancePriority(.required, for: .vertical)
  }

  override init(style: UITableViewCell.CellStyle, reuseIdentifier: String?) {
    super.init(style: style, reuseIdentifier: reuseIdentifier)
    
    backgroundColor = .secondaryBraveBackground
    selectionStyle = .none

    contentView.addSubview(promotionContentView)
    promotionContentView.snp.makeConstraints {
      $0.leading.equalTo(safeArea.leading).inset(8)
      $0.trailing.equalTo(safeArea.trailing).inset(8)
      $0.top.equalTo(safeArea.top)
      $0.bottom.equalTo(safeArea.bottom)
    }
    
    [tryButton, dismissButton].forEach(buttonsStackView.addArrangedSubview(_:))
    
    mainStackView.addStackViewItems(
      .view(titleLabel),
      .customSpace(8.0),
      .view(bodyLabel),
      .customSpace(16.0),
      .view(buttonsStackView)
    )
    
    buttonsStackView.snp.makeConstraints {
      $0.height.equalTo(tryButton.snp.height)
    }
    
    promotionContentView.addSubview(mainStackView)
    promotionContentView.addSubview(promotionalImageView)

    mainStackView.snp.makeConstraints {
      $0.leading.top.equalToSuperview().offset(16)
      $0.bottom.equalToSuperview().offset(-16)
      $0.trailing.equalTo(promotionalImageView.snp.leading)
    }
    
    promotionContentView.snp.makeConstraints {
      $0.edges.equalToSuperview()
    }
    
    promotionalImageView.snp.makeConstraints {
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
