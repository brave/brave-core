/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import Shared

@objc protocol FavoriteCellDelegate {
    func editFavorite(_ favoriteCell: FavoriteCell)
}

class FavoriteCell: UICollectionViewCell {
    static let imageAspectRatio: Float = 1.0
    static let placeholderImage = #imageLiteral(resourceName: "defaultTopSiteIcon")
    static let identifier = "FavoriteCell"
    
    private struct UI {
        /// Ratio of width:height of the thumbnail image.
        static let borderColor = UX.GreyJ
        static let borderWidth: CGFloat = 0
        static let cornerRadius: CGFloat = 8
        
        static let labelColor = UIAccessibilityDarkerSystemColorsEnabled() ? UX.GreyJ : UX.GreyH
        static let labelAlignment: NSTextAlignment = .center
        static let labelInsets = UIEdgeInsetsMake(0, 3, 2, 3)
        
        static let editButtonAnimationDuration: TimeInterval = 0.4
        static let editButtonAnimationDamping: CGFloat = 0.6
    }
    
    weak var delegate: FavoriteCellDelegate?
    
    var imageInsets: UIEdgeInsets = UIEdgeInsets.zero
    var cellInsets: UIEdgeInsets = UIEdgeInsets.zero
    
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
                    image = imageWithSize(image, size: ContainerSize.scaledDown(), maxScale: maxScale)
                    imageView.contentMode = .center
                }
                else if image.size.width > 32 {
                    imageView.contentMode = .scaleAspectFit
                }
                imageView.image = image
            } else {
                imageView.image = FavoriteCell.placeholderImage
                imageView.contentMode = .center
            }
        }
    }
    
    private func imageWithSize(_ image: UIImage, size:CGSize, maxScale: CGFloat) -> UIImage {
        var scaledImageRect = CGRect.zero;
        var aspectWidth:CGFloat = size.width / image.size.width;
        var aspectHeight:CGFloat = size.height / image.size.height;
        if aspectWidth > maxScale || aspectHeight > maxScale {
            let m = max(maxScale / aspectWidth, maxScale / aspectHeight)
            aspectWidth *= m
            aspectHeight *= m
        }
        let aspectRatio:CGFloat = min(aspectWidth, aspectHeight)
        scaledImageRect.size.width = image.size.width * aspectRatio
        scaledImageRect.size.height = image.size.height * aspectRatio
        scaledImageRect.origin.x = (size.width - scaledImageRect.size.width) / 2.0
        scaledImageRect.origin.y = (size.height - scaledImageRect.size.height) / 2.0
        UIGraphicsBeginImageContextWithOptions(size, false, 0)
        image.draw(in: scaledImageRect)
        let scaledImage = UIGraphicsGetImageFromCurrentImageContext()
        UIGraphicsEndImageContext()
        return scaledImage!
    }
    
    lazy var textLabel = UILabel().then {
        $0.setContentHuggingPriority(UILayoutPriority.defaultHigh, for: UILayoutConstraintAxis.vertical)
        $0.font = DynamicFontHelper.defaultHelper.DefaultSmallFont
        $0.textColor = UI.labelColor
        $0.textAlignment = UI.labelAlignment
        $0.lineBreakMode = NSLineBreakMode.byWordWrapping
        $0.numberOfLines = 2
    }
    
    lazy var imageView = UIImageView().then {
        $0.contentMode = .scaleAspectFit
        $0.clipsToBounds = true
        $0.layer.cornerRadius = UI.cornerRadius
        $0.layer.borderColor = UI.borderColor.cgColor
        $0.layer.borderWidth = UI.borderWidth
        $0.layer.minificationFilter = kCAFilterTrilinear
        $0.layer.magnificationFilter = kCAFilterNearest
    }
    
    lazy var editButton = UIButton().then {
        $0.isExclusiveTouch = true
        let removeButtonImage = UIImage(named: "edit-small")?.withRenderingMode(.alwaysTemplate)
        $0.setImage(removeButtonImage, for: .normal)
        $0.addTarget(self, action: #selector(FavoriteCell.editButtonTapped), for: UIControlEvents.touchUpInside)
        $0.accessibilityLabel = Strings.Edit_Bookmark
        $0.isHidden = true
        $0.backgroundColor = UX.GreyC
        $0.tintColor = UX.GreyI
        $0.frame.size = CGSize(width: 28, height: 28)
        let xOffset: CGFloat = 5
        let buttonCenterX = floor($0.bounds.width/2) + xOffset
        let buttonCenterY = floor($0.bounds.height/2)
        $0.center = CGPoint(x: buttonCenterX, y: buttonCenterY)
        $0.layer.cornerRadius = $0.bounds.width/2
        $0.layer.masksToBounds = true
    }
    
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
            make.left.right.equalTo(self.contentView).inset(UI.labelInsets)
            make.top.equalTo(imageView.snp.bottom).offset(5)
        }
        
        imageView.snp.remakeConstraints { make in
            make.top.equalTo(self.contentView).inset(8)
            make.right.left.equalTo(self.contentView).inset(16)
            make.height.equalTo(imageView.snp.width)
        }
        
        // Prevents the textLabel from getting squished in relation to other view priorities.
        textLabel.setContentCompressionResistancePriority(UILayoutPriority.defaultHigh, for: UILayoutConstraintAxis.vertical)
        
        NotificationCenter.default.addObserver(self, selector: #selector(showEditMode), name: Notification.Name.ThumbnailEditOn, object: nil)
        NotificationCenter.default.addObserver(self, selector: #selector(hideEditMode), name: Notification.Name.ThumbnailEditOff, object: nil)
    }
    
    deinit {
        NotificationCenter.default.removeObserver(self, name: Notification.Name.ThumbnailEditOn, object: nil)
        NotificationCenter.default.removeObserver(self, name: Notification.Name.ThumbnailEditOff, object: nil)
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
    
    private func updateSelectedHighlightedState() {
        let activated = isSelected || isHighlighted
        self.imageView.alpha = activated ? 0.7 : 1.0
    }
    
    @objc func editButtonTapped() {
        delegate?.editFavorite(self)
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
        UIView.animate(withDuration: UI.editButtonAnimationDuration,
                       delay: 0,
                       usingSpringWithDamping: UI.editButtonAnimationDamping,
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
        imageView.layer.borderWidth = show ? UI.borderWidth : 0
    }
}
