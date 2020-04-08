// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveUI

/// A view with a label in it with a rounded colored background
class LabelAccessoryView: UIView {
  let label = UILabel().then {
    $0.appearanceTextColor = Colors.grey800
    $0.font = .systemFont(ofSize: 14.0, weight: .medium)
  }
  override init(frame: CGRect) {
    super.init(frame: frame)
    
    layer.cornerRadius = 6.0
    backgroundColor = Colors.grey000
    addSubview(label)
    label.snp.makeConstraints {
      $0.edges.equalTo(self).inset(UIEdgeInsets(top: 6, left: 8, bottom: 6, right: 8))
    }
  }
  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
}
