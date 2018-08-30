/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import UIKit

extension PopoverController {
    
    struct PopoverUX {
        static let backgroundColor: UIColor = .white
        static let arrowSize = CGSize(width: 14.0, height: 8.0)
        static let cornerRadius: CGFloat = 10.0
        static let shadowOffset = CGSize(width: 0, height: 2.0)
        static let shadowRadius: CGFloat = 3.0
        static let shadowColor: UIColor = .black
        static let shadowOpacity: Float = 0.3
    }
    
    /// The direction the arrow faces
    enum ArrowDirection {
        /// The arrow faces upwards like: ▴
        case up
        /// The arrow faces downwards like: ▾
        case down
    }
    
    /// The internal view loaded by PopoverController. Applies default styling as well as sets up the arrow
    class ContainerView: UIView {
        
        /// The arrow direction for this view
        var arrowDirection: ArrowDirection = .up {
            didSet {
                updateTrianglePath()
                setNeedsLayout()
                setNeedsUpdateConstraints()
                updateConstraintsIfNeeded()
            }
        }
        
        /// Where to display the arrow on the popover
        var arrowOrigin = CGPoint.zero {
            didSet {
                setNeedsLayout()
                setNeedsUpdateConstraints()
                updateConstraintsIfNeeded()
            }
        }
        
        /// The view where you will place the content controller's view
        let contentView = UIView().then {
            $0.backgroundColor = PopoverUX.backgroundColor
            $0.layer.cornerRadius = PopoverUX.cornerRadius
            $0.clipsToBounds = true
        }
        
        /// The actual white background view with the arrow. We have two separate views to ensure content placed within
        /// the popover are clipped at the corners
        private let backgroundView = UIView().then {
            $0.backgroundColor = PopoverUX.backgroundColor
            $0.layer.cornerRadius = PopoverUX.cornerRadius
            $0.layer.shadowColor = PopoverUX.shadowColor.cgColor
            $0.layer.shadowOffset = PopoverUX.shadowOffset
            $0.layer.shadowRadius = PopoverUX.shadowRadius
            $0.layer.shadowOpacity = PopoverUX.shadowOpacity
            $0.translatesAutoresizingMaskIntoConstraints = false
        }
        
        private let triangleLayer = CAShapeLayer().then {
            $0.fillColor = PopoverUX.backgroundColor.cgColor
            $0.shadowColor = PopoverUX.shadowColor.cgColor
            $0.shadowOffset = PopoverUX.shadowOffset
            $0.shadowRadius = PopoverUX.shadowRadius
            $0.shadowOpacity = PopoverUX.shadowOpacity
        }
        
        override init(frame: CGRect) {
            super.init(frame: frame)
            
            backgroundColor = .clear
            
            addSubview(backgroundView)
            addSubview(contentView)
            layer.addSublayer(triangleLayer)
            
            updateTrianglePath()
            
            contentView.snp.makeConstraints { make in
                make.left.right.equalTo(self)
                make.top.bottom.equalTo(backgroundView)
            }
            
            let backgroundViewTopConstraint = backgroundView.topAnchor.constraint(equalTo: topAnchor)
            let backgroundViewBottomConstraint = backgroundView.bottomAnchor.constraint(equalTo: bottomAnchor)
            
            NSLayoutConstraint.activate([
                backgroundView.leftAnchor.constraint(equalTo: leftAnchor),
                backgroundView.rightAnchor.constraint(equalTo: rightAnchor),
                backgroundViewTopConstraint,
                backgroundViewBottomConstraint
            ])
            
            self.backgroundViewTopConstraint = backgroundViewTopConstraint
            self.backgroundViewBottomConstraint = backgroundViewBottomConstraint
            
            setNeedsUpdateConstraints()
        }
        
        @available(*, unavailable)
        required init?(coder aDecoder: NSCoder) {
            fatalError()
        }
        
        override func layoutSubviews() {
            super.layoutSubviews()
            
            // Assure the arrow will not be hanging off a corner
            // 1 is added or removed to the max values to assure the arrow will not peak off the corner radius
            // due to the arrow itself being offseted by 1 vertically below to ensure you don't see it separate
            // during rotation
            let clampedArrowXOrigin = min(max(arrowOrigin.x, PopoverUX.cornerRadius + 1), bounds.width - PopoverUX.cornerRadius - 1) - PopoverUX.arrowSize.width / 2.0
            
            CATransaction.setDisableActions(true)
            triangleLayer.position = CGPoint(x: clampedArrowXOrigin, y: arrowDirection == .down ? bounds.size.height - PopoverUX.arrowSize.height - 1.0 : 1.0)
            CATransaction.setDisableActions(false)
            
            backgroundView.layer.shadowPath = UIBezierPath(roundedRect: backgroundView.bounds, cornerRadius: PopoverUX.cornerRadius).cgPath
        }
        
        private var backgroundViewTopConstraint: NSLayoutConstraint?
        private var backgroundViewBottomConstraint: NSLayoutConstraint?
        
        override func updateConstraints() {
            super.updateConstraints()
            
            switch arrowDirection {
            case .down:
                backgroundViewTopConstraint?.constant = 0.0
                backgroundViewBottomConstraint?.constant = -PopoverUX.arrowSize.height
                
            case .up:
                backgroundViewTopConstraint?.constant = PopoverUX.arrowSize.height
                backgroundViewBottomConstraint?.constant = 0.0
            }
        }
        
        private func updateTrianglePath() {
            let arrowSize = PopoverUX.arrowSize
            
            // Also have to apply a mask to the triangle so that the shadow doesn't appear on top of the content
            let shadowMask = CALayer()
            shadowMask.backgroundColor = UIColor.black.cgColor
            
            let path = UIBezierPath()
            switch arrowDirection {
            case .up:
                path.move(to: CGPoint(x: arrowSize.width / 2.0, y: 0.0))
                path.addLine(to: CGPoint(x: arrowSize.width, y: arrowSize.height))
                path.addLine(to: CGPoint(x: 0, y: arrowSize.height))
                
                shadowMask.frame = CGRect(x: -PopoverUX.shadowRadius, y: -PopoverUX.shadowRadius + PopoverUX.shadowOffset.height, width: PopoverUX.arrowSize.width + (PopoverUX.shadowRadius * 2.0), height: PopoverUX.arrowSize.height + 1.0)
            case .down:
                path.move(to: CGPoint(x: arrowSize.width / 2.0, y: arrowSize.height))
                path.addLine(to: CGPoint(x: arrowSize.width, y: 0.0))
                path.addLine(to: CGPoint(x: 0, y: 0.0))
                
                shadowMask.frame = CGRect(x: -PopoverUX.shadowRadius, y: 0, width: PopoverUX.arrowSize.width + (PopoverUX.shadowRadius * 2.0), height: PopoverUX.arrowSize.height + PopoverUX.shadowRadius + PopoverUX.shadowOffset.height)
            }
            path.close()
            
            triangleLayer.path = path.cgPath
            triangleLayer.mask = shadowMask
        }
    }
}
