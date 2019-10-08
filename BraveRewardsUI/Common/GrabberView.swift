/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit

final class GrabberView: UIView {
  
  private struct UX {
    static let size = CGSize(width: 32.0, height: 4.0)
  }
  
  enum Style {
    case light
    case dark
    
    var backgroundColor: UIColor {
      switch self {
      case .dark:
        return UIColor(white: 0.0, alpha: 0.3)
      case .light:
        return UIColor(white: 1.0, alpha: 0.3)
      }
    }
  }
  
  init(style: Style) {
    super.init(frame: CGRect(origin: .zero, size: UX.size))
    
    clipsToBounds = true
    layer.cornerRadius = UX.size.height / 2.0
    backgroundColor = style.backgroundColor
  }
  
  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
  
  // MARK: -
  
  override func sizeToFit() {
    bounds = CGRect(origin: .zero, size: UX.size)
  }
  
  override var intrinsicContentSize: CGSize {
    return UX.size
  }
}
