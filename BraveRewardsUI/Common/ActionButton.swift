/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation

class ActionButton: Button {
  
  override init(frame: CGRect) {
    super.init(frame: frame)
    
    backgroundColor = .clear
    layer.borderWidth = 1.0
    tintColor = .white
  }
  
  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
  
  override func layoutSubviews() {
    super.layoutSubviews()
   
    layer.cornerRadius = bounds.height / 2.0
  }
  
  override var tintColor: UIColor! {
    didSet {
      appearanceTextColor = tintColor
      layer.borderColor = tintColor.withAlphaComponent(0.5).cgColor
    }
  }
}
