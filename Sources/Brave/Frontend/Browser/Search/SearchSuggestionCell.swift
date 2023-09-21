// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import Shared
import BraveUI

class SuggestionCell: UITableViewCell {
  static let identifier = "SuggestionCell"

  var openButtonActionHandler: (() -> Void)?

  private let stackView = UIStackView().then {
    $0.spacing = 20.0
    $0.isLayoutMarginsRelativeArrangement = true
    $0.layoutMargins = UIEdgeInsets(top: 0.0, left: 15.0, bottom: 0.0, right: 15.0)
  }

  private let titleLabel = UILabel().then {
    $0.font = .systemFont(ofSize: 15.0)
    $0.textColor = .bravePrimary
    $0.lineBreakMode = .byTruncatingMiddle
  }

  private let openButton = BraveButton().then {
    $0.setImage(UIImage(named: "recent-search-arrow", in: .module, compatibleWith: nil), for: .normal)
    $0.hitTestSlop = UIEdgeInsets(equalInset: -20)
    $0.setContentHuggingPriority(.required, for: .horizontal)
    $0.setContentCompressionResistancePriority(.required, for: .horizontal)
  }

  override init(style: UITableViewCell.CellStyle, reuseIdentifier: String?) {
    super.init(style: style, reuseIdentifier: reuseIdentifier)

    backgroundColor = .clear

    contentView.addSubview(stackView)
    stackView.addArrangedSubview(titleLabel)
    stackView.addArrangedSubview(openButton)

    stackView.snp.makeConstraints {
      $0.edges.equalToSuperview()
    }

    openButton.addTarget(self, action: #selector(onOpenButtonPressed), for: .touchUpInside)
  }

  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  func setTitle(_ title: String) {
    titleLabel.text = title
  }

  @objc
  private func onOpenButtonPressed() {
    openButtonActionHandler?()
  }
}
