// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Shared
import UIKit

protocol BookmarkFormFieldsProtocol: UIView {
  var titleTextField: UITextField { get }
  var urlTextField: UITextField? { get }

  var delegate: BookmarkDetailsViewDelegate? { get set }

  func validateFields() -> Bool
}

extension BookmarkFormFieldsProtocol {
  var urlTextField: UITextField? { return nil }

  func validateFields() -> Bool {
    return BookmarkValidation.validateBookmark(title: titleTextField.text, url: urlTextField?.text)
  }
}
