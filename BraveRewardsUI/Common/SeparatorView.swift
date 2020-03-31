/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import BraveUI

/// A simple view that has constant height and defaults to a gray color
class SeparatorView: UIView {

  override init(frame: CGRect) {
    super.init(frame: frame)
    backgroundColor = Colors.neutral300
    snp.makeConstraints {
      $0.height.equalTo(1.0 / UIScreen.main.scale)
    }
  }
  
  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
}
