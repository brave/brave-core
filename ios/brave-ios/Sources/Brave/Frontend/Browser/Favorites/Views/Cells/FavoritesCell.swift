// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveShared
import BraveUI
import Shared
import UIKit

@objc protocol FavoriteCellDelegate {
  func editFavorite(_ favoriteCell: FavoritesCell)
}

class FavoritesCell: UICollectionViewCell, CollectionViewReusable {
  static let imageAspectRatio: Float = 1.0
  static let placeholderImage = UIImage(
    named: "defaultTopSiteIcon",
    in: .module,
    compatibleWith: nil
  )!
  static let identifier = "FavoriteCell"

  private struct UI {
    /// Ratio of width:height of the thumbnail image.
    static let cornerRadius: CGFloat = 8
    static let spacing: CGFloat = 8
    static let labelAlignment: NSTextAlignment = .center
  }

  weak var delegate: FavoriteCellDelegate?

  var imageInsets: UIEdgeInsets = UIEdgeInsets.zero
  var cellInsets: UIEdgeInsets = UIEdgeInsets.zero

  private let textLabel = UILabel().then {
    $0.setContentHuggingPriority(
      UILayoutPriority.defaultHigh,
      for: NSLayoutConstraint.Axis.vertical
    )
    $0.font = DynamicFontHelper.defaultHelper.defaultSmallFontBold
    $0.textAlignment = UI.labelAlignment
    $0.lineBreakMode = NSLineBreakMode.byWordWrapping
    $0.numberOfLines = 2
    $0.textColor = .white
  }

  var title: String? {
    didSet {
      textLabel.attributedText = title.map {
        let shadow = NSShadow()
        shadow.shadowColor = UIColor(white: 0, alpha: 0.35)
        shadow.shadowOffset = CGSize(width: 0, height: 1)
        shadow.shadowBlurRadius = 4
        return NSAttributedString(
          string: $0,
          attributes: [
            .shadow: shadow
          ]
        )
      }
    }
  }

  let imageContainerView: UIVisualEffectView = {
    let view = UIVisualEffectView()
    if #available(iOS 26, *) {
      let glass = UIGlassEffect(style: .regular)
      glass.isInteractive = true
      view.effect = glass
    } else {
      view.effect = UIBlurEffect(style: .systemThinMaterial)
    }
    view.layer.cornerRadius = 16
    view.layer.cornerCurve = .continuous
    view.clipsToBounds = true
    return view
  }()

  let imageView = LargeFaviconView(
    config: .init(borderColor: .clear, borderWidth: 0, cornerRadius: 8, displayBackground: false)
  )

  override var isHighlighted: Bool {
    didSet {
      UIView.animate(
        withDuration: 0.25,
        delay: 0,
        options: [.beginFromCurrentState],
        animations: {
          self.imageView.alpha = self.isHighlighted ? 0.7 : 1.0
        }
      )
    }
  }

  let stackView = UIStackView().then {
    $0.axis = .vertical
    $0.spacing = UI.spacing
    $0.alignment = .center
    $0.isUserInteractionEnabled = false
  }

  override init(frame: CGRect) {
    super.init(frame: frame)

    isAccessibilityElement = true
    overrideUserInterfaceStyle = .dark

    imageContainerView.contentView.addSubview(imageView)

    contentView.addSubview(stackView)
    stackView.addArrangedSubview(imageContainerView)
    stackView.addArrangedSubview(textLabel)

    imageContainerView.snp.makeConstraints {
      $0.height.equalTo(imageContainerView.snp.width)
      $0.leading.trailing.equalToSuperview().inset(4)
    }
    imageView.snp.makeConstraints {
      $0.edges.equalToSuperview().inset(8)
    }
    stackView.snp.makeConstraints {
      $0.top.leading.trailing.equalToSuperview()
      $0.bottom.lessThanOrEqualToSuperview()
    }

    // Prevents the textLabel from getting squished in relation to other view priorities.
    textLabel.setContentCompressionResistancePriority(.defaultHigh, for: .vertical)

    addInteraction(UIPointerInteraction(delegate: self))
  }

  deinit {
    NotificationCenter.default.removeObserver(self, name: .thumbnailEditOn, object: nil)
    NotificationCenter.default.removeObserver(self, name: .thumbnailEditOff, object: nil)
  }

  required init?(coder aDecoder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  override func prepareForReuse() {
    super.prepareForReuse()
    backgroundColor = .clear
  }

  override func preferredLayoutAttributesFitting(
    _ layoutAttributes: UICollectionViewLayoutAttributes
  ) -> UICollectionViewLayoutAttributes {
    // Size of cells are determined outside of cell
    return layoutAttributes
  }

  static func height(forWidth width: CGFloat) -> CGFloat {
    let imageHeight = width - 8
    let labelHeight = (DynamicFontHelper.defaultHelper.defaultSmallFontBold.lineHeight * 2)
    return ceil(imageHeight + UI.spacing + labelHeight)
  }
}

extension FavoritesCell: UIPointerInteractionDelegate {
  func pointerInteraction(
    _ interaction: UIPointerInteraction,
    styleFor region: UIPointerRegion
  ) -> UIPointerStyle? {
    let preview = UITargetedPreview(view: imageContainerView)
    return UIPointerStyle(effect: .lift(preview))
  }
}
