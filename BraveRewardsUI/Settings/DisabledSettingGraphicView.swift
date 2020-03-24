// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit

class DisabledSettingGraphicView: UIStackView {

  init(image: UIImage, text: String) {
    super.init(frame: .zero)
    
    spacing = 15
    alignment = .center
    layoutMargins = UIEdgeInsets(top: 15, left: 15, bottom: 15, right: 15)
    isLayoutMarginsRelativeArrangement = true
    
    let imageView = UIImageView(image: image).then {
      $0.setContentHuggingPriority(.required, for: .horizontal)
      $0.setContentHuggingPriority(.required, for: .vertical)
    }
    
    let label = UILabel().then {
      $0.text = text
      $0.numberOfLines = 0
      $0.font = SettingsUX.bodyFont
      $0.appearanceTextColor = SettingsUX.bodyTextColor
    }
    
    addArrangedSubview(imageView)
    addArrangedSubview(label)
  }
  
  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
}
