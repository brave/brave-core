/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import Shared
import BraveShared

@objc protocol FavoriteCellDelegate {
    func editFavorite(_ favoriteCell: FavoriteCell)
}

class FavoriteCell: UICollectionViewCell {
    static let imageAspectRatio: Float = 1.0
    static let placeholderImage = #imageLiteral(resourceName: "defaultTopSiteIcon")
    static let identifier = "FavoriteCell"
    
    private struct UI {
        /// Ratio of width:height of the thumbnail image.
        static let cornerRadius: CGFloat = 8
        
        static let labelColor = UIAccessibility.isDarkerSystemColorsEnabled ? UX.GreyJ : UX.GreyH
        static let labelAlignment: NSTextAlignment = .center
        static let labelInsets = UIEdgeInsets(top: 0, left: 3, bottom: 2, right: 3)
        
        static let editButtonAnimationDuration: TimeInterval = 0.4
        static let editButtonAnimationDamping: CGFloat = 0.6
    }
    
    weak var delegate: FavoriteCellDelegate?
    
    var imageInsets: UIEdgeInsets = UIEdgeInsets.zero
    var cellInsets: UIEdgeInsets = UIEdgeInsets.zero
    
    let textLabel = UILabel().then {
        $0.setContentHuggingPriority(UILayoutPriority.defaultHigh, for: NSLayoutConstraint.Axis.vertical)
        $0.font = DynamicFontHelper.defaultHelper.DefaultSmallFont
        $0.textColor = UI.labelColor
        $0.textAlignment = UI.labelAlignment
        $0.lineBreakMode = NSLineBreakMode.byWordWrapping
        $0.numberOfLines = 2
    }
    
    let imageView = UIImageView().then {
        $0.contentMode = .scaleAspectFit
        $0.clipsToBounds = true
        $0.layer.cornerRadius = UI.cornerRadius
        $0.layer.borderColor = BraveUX.faviconBorderColor.cgColor
        $0.layer.borderWidth = BraveUX.faviconBorderWidth
        $0.layer.minificationFilter = CALayerContentsFilter.trilinear
        $0.layer.magnificationFilter = CALayerContentsFilter.nearest
    }
    
    let editButton = UIButton().then {
        $0.isExclusiveTouch = true
        let removeButtonImage = #imageLiteral(resourceName: "edit-small").template
        $0.setImage(removeButtonImage, for: .normal)
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
        textLabel.setContentCompressionResistancePriority(UILayoutPriority.defaultHigh, for: NSLayoutConstraint.Axis.vertical)
        
        editButton.addTarget(self, action: #selector(editButtonTapped), for: .touchUpInside)
        
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
        backgroundColor = UIColor.clear
        textLabel.font = DynamicFontHelper.defaultHelper.DefaultSmallFont
        textLabel.textColor = 
            PrivateBrowsingManager.shared.isPrivateBrowsing ? UX.Favorites.cellLabelColorPrivate : UX.Favorites.cellLabelColorNormal
        imageView.backgroundColor = UIColor.clear
        imageView.image = nil
    }
    
    private func updateSelectedHighlightedState() {
        let activatedAlpha: CGFloat = 0.7
        let disactivatedAlpha: CGFloat = 1.0
        
        let activated = isSelected || isHighlighted
        self.imageView.alpha = activated ? activatedAlpha : disactivatedAlpha
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
                       options: UIView.AnimationOptions.allowUserInteraction,
                       animations: {
                        self.editButton.transform = show ? CGAffineTransform.identity : scaleTransform
        }, completion: { _ in
            if !show {
                self.editButton.isHidden = true
            }
        })
    }
}
