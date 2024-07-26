// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveUI
import UIKit

class CustomEngineTitleInputTableViewCell: UITableViewCell, TableViewReusable {

  // MARK: UX

  struct UX {
    static let textFieldInset: CGFloat = 16
  }

  // MARK: Properties

  var textfield: UITextField = UITextField(frame: .zero)

  weak var delegate: UITextFieldDelegate? {
    didSet {
      textfield.delegate = delegate
    }
  }

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
    textfield = UITextField(
      frame: CGRect(x: 0, y: 0, width: contentView.frame.width, height: contentView.frame.height)
    )

    contentView.addSubview(textfield)

    textfield.snp.makeConstraints({ make in
      make.leading.trailing.equalToSuperview().inset(UX.textFieldInset)
      make.bottom.top.equalToSuperview().inset(UX.textFieldInset)
    })
  }
}
