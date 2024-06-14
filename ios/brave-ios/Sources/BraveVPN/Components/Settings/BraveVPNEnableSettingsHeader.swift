// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveUI
import Shared
import UIKit

public class BraveVPNEnableSettingsHeader: UIView {

  public var enableVPNTapped: (() -> Void)?
  public var dismissHeaderTapped: (() -> Void)?

  private let mainStackView = UIStackView().then {
    $0.axis = .vertical
    $0.alignment = .center
    $0.spacing = 6
  }

  private let titleLabel = UILabel().then {
    // This string should not be translated
    $0.text = "Brave VPN"
    $0.textColor = .white
    $0.font = UIFont.systemFont(ofSize: 19, weight: .semibold)
  }

  private let bodyLabel = UILabel().then {
    $0.text = Strings.VPN.settingHeaderBody
    $0.numberOfLines = 0
    $0.textAlignment = .center
    $0.textColor = .white
    $0.font = UIFont.systemFont(ofSize: 15, weight: .regular)
    $0.setContentHuggingPriority(.defaultHigh, for: .vertical)
  }

  private let enableButton = RoundInterfaceButton(type: .roundedRect).then {

    let title = { () -> String in
      switch BraveVPN.vpnState {
      case .notPurchased:
        return Strings.VPN.tryForFreeButton
      case .expired:
        return Strings.learnMore
      case .purchased:
        return Strings.VPN.enableButton
      }
    }()

    $0.setTitle(title, for: .normal)
    $0.contentEdgeInsets = .init(top: 0, left: 8, bottom: 0, right: 8)
    $0.backgroundColor = .braveBlurpleTint
    $0.titleLabel?.font = UIFont.systemFont(ofSize: 16, weight: .semibold)
    $0.setTitleColor(.white, for: .normal)
    $0.snp.makeConstraints { make in
      make.height.equalTo(44)
      make.width.greaterThanOrEqualTo(120)
    }

    $0.setContentCompressionResistancePriority(.required, for: .vertical)
  }

  private let poweredByStackView = BraveVPNCommonUI.Views.poweredByView(
    textColor: .white,
    imageColor: .white
  )

  private let contentView = UIView().then {
    $0.clipsToBounds = true
    $0.layer.cornerRadius = 16
    $0.layer.cornerCurve = .continuous
    $0.backgroundColor = BraveVPNCommonUI.UX.purpleBackgroundColor
  }

  private let backgroundImage = UIImageView().then {
    if let image = UIImage(named: "enable_vpn_settings_banner", in: .module, compatibleWith: nil) {
      $0.image = image
    } else {
      assertionFailure("Could not find asset for background image")
    }
    $0.contentMode = .scaleAspectFill
  }

  private let closeButton = UIButton().then {
    if let image = UIImage(named: "card_close", in: .module, compatibleWith: nil) {
      $0.setImage(image, for: .normal)
    } else {
      assertionFailure("Could not find asset for close button")
    }

    $0.tintColor = .white
  }

  override init(frame: CGRect) {
    super.init(frame: frame)

    contentView.addSubview(backgroundImage)
    backgroundImage.snp.makeConstraints { $0.edges.equalToSuperview() }

    addSubview(contentView)

    [titleLabel, bodyLabel, enableButton, poweredByStackView].forEach(
      mainStackView.addArrangedSubview(_:)
    )

    contentView.addSubview(mainStackView)
    mainStackView.snp.makeConstraints {
      $0.top.bottom.equalToSuperview().inset(16)
      $0.leading.trailing.equalToSuperview().inset(25)
      $0.centerX.equalToSuperview()
    }

    contentView.snp.makeConstraints {
      $0.top.equalToSuperview().inset(8)
      // Settings screen on iOS 13 uses .insetGrouped style which adds their own insets.
      // No need for extra insets then.
      $0.leading.trailing.equalToSuperview()
      $0.bottom.equalToSuperview()
    }

    contentView.addSubview(closeButton)
    closeButton.snp.makeConstraints {
      $0.top.trailing.equalToSuperview()
      $0.size.equalTo(44)
    }

    enableButton.addTarget(self, action: #selector(enableVPNAction), for: .touchUpInside)
    closeButton.addTarget(self, action: #selector(closeView), for: .touchUpInside)
  }

  @available(*, unavailable)
  required init(coder: NSCoder) { fatalError() }

  @objc func enableVPNAction() {
    enableVPNTapped?()
  }

  @objc func closeView() {
    dismissHeaderTapped?()
  }
}
