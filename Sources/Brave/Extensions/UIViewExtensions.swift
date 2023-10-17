/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import UIKit

extension UIView {
  /**
     * Takes a screenshot of the view with the given size.
     */
  func screenshot(_ size: CGSize, offset: CGPoint? = nil, quality: CGFloat = 1) -> UIImage? {
    assert(0...1 ~= quality)

    let offset = offset ?? .zero

    // Temporary check to handle _UIGraphicsBeginImageContextWithOptions zero size error
    // Should be replaced with UIGraphicsImageRenderer
    guard size.width > 0, size.height > 0 else {
      return nil
    }
    
    UIGraphicsBeginImageContextWithOptions(size, false, UIScreen.main.scale * quality)
    drawHierarchy(in: CGRect(origin: offset, size: frame.size), afterScreenUpdates: false)
    let image = UIGraphicsGetImageFromCurrentImageContext()
    UIGraphicsEndImageContext()

    return image
  }

  /**
     * Takes a screenshot of the view with the given aspect ratio.
     * An aspect ratio of 0 means capture the entire view.
     */
  func screenshot(_ aspectRatio: CGFloat = 0, offset: CGPoint? = nil, quality: CGFloat = 1) -> UIImage? {
    assert(aspectRatio >= 0)

    var size: CGSize
    if aspectRatio > 0 {
      size = CGSize()
      let viewAspectRatio = frame.width / frame.height
      if viewAspectRatio > aspectRatio {
        size.height = frame.height
        size.width = size.height * aspectRatio
      } else {
        size.width = frame.width
        size.height = size.width / aspectRatio
      }
    } else {
      size = frame.size
    }

    return screenshot(size, offset: offset, quality: quality)
  }

  /// Getting a snapshot from a view using image renderer
  var snapshot: UIImage {
    UIGraphicsImageRenderer(size: bounds.size).image { _ in
      drawHierarchy(in: bounds, afterScreenUpdates: true)
    }
  }

  /**
     * rounds the requested corners of a view with the provided radius
     */
  func addRoundedCorners(_ cornersToRound: UIRectCorner, cornerRadius: CGSize, color: UIColor) {
    let rect = bounds
    let maskPath = UIBezierPath(roundedRect: rect, byRoundingCorners: cornersToRound, cornerRadii: cornerRadius)

    // Create the shape layer and set its path
    let maskLayer = CAShapeLayer()
    maskLayer.frame = rect
    maskLayer.path = maskPath.cgPath

    let roundedLayer = CALayer()
    roundedLayer.backgroundColor = color.cgColor
    roundedLayer.frame = rect
    roundedLayer.mask = maskLayer

    layer.insertSublayer(roundedLayer, at: 0)
    backgroundColor = .clear
  }

  /**
     This allows us to find the view in a current view hierarchy that is currently the first responder
     */
  static func findSubViewWithFirstResponder(_ view: UIView) -> UIView? {
    let subviews = view.subviews

    guard !subviews.isEmpty else {
      return nil
    }

    if let firstResponderSubview = subviews.first(where: { $0.isFirstResponder }) {
      return firstResponderSubview
    }

    guard let firstSubview = subviews.first(where: { !($0 is UIRefreshControl) }) else { return nil }

    return findSubViewWithFirstResponder(firstSubview)
  }

  /// Returns a line with height of 1pt. Used to imitate a separator line in custom views.
  static var separatorLine: UIView {
    let view = UIView().then {
      $0.backgroundColor = .braveSeparator
      $0.snp.makeConstraints {
        $0.height.equalTo(1.0 / UIScreen.main.scale)
      }
    }

    return view
  }
}
