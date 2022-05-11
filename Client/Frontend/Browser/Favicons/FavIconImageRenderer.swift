// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Data
import UIKit

/// A class for rendering a FavIcon onto a `UIImage`
class FavIconImageRenderer {
  private var task: DispatchWorkItem?

  deinit {
    task?.cancel()
  }

  func loadIcon(siteURL: URL, kind: FaviconFetcher.Kind = .favicon, persistent: Bool, completion: ((UIImage?) -> Void)?) {
    task?.cancel()
    task = DispatchWorkItem {
      let domain = Domain.getOrCreate(forUrl: siteURL, persistent: persistent)
      var faviconFetcher: FaviconFetcher? = FaviconFetcher(siteURL: siteURL, kind: kind, domain: domain)
      faviconFetcher?.load() { [weak self] _, attributes in
        faviconFetcher = nil

        guard let self = self,
          let cancellable = self.task,
          !cancellable.isCancelled
        else {
          completion?(nil)
          return
        }

        if let image = attributes.image {
          if let backgroundColor = attributes.backgroundColor,
            let cgImage = image.cgImage {
            // attributes.includesPadding sometimes returns 0 for icons that should. It's better this way to always include the padding.
            let padding = 4.0
            let size = CGSize(
              width: image.size.width + padding,
              height: image.size.height + padding)

            let finalImage = self.renderOnImageContext(size: size) { context, rect in
              context.saveGState()
              context.setFillColor(backgroundColor.cgColor)
              context.fill(rect)

              context.translateBy(x: 0.0, y: rect.size.height)
              context.scaleBy(x: 1.0, y: -1.0)

              context.draw(cgImage, in: rect.insetBy(dx: padding, dy: padding))
              context.restoreGState()
            }
            completion?(finalImage)
          } else {
            completion?(image)
          }
        } else {
          // Monogram favicon attributes
          let label = UILabel().then {
            $0.textColor = .white
            $0.backgroundColor = .clear
            $0.minimumScaleFactor = 0.5
          }

          let text =
            FaviconFetcher.monogramLetter(
              for: siteURL,
              fallbackCharacter: nil
            ) as NSString

          let padding = 4.0
          let finalImage = self.renderOnImageContext { context, rect in
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

            if let backgroundColor = attributes.backgroundColor?.cgColor {
              context.setFillColor(backgroundColor)
              context.fill(rect)
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
                .foregroundColor: UIColor.white,
              ])
          }

          completion?(finalImage)
        }
      }
    }

    if let task = task {
      DispatchQueue.main.async(execute: task)
    }
  }

  private func renderOnImageContext(size: CGSize, _ draw: (CGContext, CGRect) -> Void) -> UIImage? {
    let size = CGSize(width: size.width, height: size.height)
    UIGraphicsBeginImageContextWithOptions(size, false, UIScreen.main.scale)
    draw(UIGraphicsGetCurrentContext()!, CGRect(size: size))
    let img = UIGraphicsGetImageFromCurrentImageContext()
    UIGraphicsEndImageContext()
    return img
  }

  private func renderOnImageContext(_ draw: (CGContext, CGRect) -> Void) -> UIImage? {
    renderOnImageContext(size: CGSize(width: 16.0, height: 16.0), draw)
  }
}
