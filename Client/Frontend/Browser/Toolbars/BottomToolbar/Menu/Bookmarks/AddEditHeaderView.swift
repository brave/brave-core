// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit

protocol BookmarkDetailsViewDelegate: AnyObject {
  func correctValues(validationPassed: Bool)
}

class AddEditHeaderView: UIView {

  struct UX {
    static let defaultSpacing: CGFloat = 8
    static let faviconSize: CGFloat = 64
  }

  let mainStackView = UIStackView().then {
    $0.axis = .vertical
    $0.spacing = UX.defaultSpacing
  }

  override init(frame: CGRect) {
    super.init(frame: frame)
    addSubview(mainStackView)

    mainStackView.snp.makeConstraints {
      $0.edges.equalTo(self)
    }
  }

  @available(*, unavailable)
  required init?(coder aDecoder: NSCoder) {
    fatalError()
  }
}
