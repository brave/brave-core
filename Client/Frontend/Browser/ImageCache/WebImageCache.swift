// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import SDWebImage

final class WebImageCache: ImageCacheProtocol {
    
    private let webImageManager: SDWebImageManager
    
    private let isPrivate: Bool
    
    typealias ReturnAssociatedType = SDWebImageOperation
    
    deinit {
        clearMemoryCache()
        clearDiskCache()
    }
    
    init(isPrivate: Bool, sandbox: String? = nil) {
        self.isPrivate = isPrivate
        
        var imageCache: SDImageCache
        
        if let sandbox = sandbox {
            imageCache = SDImageCache(namespace: sandbox, diskCacheDirectory: sandbox)
        } else {
            imageCache = SDImageCache.shared()
        }
        
        let imageDownloader = SDWebImageDownloader.shared()
        
        webImageManager = SDWebImageManager(cache: imageCache, downloader: imageDownloader)
    }
    
    @discardableResult
    func load(from url: URL, options: ImageCacheOptions = [], progress: ImageCacheProgress = nil,
              completion: ImageCacheCompletion = nil) -> SDWebImageOperation? {
        let progressBlock: SDWebImageDownloaderProgressBlock = { receivedSize, expectedSize, url in
            guard let url = url else {
                return
            }
            
            progress?(receivedSize, expectedSize, url)
        }
        
        var webImageOptions = self.webImageOptions
        
        if options.contains(.lowPriority) {
            webImageOptions.append(.lowPriority)
        }

        let imageOperation = webImageManager.loadImage(with: url, options: SDWebImageOptions(webImageOptions), progress: progressBlock) { image, data, error, webImageCacheType, _, imageURL in
            if let image = image, !self.isPrivate {
                self.webImageManager.saveImage(toCache: image, for: url)
            }
            
            let cacheType = self.mapImageCacheType(from: webImageCacheType)
            completion?(image, data, error, cacheType, url)
        }
        
        return imageOperation
    }
    
    func isCached(_ url: URL) -> Bool {
        return webImageManager.cacheKey(for: url) != nil
    }
    
    func isPersisted(_ url: URL) -> Bool {
        let key = webImageManager.cacheKey(for: url)
        let exists = webImageManager.imageCache?.diskImageDataExists(withKey: key) ?? false
        return exists
    }
    
    func remove(fromCache url: URL) {
        let key = webImageManager.cacheKey(for: url)
        webImageManager.imageCache?.removeImage(forKey: key)
    }
    
    func clearMemoryCache() {
        webImageManager.imageCache?.clearMemory()
    }
    
    func clearDiskCache() {
        if !isPrivate {
            webImageManager.imageCache?.clearDisk()
        }
    }
    
}

extension WebImageCache {
    
    private var webImageOptions: [SDWebImageOptions] {
        var options: [SDWebImageOptions] = [.retryFailed, .continueInBackground]
        
        if isPrivate {
            options.append(.cacheMemoryOnly)
        }
        
        return options
    }
    
    private func mapImageCacheType(from imageCacheType: SDImageCacheType) -> ImageCacheType {
        switch imageCacheType {
        case .none:
            return ImageCacheType.none
            
        case .memory:
            return ImageCacheType.memory
            
        case .disk:
            return ImageCacheType.disk

        @unknown default:
            return ImageCacheType.none
        }
    }
    
}
