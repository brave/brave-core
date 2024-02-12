// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import Shared

class FolderDetailsViewTableViewCell: AddEditHeaderView, BookmarkFormFieldsProtocol {

  // MARK: BookmarkFormFieldsProtocol

  weak var delegate: BookmarkDetailsViewDelegate?

  let titleTextField = UITextField().then {
    $0.placeholder = Strings.bookmarkTitlePlaceholderText
    $0.clearButtonMode = .whileEditing
    $0.translatesAutoresizingMaskIntoConstraints = false

    let spacerView = UIView(frame: CGRect(x: 0, y: 0, width: 10, height: 16))
    $0.leftViewMode = .always
    $0.leftView = spacerView
  }

  convenience init(title: String?, viewHeight: CGFloat) {
    self.init(frame: .zero)

    backgroundColor = .secondaryBraveGroupedBackground

    [UIView.separatorLine, titleTextField, UIView.separatorLine]
      .forEach(mainStackView.addArrangedSubview)

    titleTextField.text = title ?? Strings.newFolderDefaultName

    mainStackView.snp.remakeConstraints {
      $0.edges.equalTo(self)
      $0.height.equalTo(viewHeight)
    }

    titleTextField.addTarget(self, action: #selector(textFieldDidChange(_:)), for: .editingChanged)
  }

  @objc func textFieldDidChange(_ textField: UITextField) {
    delegate?.correctValues(validationPassed: validateFields())
  }
}
