// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveUI
import UIKit

class CustomEngineURLInputTableViewCell: UITableViewCell, TableViewReusable {

  // MARK: UX

  struct UX {
    static let textViewHeight: CGFloat = 88
    static let textViewInset: CGFloat = 16
  }

  // MARK: Properties

  var textview = UITextView(frame: .zero)

  weak var delegate: UITextViewDelegate? {
    didSet {
      textview.delegate = delegate
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
    textview = UITextView(
      frame: CGRect(x: 0, y: 0, width: contentView.frame.width, height: contentView.frame.height)
    ).then {
      $0.text = "https://"
      $0.backgroundColor = .clear
      $0.font = UIFont.preferredFont(forTextStyle: .body)
      $0.autocapitalizationType = .none
      $0.autocorrectionType = .no
      $0.spellCheckingType = .no
      $0.keyboardType = .URL
      $0.textColor = .braveLabel
    }

    contentView.addSubview(textview)

    textview.snp.makeConstraints({ make in
      make.leading.trailing.equalToSuperview().inset(UX.textViewInset)
      make.bottom.top.equalToSuperview()
      make.height.equalTo(UX.textViewHeight)
    })
  }
}
