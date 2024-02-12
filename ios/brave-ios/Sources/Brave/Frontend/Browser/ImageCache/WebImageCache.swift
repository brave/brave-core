// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import SDWebImage
import Data
import Shared
import BraveShared
import os.log

final public class WebImageCache: ImageCacheProtocol {

  private let webImageManager: SDWebImageManager

  private let isPrivate: Bool

  public typealias ReturnAssociatedType = SDWebImageOperation

  deinit {
    clearMemoryCache()
    clearDiskCache()
  }

  public init(isPrivate: Bool, sandbox: String? = nil) {
    self.isPrivate = isPrivate

    var imageCache: SDImageCache

    if let sandbox = sandbox {
      imageCache = SDImageCache(namespace: sandbox, diskCacheDirectory: sandbox)
    } else {
      imageCache = SDImageCache.shared
    }

    let imageDownloader = SDWebImageDownloader.shared

    webImageManager = SDWebImageManager(cache: imageCache, loader: imageDownloader)
  }

  @discardableResult
  public func load(
    from url: URL, options: ImageCacheOptions = [], progress: ImageCacheProgress = nil,
    completion: ImageCacheCompletion = nil
  ) -> SDWebImageOperation? {
    let progressBlock: SDWebImageDownloaderProgressBlock = { receivedSize, expectedSize, url in
      guard let url = url else {
        return
      }

      progress?(receivedSize, expectedSize, url)
    }

    var webImageOptions = self.webImageOptions
    let webImageContext = self.webImageContext

    if options.contains(.lowPriority) {
      webImageOptions.append(.lowPriority)
    }

    webImageOptions.append(.scaleDownLargeImages)

    let imageOperation = webImageManager.loadImage(with: url, options: SDWebImageOptions(webImageOptions), context: webImageContext, progress: progressBlock) { image, data, error, webImageCacheType, _, imageURL in
      let key = self.webImageManager.cacheKey(for: url)
      if let image = image, !self.isPrivate {
        self.webImageManager.imageCache.store(image, imageData: data, forKey: key, cacheType: .all)
      }

      let cacheType = self.mapImageCacheType(from: webImageCacheType)
      completion?(image, data, error, cacheType, url)
    }

    return imageOperation
  }

  public func cacheImage(image: UIImage, data: Data?, url: URL) {
    let key = self.webImageManager.cacheKey(for: url)
    webImageManager.imageCache.store(image, imageData: data, forKey: key, cacheType: .all)
  }

  public func getCachedImage(for url: URL, completion: @escaping (UIImage?) -> Void) {
    let key = self.webImageManager.cacheKey(for: url)
    webImageManager.imageCache.queryImage(forKey: key, options: [.fromCacheOnly, .queryMemoryData], context: nil, cacheType: .all) { image, _, _ in
      completion(image)
    }
  }
  
  public func isCached(_ url: URL) -> Bool {
    return webImageManager.cacheKey(for: url) != nil
  }

  public func remove(fromCache url: URL) {
    let key = webImageManager.cacheKey(for: url)
    webImageManager.imageCache.removeImage(forKey: key, cacheType: .all)
  }

  public func clearMemoryCache() {
    webImageManager.imageCache.clear(with: .memory)
  }

  public func clearDiskCache() {
    if !isPrivate {
      webImageManager.imageCache.clear(with: .disk)
    }
  }

}

extension WebImageCache {

  private var webImageOptions: [SDWebImageOptions] {
    return [.retryFailed, .continueInBackground]
  }

  private var webImageContext: [SDWebImageContextOption: Any] {
    var context: [SDWebImageContextOption: Any] = [:]
    if isPrivate {
      context[.storeCacheType] = SDImageCacheType.memory.rawValue
    }
    return context
  }

  private func mapImageCacheType(from imageCacheType: SDImageCacheType) -> ImageCacheType {
    switch imageCacheType {
    case .none:
      return ImageCacheType.none

    case .memory:
      return ImageCacheType.memory

    case .disk:
      return ImageCacheType.disk

    case .all:
      return ImageCacheType.disk

    @unknown default:
      return ImageCacheType.none
    }
  }

}
