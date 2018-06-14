/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import Shared

struct ThumbnailCellUX {
    /// Ratio of width:height of the thumbnail image.
    static let ImageAspectRatio: Float = 1.0
    static let BorderColor = UX.GreyJ
    static let BorderWidth: CGFloat = 0
    static let LabelColor = UIAccessibilityDarkerSystemColorsEnabled() ? UX.GreyJ : UX.GreyH
    static let LabelAlignment: NSTextAlignment = .center
    
    static let LabelInsets = UIEdgeInsetsMake(0, 3, 2, 3)
    static let PlaceholderImage = UIImage(named: "defaultTopSiteIcon")
    static let CornerRadius: CGFloat = 8
    
    static let EditButtonAnimationDuration: TimeInterval = 0.4
    static let EditButtonAnimationDamping: CGFloat = 0.6
}

@objc protocol ThumbnailCellDelegate {
    func editThumbnail(_ thumbnailCell: ThumbnailCell)
}

class ThumbnailCell: UICollectionViewCell {
    weak var delegate: ThumbnailCellDelegate?
    
    var imageInsets: UIEdgeInsets = UIEdgeInsets.zero
    var cellInsets: UIEdgeInsets = UIEdgeInsets.zero
    
    static func imageWithSize(_ image: UIImage, size:CGSize, maxScale: CGFloat) -> UIImage {
        var scaledImageRect = CGRect.zero;
        var aspectWidth:CGFloat = size.width / image.size.width;
        var aspectHeight:CGFloat = size.height / image.size.height;
        if aspectWidth > maxScale || aspectHeight > maxScale {
            let m = max(maxScale / aspectWidth, maxScale / aspectHeight)
            aspectWidth *= m
            aspectHeight *= m
        }
        let aspectRatio:CGFloat = min(aspectWidth, aspectHeight);
        scaledImageRect.size.width = image.size.width * aspectRatio;
        scaledImageRect.size.height = image.size.height * aspectRatio;
        scaledImageRect.origin.x = (size.width - scaledImageRect.size.width) / 2.0;
        scaledImageRect.origin.y = (size.height - scaledImageRect.size.height) / 2.0;
        UIGraphicsBeginImageContextWithOptions(size, false, 0);
        image.draw(in: scaledImageRect);
        let scaledImage = UIGraphicsGetImageFromCurrentImageContext();
        UIGraphicsEndImageContext();
        return scaledImage!;
    }
    
    var image: UIImage? = nil {
        didSet {
            struct ContainerSize {
                static var size: CGSize = CGSize.zero
                static func scaledDown() -> CGSize {
                    return CGSize(width: size.width * 0.75, height: size.height * 0.75)
                }
            }
            
            if imageView.frame.size.width > 0 {
                ContainerSize.size = imageView.frame.size
            }
            
            if var image = image {
                if image.size.width <= 32 && ContainerSize.size != CGSize.zero {
                    var maxScale = CGFloat(image.size.width < 24 ? 3.0 : 1.5)
                    if ContainerSize.size.width > 170 {
                        // we are on iPad pro. Fragile, but no other way to detect this on simulator.
                        maxScale *= 2.0
                    }
                    image = ThumbnailCell.imageWithSize(image, size: ContainerSize.scaledDown(), maxScale: maxScale)
                    imageView.contentMode = .center
                }
                else if image.size.width > 32 {
                    imageView.contentMode = .scaleAspectFit
                }
                imageView.image = image
            } else {
                imageView.image = ThumbnailCellUX.PlaceholderImage
                imageView.contentMode = .center
            }
        }
    }
    
    lazy var textLabel: UILabel = {
        let textLabel = UILabel()
        textLabel.setContentHuggingPriority(UILayoutPriority.defaultHigh, for: UILayoutConstraintAxis.vertical)
        textLabel.font = DynamicFontHelper.defaultHelper.DefaultSmallFont
        textLabel.textColor = ThumbnailCellUX.LabelColor
        textLabel.textAlignment = ThumbnailCellUX.LabelAlignment
        textLabel.lineBreakMode = NSLineBreakMode.byWordWrapping
        textLabel.numberOfLines = 2
        return textLabel
    }()
    
    lazy var imageView: UIImageView = {
        let imageView = UIImageView()
        imageView.contentMode = .scaleAspectFit
        imageView.clipsToBounds = true
        imageView.layer.cornerRadius = ThumbnailCellUX.CornerRadius
        imageView.layer.borderColor = ThumbnailCellUX.BorderColor.cgColor
        imageView.layer.borderWidth = ThumbnailCellUX.BorderWidth
        imageView.layer.minificationFilter = kCAFilterTrilinear
        imageView.layer.magnificationFilter = kCAFilterNearest
        return imageView
    }()
    
    lazy var editButton: UIButton = {
        let editButton = UIButton()
        editButton.isExclusiveTouch = true
        let removeButtonImage = UIImage(named: "edit-small")?.withRenderingMode(.alwaysTemplate)
        editButton.setImage(removeButtonImage, for: .normal)
        editButton.addTarget(self, action: #selector(ThumbnailCell.editButtonTapped), for: UIControlEvents.touchUpInside)
        editButton.accessibilityLabel = Strings.Edit_Bookmark
        editButton.isHidden = true
        editButton.backgroundColor = UX.GreyC
        editButton.tintColor = UX.GreyI
        editButton.frame.size = CGSize(width: 28, height: 28)
        let xOffset: CGFloat = 5
        let buttonCenterX = floor(editButton.bounds.width/2) + xOffset
        let buttonCenterY = floor(editButton.bounds.height/2)
        editButton.center = CGPoint(x: buttonCenterX, y: buttonCenterY)
        editButton.layer.cornerRadius = editButton.bounds.width/2
        editButton.layer.masksToBounds = true
        return editButton
    }()
    
    override var isSelected: Bool {
        didSet { updateSelectedHighlightedState() }
    }
    
    override var isHighlighted: Bool {
        didSet { updateSelectedHighlightedState() }
    }
    
    override init(frame: CGRect) {
        super.init(frame: frame)
        
        layer.shouldRasterize = true
        layer.rasterizationScale = UIScreen.main.scale
        
        isAccessibilityElement = true
        
        contentView.addSubview(imageView)
        contentView.addSubview(textLabel)
        contentView.addSubview(editButton)
        
        textLabel.snp.remakeConstraints { make in
            // TODO: relook at insets
            make.left.right.equalTo(self.contentView).inset(ThumbnailCellUX.LabelInsets)
            make.top.equalTo(imageView.snp.bottom).offset(5)
        }
        
        imageView.snp.remakeConstraints { make in
            make.top.equalTo(self.contentView).inset(8)
            make.right.left.equalTo(self.contentView).inset(16)
            make.height.equalTo(imageView.snp.width)
        }
        
        // Prevents the textLabel from getting squished in relation to other view priorities.
        textLabel.setContentCompressionResistancePriority(UILayoutPriority.defaultHigh, for: UILayoutConstraintAxis.vertical)
        
        NotificationCenter.default.addObserver(self, selector: #selector(showEditMode), name: Notification.Name.NotificationThumbnailEditOn,
                                               object: nil)
        NotificationCenter.default.addObserver(self, selector: #selector(hideEditMode), name: Notification.Name.NotificationThumbnailEditOff,
                                               object: nil)
    }
    
    deinit {
        NotificationCenter.default.removeObserver(self, name: Notification.Name.NotificationThumbnailEditOn, object: nil)
        NotificationCenter.default.removeObserver(self, name: Notification.Name.NotificationThumbnailEditOff, object: nil)
    }
    
    @objc func showEditMode() {
        toggleEditButton(true)
    }
    
    @objc func hideEditMode() {
        toggleEditButton(false)
    }
    
    required init?(coder aDecoder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }
    
    override func prepareForReuse() {
        super.prepareForReuse()
        editButton.isHidden = true
        showBorder(false)
        backgroundColor = UIColor.clear
        textLabel.font = DynamicFontHelper.defaultHelper.DefaultSmallFont
        textLabel.textColor = UIApplication.isInPrivateMode ? UIColor(rgb: 0xDBDBDB) : UIColor(rgb: 0x2D2D2D)
        imageView.backgroundColor = UIColor.clear
        imageView.image = nil
    }
    
    fileprivate func updateSelectedHighlightedState() {
        let activated = isSelected || isHighlighted
        self.imageView.alpha = activated ? 0.7 : 1.0
    }
    
    @objc func editButtonTapped() {
        delegate?.editThumbnail(self)
    }
    
    func toggleEditButton(_ show: Bool) {
        // Only toggle if we change state
        if editButton.isHidden != show {
            return
        }
        
        if show {
            editButton.isHidden = false
        }
        
        let scaleTransform = CGAffineTransform(scaleX: 0.01, y: 0.01)
        editButton.transform = show ? scaleTransform : CGAffineTransform.identity
        UIView.animate(withDuration: ThumbnailCellUX.EditButtonAnimationDuration,
                       delay: 0,
                       usingSpringWithDamping: ThumbnailCellUX.EditButtonAnimationDamping,
                       initialSpringVelocity: 0,
                       options: UIViewAnimationOptions.allowUserInteraction,
                       animations: {
                        self.editButton.transform = show ? CGAffineTransform.identity : scaleTransform
        }, completion: { _ in
            if !show {
                self.editButton.isHidden = true
            }
        })
    }
    
    func showBorder(_ show: Bool) {
        imageView.layer.borderWidth = show ? ThumbnailCellUX.BorderWidth : 0
    }
}
