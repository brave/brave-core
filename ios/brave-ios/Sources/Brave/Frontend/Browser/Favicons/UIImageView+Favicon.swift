// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import UIKit
import BraveShared
import Favicon

extension UIImageView {

  private struct AssociatedObjectKeys {
    static var faviconTask: Int = 0
  }

  /// The associated favicon fetcher for a UIImageView to ensure we don't
  /// immediately cancel the FaviconFetcher load when the function call goes
  /// out of scope.
  ///
  /// Must use objc associated objects because we are extending UIKit in an
  /// extension
  private var faviconTask: Task<Void, Error>? {
    get { objc_getAssociatedObject(self, &AssociatedObjectKeys.faviconTask) as? Task<Void, Error> }
    set { objc_setAssociatedObject(self, &AssociatedObjectKeys.faviconTask, newValue, .OBJC_ASSOCIATION_RETAIN_NONATOMIC) }
  }

  /// Load the favicon from a site URL directly into a `UIImageView`. If no
  /// favicon is found, a monogram will be used where the letter is determined
  /// based on the site URL.
  func loadFavicon(for siteURL: URL, isPrivateBrowsing: Bool, monogramFallbackCharacter: Character? = nil, completion: ((Favicon?) -> Void)? = nil) {
    cancelFaviconLoad()
    
    if let icon = FaviconFetcher.getIconFromCache(for: siteURL) {
      self.image = icon.image ?? Favicon.defaultImage
      return
    }
    
    self.image = Favicon.defaultImage
    faviconTask = Task { @MainActor in
      do {
        let favicon = try await FaviconFetcher.loadIcon(url: siteURL, persistent: !isPrivateBrowsing)
        self.image = favicon.image ?? Favicon.defaultImage
        completion?(favicon)
      } catch {
        self.image = Favicon.defaultImage
        completion?(nil)
      }
    }
  }

  /// Cancel any pending favicon load task. This is to prevent race condition UI glitches with reusable table/collection view cells.
  func cancelFaviconLoad() {
    faviconTask?.cancel()
    faviconTask = nil
  }
  
  func clearMonogramFavicon() {
    
  }
}
