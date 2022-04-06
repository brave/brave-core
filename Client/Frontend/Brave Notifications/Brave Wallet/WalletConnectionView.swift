// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import UIKit
import Shared
import BraveUI

class WalletConnectionView: UIControl {
  private let stackView: UIStackView = {
    let result = UIStackView()
    result.axis = .horizontal
    result.spacing = 15
    result.alignment = .center
    result.isUserInteractionEnabled = false
    return result
  }()

  private let iconImageView: UIImageView = {
    let result = UIImageView(image: UIImage(imageLiteralResourceName: "brave.unlock"))
    result.tintColor = .white
    return result
  }()

  private let titleLabel: UILabel = {
    let result = UILabel()
    result.textColor = .white
    result.font = .preferredFont(forTextStyle: .subheadline, weight: .semibold)
    result.adjustsFontForContentSizeCategory = true
    result.numberOfLines = 0
    result.text = Strings.Wallet.dappsConnectionNotificationTitle
    return result
  }()

  override init(frame: CGRect) {
    super.init(frame: frame)

    addSubview(stackView)
    stackView.snp.makeConstraints {
      $0.edges.equalToSuperview().inset(20)
    }
    stackView.addArrangedSubview(iconImageView)
    stackView.addArrangedSubview(titleLabel)

    layer.backgroundColor = UIColor.braveBlurpleTint.cgColor
    layer.cornerRadius = 10
  }
  @available(*, unavailable)
  required init(coder: NSCoder) {
      fatalError()
  }
}
