// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import SwiftUI

struct GIFImage: UIViewRepresentable {
  private let asset: String
  private let animationRepeatCount: Int
  private let animate: Bool

  public init(asset: String, animationRepeatCount: Int = 1, animate: Bool) {
    self.asset = asset
    self.animationRepeatCount = animationRepeatCount
    self.animate = animate
  }

  func makeUIView(context: Context) -> GIFImageView {
    return GIFImageView(asset: asset)
  }

  func updateUIView(_ uiView: GIFImageView, context: Context) {
    uiView.startAnimation(start: animate)
  }
}

class GIFImageView: UIView, CAAnimationDelegate {
  private var assetName: String = ""
  private var animationRepeatCount: Int = 1
  
  private let imageView = UIImageView()
  private var firstFrame: UIImage?
  private var lastFrame: UIImage?
  private var framesCount: Int = 0
  private var animationStarted = false
  
  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  init(asset: String, animationRepeatCount: Int = 1) {
    super.init(frame: .zero)
    self.assetName = asset
    self.animationRepeatCount = animationRepeatCount
    initView()
  }
  
  override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
    super.traitCollectionDidChange(previousTraitCollection)
    loadAsset()
  }
  
  private func loadAsset() {
    let frames = Self.getFramesFrom(asset: assetName)
    firstFrame = frames.first
    lastFrame = frames.last
    framesCount = frames.count
    imageView.image = frames.first
    imageView.animationImages = frames
    imageView.animationRepeatCount = animationRepeatCount
  }

  override func layoutSubviews() {
    super.layoutSubviews()
    imageView.frame = bounds
    self.addSubview(imageView)
  }
  
  func startAnimation(start: Bool) {
    if start {
      imageView.image = lastFrame
      imageView.startAnimating()
    } else {
      imageView.image = firstFrame
      imageView.stopAnimating()
    }
  }

  private func initView() {
    imageView.contentMode = .scaleAspectFit
    loadAsset()
  }
  
  /// Get `gif` image frames (an array of images) from an asset by its name
  private class func getFramesFrom(asset: String) -> [UIImage] {
    guard let asset = getAsset(name: asset) else { return [] }
    return getFrames(from: asset.data)
  }
  
  /// Get `gif` image asset by its name. Will append `-dark` to the name when dark mode is enabled
  private class func getAsset(name: String) -> NSDataAsset? {
    switch UITraitCollection.current.userInterfaceStyle {
    case .light, .unspecified:
      return NSDataAsset(name: name, bundle: .module)
    case .dark:
      return NSDataAsset(name: [name, "dark"].joined(separator: "-"), bundle: .module)
    @unknown default:
      return NSDataAsset(name: name, bundle: .module)
    }
  }
  
  /// Get `gif` image frames (an array of images) from data
  class func getFrames(from data: Data) -> [UIImage] {
    guard let source = CGImageSourceCreateWithData(data as CFData, nil) else {
      return []
    }
    
    let count = CGImageSourceGetCount(source)
    let delays = (0..<count).map {
      // store in ms and truncate to compute GCD more easily
      Int(delayForImage(at: $0, source: source) * 1000)
    }
    
    let gcd = delays.reduce(0, absoluteGCDEuklidean)
    
    return (0..<count).flatMap { index -> [UIImage] in
      guard let cgImage = CGImageSourceCreateImageAtIndex(source, index, nil) else {
        return []
      }
      let frame = UIImage(cgImage: cgImage)
      let frameCount = delays[index] / gcd
      
      return (0..<frameCount).map { index2 -> UIImage in
        return frame
      }
    }
  }
  
  /// Return the greatest common divisor of two integers using recursive euklidean returning only an absolut value
  /// https://github.com/raywenderlich/swift-algorithm-club/blob/master/GCD/README.markdown
  private class func absoluteGCDEuklidean(_ firstValue: Int, _ secondValue: Int) -> Int {
    let secondAbsolute = abs(secondValue)
    let remainder = abs(firstValue) % secondAbsolute
    if remainder != 0 {
      return absoluteGCDEuklidean(secondAbsolute, remainder)
    } else {
      return secondAbsolute
    }
  }
  
  private class func delayForImage(at index: Int, source: CGImageSource) -> Double {
    let defaultDelay = 1.0
    let cfProperties = CGImageSourceCopyPropertiesAtIndex(source, index, nil)
    let gifPropertiesPointer = UnsafeMutablePointer<UnsafeRawPointer?>.allocate(capacity: 0)
    
    defer {
      gifPropertiesPointer.deallocate()
    }
    
    let unsafePointer = Unmanaged.passUnretained(kCGImagePropertyGIFDictionary).toOpaque()
    
    guard CFDictionaryGetValueIfPresent(cfProperties, unsafePointer, gifPropertiesPointer) != false else {
      return defaultDelay
    }
    
    let gifProperties = unsafeBitCast(gifPropertiesPointer.pointee, to: CFDictionary.self)
    var delayWrapper = unsafeBitCast(CFDictionaryGetValue(
      gifProperties,
      Unmanaged.passUnretained(kCGImagePropertyGIFUnclampedDelayTime).toOpaque()
    ), to: AnyObject.self)
    
    if delayWrapper.doubleValue == 0 {
      delayWrapper = unsafeBitCast(CFDictionaryGetValue(
        gifProperties,
        Unmanaged.passUnretained(kCGImagePropertyGIFDelayTime).toOpaque()
      ), to: AnyObject.self)
    }
    
    if let delay = delayWrapper as? Double, delay > 0 {
      return delay
    } else {
      return defaultDelay
    }
  }
}
