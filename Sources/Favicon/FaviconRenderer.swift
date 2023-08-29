// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import UIKit
import BraveCore

/// A class for rendering a FavIcon onto a `UIImage`
public class FaviconRenderer {
  @MainActor
  public static func loadIcon(for url: URL, persistent: Bool) async throws -> Favicon {
    // Load the Favicon from Brave-Core
    let attributes: FaviconAttributes = await withCheckedContinuation { continuation in
      FaviconLoader.getForPrivateMode(!persistent).favicon(forPageURLOrHost: url, sizeInPoints: .desiredLargest, minSizeInPoints: .desiredMedium /*32x32*/) { _, attributes in

        // If the completion block was called with the `default` image, do nothing
        if attributes.usesDefaultImage {
          return
        }

        continuation.resume(returning: attributes)
      }
    }

    try Task.checkCancellation()

    if let image = attributes.faviconImage {
      // Render the Favicon on a UIImage
      let favicon = await UIImage.renderFavicon(image, backgroundColor: attributes.backgroundColor, shouldScale: true)
      try Task.checkCancellation()
      return favicon
    } else {
      // Render the Monogram on a UIImage
      let textColor = !attributes.isDefaultBackgroundColor ? attributes.textColor : nil
      let backColor = !attributes.isDefaultBackgroundColor ? attributes.backgroundColor : nil

      let favicon = await UIImage.renderMonogram(url, textColor: textColor, backgroundColor: backColor, monogramString: attributes.monogramString)
      try Task.checkCancellation()
      return favicon
    }
  }
}
