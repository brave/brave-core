// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import BraveShared

class UrlBarTextField: AutocompleteTextField {

  @objc dynamic var clearButtonTintColor: UIColor? {
    didSet {
      // Clear previous tinted image that's cache and ask for a relayout
      tintedClearImage = nil
      setNeedsLayout()
    }
  }

  fileprivate var tintedClearImage: UIImage?

  override init(frame: CGRect) {
    super.init(frame: frame)
  }

  required init?(coder aDecoder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  override func layoutSubviews() {
    super.layoutSubviews()

    // Since we're unable to change the tint color of the clear image, we need to iterate through the
    // subviews, find the clear button, and tint it ourselves. Thanks to Mikael Hellman for the tip:
    // http://stackoverflow.com/questions/27944781/how-to-change-the-tint-color-of-the-clear-button-on-a-uitextfield
    if clearButtonMode != .never {
      for view in subviews as [UIView] {
        if let button = view as? UIButton {
          if let image = button.image(for: []) {
            if tintedClearImage == nil {
              tintedClearImage = tintImage(image, color: clearButtonTintColor)
            }

            if button.imageView?.image != tintedClearImage {
              button.setImage(tintedClearImage, for: [])
            }
          }
        }
      }
    }
  }

  fileprivate func tintImage(_ image: UIImage, color: UIColor?) -> UIImage {
    guard let color = color else { return image }

    let size = image.size

    UIGraphicsBeginImageContextWithOptions(size, false, 2)
    let context = UIGraphicsGetCurrentContext()!
    image.draw(at: .zero, blendMode: .normal, alpha: 1.0)

    context.setFillColor(color.cgColor)
    context.setBlendMode(.sourceIn)
    context.setAlpha(1.0)

    let rect = CGRect(size: image.size)
    context.fill(rect)
    let tintedImage = UIGraphicsGetImageFromCurrentImageContext()!
    UIGraphicsEndImageContext()

    return tintedImage
  }
}
