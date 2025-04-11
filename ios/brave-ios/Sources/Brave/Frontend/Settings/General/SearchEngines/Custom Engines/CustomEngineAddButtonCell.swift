// Copyright 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveUI
import Strings
import UIKit

class CustomEngineAddButtonCell: UITableViewCell, TableViewReusable {

  // MARK: Properties

  var doneButton = UIButton()
  var spinner = UIActivityIndicatorView(style: .medium)

  // MARK: Lifecycle

  override init(style: UITableViewCell.CellStyle, reuseIdentifier: String?) {
    super.init(style: style, reuseIdentifier: reuseIdentifier)

    setup()
  }

  required init?(coder aDecoder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  // MARK: Internal

  private func setup() {
    doneButton.do {
      $0.setTitle(Strings.done, for: .normal)
      $0.setTitleColor(UIColor(braveSystemName: .schemesOnPrimary), for: .normal)
      $0.layer.cornerRadius = 12.0
      $0.backgroundColor = UIColor(braveSystemName: .buttonBackground)
      $0.titleLabel?.font = .preferredFont(for: .subheadline, weight: .semibold)
    }
    spinner.do {
      $0.color = UIColor(braveSystemName: .dividerInteractive)
      $0.isHidden = true
      $0.hidesWhenStopped = true
    }

    contentView.addSubview(doneButton)
    contentView.addSubview(spinner)

    doneButton.snp.makeConstraints {
      $0.edges.equalToSuperview()
    }
    spinner.snp.makeConstraints {
      $0.center.equalToSuperview()
    }
  }

  // MARK: Public

  func updateButtonStatus(_ status: CustomEngineViewController.DoneButtonStatus) {
    switch status {
    case .enabled:
      doneButton.setTitle(Strings.done, for: .normal)
      doneButton.backgroundColor = UIColor(braveSystemName: .buttonBackground)
      doneButton.isEnabled = true
      spinner.stopAnimating()
    case .disabled:
      doneButton.setTitle(Strings.done, for: .normal)
      doneButton.backgroundColor = UIColor(braveSystemName: .buttonDisabled)
      doneButton.isEnabled = false
      spinner.stopAnimating()
    case .loading:
      doneButton.setTitle(nil, for: .normal)
      doneButton.backgroundColor = UIColor(braveSystemName: .buttonBackground)
      spinner.isHidden = false
      spinner.startAnimating()
    }
  }
}
