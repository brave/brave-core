/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import FastImageCache
import CoreGraphics

class ImageEntity: NSObject, FICEntity {
  var uuid: String?
  var url: URL?
  
  required init(url: URL) {
    super.init()
    self.url = url
  }
  
  var fic_UUID: String {
    get {
      if uuid == nil, let urlString = self.url?.absoluteString {
        let uuidBytes = FICUUIDBytesFromMD5HashOfString(urlString.lowercased())
        uuid = FICStringWithUUIDBytes(uuidBytes)
      }
      return uuid ?? UUID().uuidString
    }
  }
  
  var sourceImageUUID: String {
    get {
      return uuid ?? UUID().uuidString
    }
  }
  
  func sourceImageURL(withFormatName formatName: String) -> URL? {
    return url
  }
  
  func drawingBlock(for image: UIImage, withFormatName formatName: String) -> FICEntityImageDrawingBlock? {
    let drawingBlock: FICEntityImageDrawingBlock = { (context, contextSize) in
        guard let context = context else { return }
        let contextBounds: CGRect = CGRect(x: 0, y: 0, width: contextSize.width, height: contextSize.height)
        context.clear(contextBounds)
        UIGraphicsPushContext(context)
        image.draw(in: contextBounds)
        UIGraphicsPopContext()
    }
    return drawingBlock
  }
}

enum ImageCacheEntityType: String {
  case square = "com.brave.imageFormatPortrait"
  case portrait = "com.brave.imageFormatSquare"
}

class ImageCache: NSObject, FICImageCacheDelegate {
  static let shared = ImageCache()
  
  fileprivate let ImageFormatFrameDevice = "com.brave.imageFormatFrameDevice"
  
  fileprivate var bitmapCache: FICImageCache?
  
  fileprivate var portraitSize: CGSize {
    get {
      // Always know portrait size for screen.
      // FIC entity design requires specifying size associated with entity with each format.
      // To simplify image cache for our use (screeshots on webview) enforce portrait sizing.
      // Display differences happen when capturing at landscape. An alternative design approach
      // would be to create both portrait and landscape sizes on each cache() call and allow FIC
      // to return image based on device orientation. This would require a larger cache size
      // and we'd still add to complexity of accessing image cache. A better pattern for alternatvie
      // image formats may be to subclass imageCache, or to create a new instance and pass in format.
      var size = CGSize(width: UIScreen.main.bounds.width, height: UIScreen.main.bounds.height)
      if UIApplication.shared.statusBarOrientation != .portrait {
        size = CGSize(width: size.height, height: size.width)
      }
      return size
    }
  }
  
  fileprivate var squareSize: CGSize = CGSize(width: 250, height: 250)
  
  override init() {
    super.init()
    
    // Guard from locked filesystem on private browsing.
    // FIC will crash on init.
    let fileManager = FileManager.default
    let paths = NSSearchPathForDirectoriesInDomains(.cachesDirectory, .userDomainMask, true) as [String]
    guard let path = paths.first else { return }
    if !fileManager.isWritableFile(atPath: path) {
      return
    }
    
    let imageFormat = FICImageFormat()
    imageFormat.name = ImageCacheEntityType.portrait.rawValue
    imageFormat.family = ImageFormatFrameDevice
    imageFormat.style = .style16BitBGR
    imageFormat.imageSize = portraitSize
    imageFormat.maximumCount = 1000
    imageFormat.devices = UIDevice.current.userInterfaceIdiom == .phone ? .phone : .pad
    imageFormat.protectionMode = .none
    
    let imageSquareFormat = FICImageFormat()
    imageSquareFormat.name = ImageCacheEntityType.square.rawValue
    imageSquareFormat.family = ImageFormatFrameDevice
    imageSquareFormat.style = .style32BitBGRA
    imageSquareFormat.imageSize = squareSize
    imageSquareFormat.maximumCount = 5000
    imageSquareFormat.devices = UIDevice.current.userInterfaceIdiom == .phone ? .phone : .pad
    imageSquareFormat.protectionMode = .none
    
    bitmapCache = FICImageCache(nameSpace: "com.brave.images")
    bitmapCache?.delegate = self
    bitmapCache?.setFormats([imageFormat, imageSquareFormat])
  }
  
  func cache(_ image: UIImage, url: URL, type: ImageCacheEntityType, callback: (() -> Void)?) {
    guard let bitmapCache = bitmapCache else { callback?(); return }
    
    let entity = ImageEntity(url: url)
    let format = type.rawValue
    if bitmapCache.imageExists(for: entity, withFormatName: format) {
      bitmapCache.deleteImage(for: entity, withFormatName: format)
    }
    
    // Size enforced crop.
    var _image = UIImage()
    switch type {
    case .portrait:
      _image = resize(image, size: portraitSize)
    case .square:
      _image = image
    }
    
    bitmapCache.setImage(_image, for: entity, withFormatName: format, completionBlock: { (cachedEntity, format, cachedImage) in
      callback?()
    })
  }
  
  func image(_ url: URL, type: ImageCacheEntityType, callback: @escaping (_ image: UIImage?) -> Void) {
    guard let bitmapCache = bitmapCache else { callback(nil); return }
    
    let entity = ImageEntity(url: url)
    let format = type.rawValue
    if !bitmapCache.imageExists(for: entity, withFormatName: format) {
      callback(nil)
      return
    }
    bitmapCache.retrieveImage(for: entity, withFormatName: format) { (cachedEntity, format, cachedImage) in
      callback(cachedImage)
    }
  }
  
  func hasImage(_ url: URL, type: ImageCacheEntityType) -> Bool {
    guard let bitmapCache = bitmapCache else { return false }
    
    let entity = ImageEntity(url: url)
    let format = type.rawValue
    if bitmapCache.imageExists(for: entity, withFormatName: format) {
      return true
    }
    return false
  }
  
  func remove(_ url: URL, type: ImageCacheEntityType) {
    guard let bitmapCache = bitmapCache else { return }
    
    let entity = ImageEntity(url: url)
    let format = type.rawValue
    if bitmapCache.imageExists(for: entity, withFormatName: format) {
      bitmapCache.deleteImage(for: entity, withFormatName: format)
    }
  }
  
  fileprivate func resize(_ image: UIImage, size: CGSize) -> UIImage {
    let ratio = size.width / size.height
    let height = min(size.height, image.size.height)
    let width = height * ratio
    let crop = CGRect(x: 0, y: 0, width: width * image.scale, height: height * image.scale)
    guard let imageRef = image.cgImage?.cropping(to: crop) else { return UIImage() }
    return UIImage(cgImage: imageRef, scale: image.scale, orientation: image.imageOrientation)
  }
  
  func imageCache(_ imageCache: FICImageCache, errorDidOccurWithMessage errorMessage: String) {
    debugPrint("FICImageCache Error: \(errorMessage)")
  }
  
  func clear() {
    guard let bitmapCache = bitmapCache else { return }
    
    bitmapCache.reset()
  }
}
