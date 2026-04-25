// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import UIKit

private let imageLock = NSLock()

extension CGRect {
  public init(width: CGFloat, height: CGFloat) {
    self.init(x: 0, y: 0, width: width, height: height)
  }

  public init(size: CGSize) {
    self.init(origin: .zero, size: size)
  }
}

extension UIImage {
  /// Despite docs that say otherwise, UIImage(data: NSData) isn't thread-safe (see bug 1223132).
  /// As a workaround, synchronize access to this initializer.
  /// This fix requires that you *always* use this over UIImage(data: NSData)!
  public static func imageFromDataThreadSafe(_ data: Data) -> UIImage? {
    imageLock.lock()
    let image = UIImage(data: data)
    imageLock.unlock()
    return image
  }

  public func createScaled(_ size: CGSize) -> UIImage? {
    guard size.width > 0, size.height > 0 else {
      return nil
    }

    UIGraphicsBeginImageContextWithOptions(size, false, 0)
    draw(in: CGRect(size: size))
    let scaledImage = UIGraphicsGetImageFromCurrentImageContext()
    UIGraphicsEndImageContext()
    return scaledImage!
  }

  /// Return a UIImage which will always render as a template
  public var template: UIImage {
    return withRenderingMode(.alwaysTemplate)
  }

  public func scale(toSize size: CGSize) -> UIImage {
    if self.size == size {
      return self
    }

    let rendererFormat = UIGraphicsImageRendererFormat.default()
    rendererFormat.opaque = false
    rendererFormat.scale = 1.0

    let scaledImageRect = CGRect(x: 0, y: 0, width: size.width, height: size.height)
    let renderer = UIGraphicsImageRenderer(size: size, format: rendererFormat)
    let scaledImage = renderer.image { context in
      self.draw(in: scaledImageRect)
    }

    return scaledImage
  }

  public func textToImage(
    drawText text: String,
    textFont: UIFont? = nil,
    textColor: UIColor? = nil,
    atPoint point: CGPoint
  ) -> UIImage? {
    guard size.width > 0, size.height > 0 else {
      return nil
    }

    let paragraphStyle = NSMutableParagraphStyle()
    paragraphStyle.alignment = .center

    let font = textFont ?? UIFont.systemFont(ofSize: 20, weight: .medium)

    let fontAttributes: [NSAttributedString.Key: Any] = [
      .paragraphStyle: paragraphStyle,
      .font: font,
      .foregroundColor: textColor ?? UIColor.white,
    ]

    UIGraphicsBeginImageContextWithOptions(size, false, scale)
    draw(in: CGRect(origin: CGPoint.zero, size: size))
    let rect = CGRect(origin: point, size: size)
    text.draw(in: rect.insetBy(dx: 15, dy: 0), withAttributes: fontAttributes)
    let newImage = UIGraphicsGetImageFromCurrentImageContext()
    UIGraphicsEndImageContext()
    return newImage
  }

  public func imageWithInsets(insets: UIEdgeInsets) -> UIImage? {
    guard size.width > 0, size.height > 0 else {
      return nil
    }

    UIGraphicsBeginImageContextWithOptions(
      CGSize(
        width: self.size.width + insets.left + insets.right,
        height: self.size.height + insets.top + insets.bottom
      ),
      false,
      self.scale
    )
    let _ = UIGraphicsGetCurrentContext()
    let origin = CGPoint(x: insets.left, y: insets.top)
    self.draw(at: origin)
    let imageWithInsets = UIGraphicsGetImageFromCurrentImageContext()
    UIGraphicsEndImageContext()
    return imageWithInsets
  }
}
