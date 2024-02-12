// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import UIKit

/// ImageCacheProgress A block called while the image is downloading.
///
/// The first parameter is the received image size.
///
/// The second parameter is the
///
/// The last parameter is the original image URL.

public typealias ImageCacheProgress = ((Int, Int, URL) -> Void)?

/// ImageCacheCompletion A block called when the operation has completed.
///
/// This block takes the requested UIImage as first parameter and the NSData representation as second
/// parameter. In case of an error the image parameter will be nil and the third parameter may contain an
/// Error.
///
/// The fourth parameter is an `ImageCacheType` enum indicating if the image was retrieved from the web,
/// memory cache or disk cache.
///
/// The last parameter is the original image URL.

public typealias ImageCacheCompletion = ((UIImage?, Data?, Error?, ImageCacheType, URL) -> Void)?

public protocol ImageCacheProtocol {

  associatedtype ReturnAssociatedType

  /// Initialize an image cache with privacy protection.
  ///
  /// - parameter isPrivate: Bool representing privacy protection for this cache store.
  /// - parameter sandbox: The sandbox for this cache store.
  init(isPrivate: Bool, sandbox: String?)

  /// Downloads the image at the given URL if not present in the cache otherwise returns the cached version.
  ///
  /// - parameter url: URL to the image.
  /// - parameter options: A mask to specify ImageCacheOptions options.
  /// - parameter progress: An ImageCacheProgress block called while the image is downloading.
  /// - parameter completion: An ImageCacheCompletion block called when the operation has completed.
  /// - returns: ReturnAssociatedType customized using typealias.
  @discardableResult func load(from url: URL, options: ImageCacheOptions, progress: ImageCacheProgress, completion: ImageCacheCompletion) -> ReturnAssociatedType?

  /// Returns whether the image at the given URL is cached in memory or not.
  ///
  /// - parameter url: URL to image.
  /// - returns: true if the image is cached in memory, otherwise false.
  func isCached(_ url: URL) -> Bool

  /// Remove image at the given URL from the cache.
  ///
  /// - parameter url: URL to image.
  func remove(fromCache url: URL)

  /// Clears the memory cache.
  func clearMemoryCache()

  /// Clears the disk cache.
  func clearDiskCache()

}
