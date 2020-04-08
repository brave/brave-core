/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import BraveUI

class SettingsViewDetailsButton: Button {
  
  override init(frame: CGRect) {
    super.init(frame: frame)
    
    tintColor = Colors.purple600
    setTitle(Strings.settingsViewDetails, for: .normal)
    appearanceTextColor = Colors.purple600
    setImage(UIImage(frameworkResourceNamed: "right-arrow").alwaysTemplate, for: .normal)
    titleLabel?.font = .systemFont(ofSize: 14.0, weight: .medium)
    titleEdgeInsets = UIEdgeInsets(top: 0, left: 0, bottom: 0, right: 3)
    imageEdgeInsets = UIEdgeInsets(top: 0, left: 3, bottom: 0, right: 0)
    flipImageOrigin = true
    
    snp.makeConstraints { $0.height.equalTo(28.0) }
  }
  
  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
}
