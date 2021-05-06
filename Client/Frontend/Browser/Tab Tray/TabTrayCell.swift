// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveUI
import Shared

class TabCell: UICollectionViewCell {
    static let identifier = "TabCellIdentifier"
    static let borderWidth: CGFloat = 3
    
    let backgroundHolder = UIView()
    let screenshotView = UIImageViewAligned()
    let titleBackgroundView = GradientView(
        colors: [UIColor(white: 1.0, alpha: 0.98), UIColor(white: 1.0, alpha: 0.9), UIColor(white: 1.0, alpha: 0.0)],
        positions: [0, 0.5, 1],
        startPoint: .zero,
        endPoint: CGPoint(x: 0, y: 1)
    )
    let titleLabel: UILabel
    let favicon: UIImageView = UIImageView()
    let closeButton: UIButton
    
    var animator: SwipeAnimator!
    
    weak var delegate: TabCellDelegate?
    
    // Changes depending on whether we're full-screen or not.
    var margin = CGFloat(0)
    
    override init(frame: CGRect) {
        self.backgroundHolder.backgroundColor = .white
        self.backgroundHolder.layer.cornerRadius = TabTrayControllerUX.cornerRadius
        self.backgroundHolder.layer.cornerCurve = .continuous
        self.backgroundHolder.clipsToBounds = true
        
        self.screenshotView.contentMode = .scaleAspectFill
        self.screenshotView.clipsToBounds = true
        self.screenshotView.isUserInteractionEnabled = false
        self.screenshotView.alignLeft = true
        self.screenshotView.alignTop = true
        self.screenshotView.backgroundColor = .braveBackground
        
        self.favicon.backgroundColor = .clear
        self.favicon.layer.cornerRadius = 2.0
        self.favicon.layer.cornerCurve = .continuous
        self.favicon.layer.masksToBounds = true
        
        self.titleLabel = UILabel()
        self.titleLabel.isUserInteractionEnabled = false
        self.titleLabel.numberOfLines = 1
        self.titleLabel.font = DynamicFontHelper.defaultHelper.DefaultSmallFontBold
        self.titleLabel.textColor = .black
        self.titleLabel.backgroundColor = .clear
        
        self.closeButton = UIButton()
        self.closeButton.setImage(#imageLiteral(resourceName: "tab_close"), for: [])
        self.closeButton.imageView?.contentMode = .scaleAspectFit
        self.closeButton.contentMode = .center
        self.closeButton.imageEdgeInsets = UIEdgeInsets(equalInset: TabTrayControllerUX.closeButtonEdgeInset)
        
        super.init(frame: frame)
        
        self.animator = SwipeAnimator(animatingView: self)
        self.closeButton.addTarget(self, action: #selector(close), for: .touchUpInside)
        
        layer.borderWidth = TabTrayControllerUX.defaultBorderWidth
        layer.borderColor = UIColor.braveSeparator.resolvedColor(with: traitCollection).cgColor
        layer.cornerRadius = TabTrayControllerUX.cornerRadius
        layer.cornerCurve = .continuous
        
        contentView.addSubview(backgroundHolder)
        backgroundHolder.addSubview(self.screenshotView)
        backgroundHolder.addSubview(self.titleBackgroundView)
        
        titleBackgroundView.addSubview(self.closeButton)
        titleBackgroundView.addSubview(self.titleLabel)
        titleBackgroundView.addSubview(self.favicon)
        
        self.accessibilityCustomActions = [
            UIAccessibilityCustomAction(name: Strings.tabAccessibilityCloseActionLabel, target: self.animator, selector: #selector(SwipeAnimator.closeWithoutGesture))
        ]
    }
    
    func setTabSelected(_ tab: Tab) {
        layer.shadowColor = UIColor.braveInfoBorder.resolvedColor(with: traitCollection).cgColor
        layer.shadowOpacity = 1
        layer.shadowRadius = 0 // A 0 radius creates a solid border instead of a gradient blur
        layer.masksToBounds = false
        // create a frame that is "BorderWidth" size bigger than the cell
        layer.shadowOffset = CGSize(width: -TabCell.borderWidth, height: -TabCell.borderWidth)
        let shadowPath = CGRect(width: layer.frame.width + (TabCell.borderWidth * 2), height: layer.frame.height + (TabCell.borderWidth * 2))
        layer.shadowPath = UIBezierPath(roundedRect: shadowPath, cornerRadius: TabTrayControllerUX.cornerRadius+TabCell.borderWidth).cgPath
        layer.borderWidth = 0.0
    }
    
    required init?(coder aDecoder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }
    
    override func layoutSubviews() {
        super.layoutSubviews()
        
        backgroundHolder.frame = CGRect(x: margin, y: margin, width: frame.width, height: frame.height)
        screenshotView.frame = CGRect(size: backgroundHolder.frame.size)
        
        titleBackgroundView.snp.makeConstraints { make in
            make.top.left.right.equalTo(backgroundHolder)
            make.height.equalTo(TabTrayControllerUX.textBoxHeight + 15.0)
        }
        
        favicon.snp.makeConstraints { make in
            make.leading.equalTo(titleBackgroundView).offset(6)
            make.top.equalTo((TabTrayControllerUX.textBoxHeight - TabTrayControllerUX.faviconSize) / 2)
            make.size.equalTo(TabTrayControllerUX.faviconSize)
        }
        
        titleLabel.snp.makeConstraints { make in
            make.leading.equalTo(favicon.snp.trailing).offset(6)
            make.trailing.equalTo(closeButton.snp.leading).offset(-6)
            make.centerY.equalTo(favicon)
        }
        
        closeButton.snp.makeConstraints { make in
            make.size.equalTo(TabTrayControllerUX.closeButtonSize)
            make.trailing.equalTo(titleBackgroundView)
            make.centerY.equalTo(favicon)
        }
        
        let shadowPath = CGRect(width: layer.frame.width + (TabCell.borderWidth * 2), height: layer.frame.height + (TabCell.borderWidth * 2))
        layer.shadowPath = UIBezierPath(roundedRect: shadowPath, cornerRadius: TabTrayControllerUX.cornerRadius+TabCell.borderWidth).cgPath
    }
    
    override func prepareForReuse() {
        // Reset any close animations.
        backgroundHolder.transform = .identity
        backgroundHolder.alpha = 1
        titleLabel.font = DynamicFontHelper.defaultHelper.DefaultSmallFontBold
        layer.shadowOffset = .zero
        layer.shadowPath = nil
        layer.shadowOpacity = 0
        layer.borderWidth = TabTrayControllerUX.defaultBorderWidth
    }
    
    override func accessibilityScroll(_ direction: UIAccessibilityScrollDirection) -> Bool {
        var right: Bool
        switch direction {
        case .left:
            right = false
        case .right:
            right = true
        default:
            return false
        }
        animator.close(right: right)
        return true
    }
    
    @objc
    func close() {
        animator.closeWithoutGesture()
    }
    
    override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
        super.traitCollectionDidChange(previousTraitCollection)
        
        // cgcolor does not dynamically update
        traitCollection.performAsCurrent {
            layer.shadowColor = UIColor.braveInfoBorder.cgColor
            layer.borderColor = UIColor.braveSeparator.cgColor
        }
    }
}
