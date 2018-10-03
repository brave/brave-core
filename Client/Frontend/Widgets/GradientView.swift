// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit

/// A UIView whos layer is a gradient layer
class GradientView: UIView {
    
    convenience init() {
        self.init(colors: [], positions: [], startPoint: .zero, endPoint: CGPoint(x: 0, y: 1))
    }
    
    init(colors: [UIColor], positions: [CGFloat], startPoint: CGPoint, endPoint: CGPoint) {
        super.init(frame: .zero)
        
        gradientLayer.colors = colors.map { $0.cgColor }
        gradientLayer.locations = positions.map { NSNumber(value: Double($0)) }
        gradientLayer.startPoint = startPoint
        gradientLayer.endPoint = endPoint
    }
    
    @available(*, unavailable)
    required init?(coder aDecoder: NSCoder) {
        fatalError()
    }
    
    /// The gradient layer which you may modify
    var gradientLayer: CAGradientLayer {
        return layer as! CAGradientLayer // swiftlint:disable:this force_cast
    }
    
    override class var layerClass: AnyClass {
        return CAGradientLayer.self
    }
}
