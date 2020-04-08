// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import BraveUI

/// Simply a label with a wide amount of padding around it
class EmptyWalletSummaryView: UIView {
  
  override init(frame: CGRect) {
    super.init(frame: frame)
    
    let label = UILabel().then {
      $0.text = Strings.noActivitiesYet
      $0.textAlignment = .center
      $0.appearanceTextColor = Colors.grey500
      $0.font = .systemFont(ofSize: 20.0)
      $0.numberOfLines = 0
    }
    
    addSubview(label)
    
    label.snp.makeConstraints {
      $0.leading.trailing.equalTo(self).inset(20.0)
      $0.top.bottom.equalTo(self).inset(40.0)
    }
  }
  
  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
}
