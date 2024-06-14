// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveShared
import BraveUI
import Shared
import UIKit

extension BraveVPNInstallViewController {
  class View: UIView {

    private let mainStackView = UIStackView().then {
      $0.axis = .vertical
      $0.translatesAutoresizingMaskIntoConstraints = false
      $0.spacing = 30
    }

    private let imageView = UIView().then {
      $0.backgroundColor = BraveVPNCommonUI.UX.purpleBackgroundColor

      let image = UIImageView(
        image: UIImage(named: "install_vpn_image", in: .module, compatibleWith: nil)!
      ).then { img in
        img.contentMode = .scaleAspectFill
        img.setContentHuggingPriority(.required, for: .vertical)
        img.setContentCompressionResistancePriority(UILayoutPriority(1), for: .vertical)
      }

      $0.snp.makeConstraints { make in
        make.height.greaterThanOrEqualTo(250)
      }

      $0.addSubview(image)
      image.snp.makeConstraints { make in
        make.edges.equalToSuperview()
      }

      image.clipsToBounds = true

      $0.setContentHuggingPriority(.required, for: .vertical)
      $0.setContentCompressionResistancePriority(UILayoutPriority(1), for: .vertical)
    }

    private lazy var infoStackView = UIStackView().then {

      let contentStackView = UIStackView().then { stackView in
        stackView.axis = .vertical
        stackView.distribution = .equalSpacing
        stackView.spacing = 16
      }

      let textStackView = UIStackView().then { stackView in
        stackView.axis = .vertical
        stackView.spacing = 8

        let titleLabel = BraveVPNCommonUI.Views.ShrinkableLabel().then { label in
          label.text = Strings.VPN.installProfileTitle
          label.textAlignment = .center
          label.font = .systemFont(ofSize: 20, weight: .medium)
          label.textColor = .black
          label.minimumScaleFactor = 0.5
          label.adjustsFontSizeToFitWidth = true
        }

        let bodyLabel = BraveVPNCommonUI.Views.ShrinkableLabel().then { label in
          label.text = Strings.VPN.installProfileBody
          label.numberOfLines = 0
          label.font = .systemFont(ofSize: 18, weight: .medium)
          label.textColor = #colorLiteral(
            red: 0.4745098039,
            green: 0.4745098039,
            blue: 0.4745098039,
            alpha: 1
          )
          label.setContentCompressionResistancePriority(.defaultLow, for: .vertical)
        }

        [titleLabel, bodyLabel].forEach(stackView.addArrangedSubview(_:))
      }

      let buttonsStackView = UIStackView().then {
        $0.axis = .vertical
        $0.spacing = 4
        $0.addStackViewItems(
          .view(installVPNButton),
          .view(contactSupportButton)
        )
      }

      [textStackView, buttonsStackView].forEach(contentStackView.addArrangedSubview(_:))

      [
        UIView.spacer(.horizontal, amount: 30),
        contentStackView,
        UIView.spacer(.horizontal, amount: 30),
      ]
      .forEach($0.addArrangedSubview(_:))
    }

    let installVPNButton = BraveButton().then {
      $0.setTitle(Strings.VPN.installProfileButtonText, for: .normal)
      $0.backgroundColor = .braveDarkerBlurple
      $0.titleLabel?.font = .systemFont(ofSize: 16, weight: .semibold)
      $0.setTitleColor(.white, for: .normal)
      $0.snp.makeConstraints { make in
        make.height.equalTo(44)
      }
      $0.contentEdgeInsets = UIEdgeInsets(top: 0, left: 25, bottom: 0, right: 25)
      $0.layer.cornerRadius = 22
      $0.layer.cornerCurve = .continuous

      $0.loaderView = LoaderView(size: .small)
    }

    let contactSupportButton = BraveButton(type: .system).then {
      $0.setTitle(Strings.VPN.settingsContactSupport, for: .normal)
      $0.titleLabel?.font = .systemFont(ofSize: 16)
      $0.setTitleColor(.secondaryBraveLabel, for: .normal)
      $0.snp.makeConstraints { make in
        make.height.equalTo(44)
      }
      $0.contentEdgeInsets = UIEdgeInsets(top: 0, left: 25, bottom: 0, right: 25)
      $0.layer.cornerRadius = 22
      $0.layer.cornerCurve = .continuous
      $0.loaderView = LoaderView(size: .small)
    }

    override init(frame: CGRect) {
      super.init(frame: frame)
      backgroundColor = .white
      addSubview(mainStackView)

      [imageView, infoStackView].forEach(mainStackView.addArrangedSubview(_:))

      mainStackView.snp.makeConstraints {
        $0.top.leading.trailing.equalTo(self.safeAreaLayoutGuide)
        $0.bottom.equalTo(self.safeAreaLayoutGuide).inset(30)
      }
    }

    @available(*, unavailable)
    required init(coder: NSCoder) { fatalError() }
  }
}
