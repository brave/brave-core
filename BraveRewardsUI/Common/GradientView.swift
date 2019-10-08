/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit

extension UIColor {
  convenience init(_ r: UInt8, _ g: UInt8, _ b: UInt8) {
    self.init(red: CGFloat(r) / 255.0, green: CGFloat(g) / 255.0, blue: CGFloat(b) / 255.0, alpha: 1.0)
  }
}

class GradientView: UIView {
  
  var gradientLayer: CAGradientLayer {
    return layer as! CAGradientLayer // swiftlint:disable:this force_cast
  }
  
  override class var layerClass: AnyClass {
    return CAGradientLayer.self
  }
}

extension GradientView {
  
  class func purpleRewardsGradientView() -> GradientView {
    let view = GradientView()
    view.gradientLayer.colors = [ UIColor(57, 45, 209).cgColor,
                                  UIColor(255, 26, 26).cgColor ]
    view.gradientLayer.startPoint = CGPoint(x: 0, y: 0.4)
    view.gradientLayer.endPoint = CGPoint(x: 1.0, y: 2.5)
    return view
  }
  
  class func softBlueToClearGradientView() -> GradientView {
    let view = GradientView()
    view.gradientLayer.colors = [ UIColor(white: 1.0, alpha: 0.0).cgColor,
                                  UIColor(white: 1.0, alpha: 0.0).cgColor,
                                  UIColor(233, 235, 255).cgColor ]
    view.gradientLayer.locations = [ 0.0, 0.8, 1.0 ]
    return view
  }
}
