/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation

/// Protocol defining a few things the Client has to give to BraveRewards
public protocol RewardsDataSource: AnyObject {
  /// The string to display given a URL (Usually eTLD+1). If `nil` is returned, `URL.host` will be
  /// used to display
  func displayString(for url: URL) -> String?
  
  /// Download or retrieve a cached version of the favicon given a URL. This should also return the
  /// default letter/color favicon if a website has no official favicon.
  ///
  /// Execute completionBlock with FaviconData
  func retrieveFavicon(for pageURL: URL,
                       on imageView: UIImageView)
  
  /// Get the page HTML for the given tab id if its available
  func pageHTML(for tabId: UInt64, completionHandler: @escaping (String?) -> Void)
}
