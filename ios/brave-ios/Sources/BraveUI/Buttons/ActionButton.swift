/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit

open class ActionButton: BraveButton {

  override public init(frame: CGRect) {
    super.init(frame: frame)

    backgroundColor = .clear
    layer.borderWidth = 1.0
    layer.cornerCurve = .continuous
    tintColor = .white
  }

  @available(*, unavailable)
  required public init(coder: NSCoder) {
    fatalError()
  }

  override open func layoutSubviews() {
    super.layoutSubviews()

    layer.cornerRadius = bounds.height / 2.0
  }

  override open var tintColor: UIColor! {
    didSet {
      setTitleColor(tintColor, for: .normal)
      layer.borderColor = tintColor.withAlphaComponent(0.5).cgColor
    }
  }
}

open class FilledActionButton: BraveButton {
  override public init(frame: CGRect) {
    super.init(frame: frame)

    tintColor = .white
  }

  @available(*, unavailable)
  required public init(coder: NSCoder) {
    fatalError()
  }

  override open func layoutSubviews() {
    super.layoutSubviews()

    layer.cornerRadius = bounds.height / 2.0
  }
}
