// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import UIKit

private struct UIConstants {
  /// Used as backgrounds for favicons
  static let defaultColorStrings = ["2e761a", "399320", "40a624", "57bd35", "70cf5b",
                                    "90e07f", "b1eea5", "881606", "aa1b08", "c21f09",
                                    "d92215", "ee4b36", "f67964", "ffa792", "025295",
                                    "0568ba", "0675d3", "0996f8", "2ea3ff", "61b4ff",
                                    "95cdff", "00736f", "01908b", "01a39d", "01bdad",
                                    "27d9d2", "58e7e6", "89f4f5", "c84510", "e35b0f",
                                    "f77100", "ff9216", "ffad2e", "ffc446", "ffdf81",
                                    "911a2e", "b7223b", "cf2743", "ea385e", "fa526e",
                                    "ff7a8d", "ffa7b3"]
}

private struct FaviconUtils {
  /// Obtain the letter which will be used for monogram favicons based one of
  /// the following (in order):
  ///     1. The `baseDomain`'s first character (i.e. www.amazon.co.uk becomes
  ///        amazon.co.uk, which then gets a
  ///     2. The fallback character provided (for special cases such as
  ///        Bookmarks which have a title and we will use the first character
  ///        of that title)
  ///     3. The URL's `host`'s first character (i.e. `http://192.168.1.1`)
  ///        will return the letter `1`
  ///     4. The URL's `absoluteString`'s first character. Highly unlikely
  ///        this would be used. Basically only if a user specifically edits
  ///        a bookmark and changes the URL to say "https:Test" or something.
  ///        In that case, `T` would be used
  ///     5. If all of these cases fail we simply return the letter `W`
  static func monogramLetter(for url: URL, fallbackCharacter: Character?) -> String {
    guard let finalFallback = url.absoluteString.first else {
      return "W"
    }
    return (url.baseDomain?.first ?? fallbackCharacter ?? url.host?.first ?? finalFallback).uppercased()
  }
}

/// Extension on UIImage to determine if the image has primarily transparent edges
extension UIImage {
  public var hasTransparentEdges: Bool {
    if size.width.isZero || size.height.isZero {
      return false
    }

    guard let cgImage = createScaled(CGSize(width: 48.0, height: 48.0))?.cgImage else {
      return false
    }

    let iconSize = CGSize(width: cgImage.width, height: cgImage.height)
    let alphaInfo = cgImage.alphaInfo
    let hasAlphaChannel = alphaInfo == .first || alphaInfo == .last || alphaInfo == .premultipliedFirst || alphaInfo == .premultipliedLast
    if hasAlphaChannel, let dataProvider = cgImage.dataProvider {
      let length = CFDataGetLength(dataProvider.data)
      // Sample the image edges to determine if it has tranparent pixels
      if let data = CFDataGetBytePtr(dataProvider.data) {
        // Scoring system: if the pixel alpha is 1.0, score -1 otherwise
        // score +1. If the score at the end of scanning all edge pixels
        // is higher than 0, then the majority of the image's edges
        // are transparent and the image should be padded slightly
        var score: Int = 0
        func updateScore(x: Int, y: Int) {
          let location = ((Int(iconSize.width) * y) + x) * 4
          guard location + 3 < length else { return }
          let alpha = data[location + 3]
          if alpha == 255 {
            score -= 1
          } else {
            score += 1
          }
        }

        for x in 0..<Int(iconSize.width) {
          updateScore(x: x, y: 0)
          updateScore(x: x, y: Int(iconSize.height))
        }

        // We've already scanned the first and last pixel during
        // top/bottom pass
        for y in 1..<Int(iconSize.height) - 1 {
          updateScore(x: 0, y: y)
          updateScore(x: Int(iconSize.width), y: y)
        }
        return score > 0
      }
    }

    // No alpha channel OR cgImage.dataProvider is nil
    return false
  }
}

// MARK: - Rendering
extension UIImage {
  private static let maxScaledFaviconSize = 256.0
  
  /// Renders an image to a canvas with a background colour and passing
  @MainActor
  static func renderFavicon(_ image: UIImage, backgroundColor: UIColor?, shouldScale: Bool) async -> Favicon {
    if let cgImage = image.cgImage {
      let hasTransparentEdges = image.hasTransparentEdges
      
      var idealSize = image.size
      var padding = hasTransparentEdges ? 4.0 : 0.0
      
      if shouldScale && max(idealSize.width, idealSize.height) > maxScaledFaviconSize {
        let ratio = maxScaledFaviconSize / max(idealSize.width, idealSize.height)
        idealSize.width *= ratio
        idealSize.height *= ratio
      }
      
      if shouldScale && hasTransparentEdges {
        padding = max(idealSize.width, idealSize.height) * 0.20
      }
      
      if shouldScale && max(idealSize.width, idealSize.height) < 64.0 && min(idealSize.width, idealSize.height) > 0 {
        let ratio = 64.0 / min(idealSize.width, idealSize.height)
        idealSize.width *= ratio
        idealSize.height *= ratio
        padding = hasTransparentEdges ? max(idealSize.width, idealSize.height) * 0.20 : padding
      }
        
      let size = CGSize(
        width: idealSize.width + padding,
        height: idealSize.height + padding)

      let finalImage = await drawOnImageContext(size: size, { context, rect in
        context.cgContext.saveGState()
        context.cgContext.setFillColor(backgroundColor?.cgColor ?? UIColor.clear.cgColor)
        context.cgContext.fill(rect)

        context.cgContext.translateBy(x: 0.0, y: rect.size.height)
        context.cgContext.scaleBy(x: 1.0, y: -1.0)

        context.cgContext.draw(cgImage, in: rect.insetBy(dx: padding, dy: padding))
        context.cgContext.restoreGState()
      })

      return Favicon(image: finalImage, isMonogramImage: false, backgroundColor: backgroundColor ?? .clear)
    }

    return Favicon(image: image, isMonogramImage: false, backgroundColor: backgroundColor ?? .clear)
  }

  /// Renders a monogram letter of the specified URL onto a canvas with textColor and backgroundColor
  @MainActor
  static func renderMonogram(_ url: URL, textColor: UIColor?, backgroundColor: UIColor?, monogramString: String?) async -> Favicon {
    // Monogram favicon attributes
    let imageSize = CGSize(width: 64.0, height: 64.0)

    let createBackgroundColor = { (url: URL) -> UIColor in
      guard let hash = url.baseDomain?.fnv1a else {
        return .gray
      }
      let index = abs(hash) % (UIConstants.defaultColorStrings.count - 1)
      let colorHex = UIConstants.defaultColorStrings[index]
      return UIColor(colorString: colorHex)
    }

    let solidTextColor = textColor ?? .white
    let solidBackgroundColor = backgroundColor ?? createBackgroundColor(url)

    let label = UILabel()
    label.textColor = solidTextColor
    label.backgroundColor = solidBackgroundColor
    label.minimumScaleFactor = 0.5
    label.font = .systemFont(ofSize: imageSize.height / 2.0)

    let text = (monogramString ?? FaviconUtils.monogramLetter(
        for: url,
        fallbackCharacter: nil
    )).uppercased() as NSString

    let padding = 28.0
    let finalImage = await drawOnImageContext(size: imageSize) { context, rect in
      guard let font = label.font else { return }
      var fontSize = font.pointSize

      // Estimate the size of the font required to fit the context's bounds + padding
      // Usually we can do this by iterating and calculating the size that fits
      // But this is a very good estimated size
      let newSize = text.size(withAttributes: [.font: font.withSize(fontSize)])
      guard newSize.width > 0.0 && newSize.height > 0.0 else { return }

      let ratio = min(
        (rect.size.width - padding) / newSize.width,
        (rect.size.height - padding) / newSize.height)
      fontSize *= ratio

      if fontSize < label.font.pointSize * 0.5 {
        fontSize = label.font.pointSize * 0.5
      }

      if let backgroundColor = label.backgroundColor?.cgColor {
        context.cgContext.setFillColor(backgroundColor)
        context.cgContext.fill(rect)
      }

      let newFont = font.withSize(fontSize)
      let size = text.size(withAttributes: [.font: newFont])

      // Center the text drawing in the CGContext
      let x = (rect.size.width - size.width) / 2.0
      let y = (rect.size.height - size.height) / 2.0

      text.draw(
        in: rect.insetBy(dx: x, dy: y),
        withAttributes: [
          .font: newFont,
          .foregroundColor: label.textColor ?? .white,
        ]
      )
    }

    return Favicon(image: finalImage, isMonogramImage: true, backgroundColor: solidBackgroundColor)
  }
}

// MARK: - Drawing
extension UIImage {
  @MainActor
  private static func drawOnImageContext(size: CGSize, _ draw: (UIGraphicsImageRendererContext, CGRect) -> Void) async -> UIImage {
    await withCheckedContinuation { continuation in
      continuation.resume(returning: UIGraphicsImageRenderer(size: size).image { context in
        draw(context, CGRect(size: size))
      })
    }
  }
}
