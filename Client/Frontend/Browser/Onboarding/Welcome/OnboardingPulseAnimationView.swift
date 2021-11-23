// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveUI

class RadialPulsingAnimation: UIView {
    private var pulseLayers = [CAShapeLayer]()
    
    init(ringCount: Int) {
        super.init(frame: .zero)
        isUserInteractionEnabled = false
        
        // [1, 4] => {Y ∈ ℝ: 1 <= Y <= 4} where Y = Thicc, X = amount of rings
        let idealThicc = 1.5
        
        for i in 0..<ringCount {
            let width = cos(20.0 * Double(i)) - sin(2.0 * Double(i)) + idealThicc
            
            let layer = CAShapeLayer()
            layer.strokeColor = UIColor.white.cgColor
            layer.lineWidth = width
            layer.fillColor = UIColor.clear.cgColor
            layer.lineCap = .round
            pulseLayers.append(layer)
        }
    }
    
    required init?(coder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }
    
    override func layoutSubviews() {
        super.layoutSubviews()
        
        for i in 0..<pulseLayers.count {
            let inset = 7.0 * sin(4.0 * Double(i)) - cos(2.0 * Double(i)) + 1
            
            let frame = bounds.insetBy(dx: inset, dy: inset)
            let path = UIBezierPath(roundedRect: frame, cornerRadius: min(frame.width, frame.height) / 2.0)
            
            let pulseLayer = pulseLayers[i]
            pulseLayer.frame = bounds
            pulseLayer.path = path.cgPath
            layer.addSublayer(pulseLayer)
        }
    }
    
    func animate() {
        let isBreathing = false
        
        if isBreathing {
            let animation = CABasicAnimation(keyPath: "transform.scale")
            animation.toValue = 1.2
            animation.duration = 1.0
            animation.timingFunction = CAMediaTimingFunction(name: .easeInEaseOut)
            animation.autoreverses = true
            animation.repeatCount = .infinity
            
            for i in 0..<pulseLayers.count {
                self.pulseLayers[i].add(animation, forKey: "pulse")
            }
        } else {
            let animation = CABasicAnimation(keyPath: "transform.scale")
            animation.toValue = 1.2
            animation.duration = 1.0
            animation.timingFunction = CAMediaTimingFunction(name: .easeInEaseOut)
            animation.autoreverses = false
            animation.repeatCount = .infinity
            
            let fadeAnimation = CABasicAnimation(keyPath: "opacity")
            fadeAnimation.toValue = 0.0
            fadeAnimation.duration = 1.0
            fadeAnimation.timingFunction = CAMediaTimingFunction(name: .easeInEaseOut)
            fadeAnimation.autoreverses = false
            fadeAnimation.repeatCount = .infinity
            
            for i in 0..<pulseLayers.count {
                DispatchQueue.main.asyncAfter(deadline: .now() + TimeInterval(i * i)) {
                    self.pulseLayers[i].add(animation, forKey: "pulse")
                    self.pulseLayers[i].add(fadeAnimation, forKey: "fade")
                }
            }
        }
    }
    
    func present(icon: UIImage?, from view: UIView, on popoverController: PopoverController, browser: BrowserViewController) {
        let origin = browser.view.convert(view.center, from: view.superview)
        popoverController.view.insertSubview(self, aboveSubview: popoverController.backgroundOverlayView)
        
        if let icon = icon {
            let imageView = UIImageView().then {
                $0.image = icon
                $0.contentMode = .scaleAspectFit
                $0.isUserInteractionEnabled = false
            }
            
            addSubview(imageView)
            imageView.snp.makeConstraints {
                $0.center.equalToSuperview()
                $0.width.equalTo(view.bounds.size.width)
                $0.height.equalTo(view.bounds.size.height)
            }
        }
        
        frame = view.frame.insetBy(dx: -20.0, dy: -20.0)
        center = origin
        DispatchQueue.main.asyncAfter(deadline: .now() + 1.0) {
            self.animate()
        }
    }
}
