// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import CoreData
import Data
import Foundation
import Shared
import Storage
import UIKit

/// A set of methods related to managing favorites. Most are just wrappers over Bookmark model.
struct FavoritesHelper {
  // Indicates if favorites have been initialized.
  static let initPrefsKey = "FavoritesHelperInitPrefsKey"

  // MARK: - Favorites initialization
  static func addDefaultFavorites() {
    Favorite.addDefaults(from: FavoritesPreloadedData.getList())
  }

  static func add(url: URL, title: String?) {
    Favorite.add(url: url, title: title)
  }

  static func isAlreadyAdded(_ url: URL) -> Bool {
    return Favorite.contains(url: url)
  }

  static func fallbackIcon(
    withLetter letter: String,
    color: UIColor,
    andSize iconSize: CGSize
  ) -> UIImage {
    let renderer = UIGraphicsImageRenderer(size: iconSize)
    return renderer.image { ctx in
      let rectangle = CGRect(x: 0, y: 0, width: iconSize.width, height: iconSize.height)

      ctx.cgContext.addRect(rectangle)
      ctx.cgContext.setFillColor(color.cgColor)
      ctx.cgContext.drawPath(using: .fillStroke)

      let paragraphStyle = NSMutableParagraphStyle()
      paragraphStyle.alignment = .center

      let attrs = [
        NSAttributedString.Key.font: UIFont(name: "HelveticaNeue-Thin", size: iconSize.height - 90)
          ?? UIFont.systemFont(ofSize: iconSize.height - 90, weight: UIFont.Weight.thin),
        NSAttributedString.Key.paragraphStyle: paragraphStyle,
        NSAttributedString.Key.backgroundColor: UIColor.clear,
      ]

      let string: NSString = NSString(string: letter.uppercased())
      let size = string.size(withAttributes: attrs)
      string.draw(
        at: CGPoint(x: (iconSize.width - size.width) / 2, y: (iconSize.height - size.height) / 2),
        withAttributes: attrs
      )
    }
  }
}
