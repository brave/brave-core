// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import AVFoundation
import CoreImage
import Combine
import SDWebImage
import Shared
import Data
import Favicon
import os.log

public class PlaylistThumbnailRenderer {
  private let timeout: TimeInterval = 3
  private var hlsGenerator: HLSThumbnailGenerator?
  private var assetGenerator: AVAssetImageGenerator?
  private var favIconGenerator: Task<Void, Error>?
  private var thumbnailGenerator = Set<AnyCancellable>()

  func loadThumbnail(assetUrl: URL?, favIconUrl: URL?, completion: @escaping (UIImage?) -> Void) {
    if let assetUrl = assetUrl, let cachedImage = SDImageCache.shared.imageFromCache(forKey: assetUrl.absoluteString) {
      self.destroy()
      completion(cachedImage)
      return
    }
    
    if let favIconUrl = favIconUrl, let cachedImage = SDImageCache.shared.imageFromCache(forKey: favIconUrl.absoluteString) {
      self.destroy()
      completion(cachedImage)
      return
    }
    
    var generators = [Future<UIImage, Error>]()
    if let assetUrl = assetUrl {
      generators.append(contentsOf: [
        bind(loadHLSThumbnail, url: assetUrl),
        bind(loadAssetThumbnail, url: assetUrl),
      ])
    }

    if let favIconUrl = favIconUrl {
      generators.append(bind(loadFavIconThumbnail, url: favIconUrl))
    }

    var chainedGenerator = generators.removeFirst().eraseToAnyPublisher()
    for generator in generators {
      chainedGenerator = chainedGenerator.catch { _ in
        generator
      }.eraseToAnyPublisher()
    }

    chainedGenerator.receive(on: RunLoop.main).sink(
      receiveCompletion: {
        if case .failure(let error) = $0 {
          Logger.module.error("\(error.localizedDescription)")
          completion(nil)
        }
      },
      receiveValue: {
        completion($0)
      }
    ).store(in: &thumbnailGenerator)
  }

  func cancel() {
    self.destroy()
  }

  deinit {
    destroy()
  }

  private func destroy() {
    thumbnailGenerator.forEach({ $0.cancel() })
    thumbnailGenerator = Set()

    hlsGenerator = nil
    assetGenerator = nil
    favIconGenerator = nil
  }
  
  private enum BindError: Error {
    case failedToLoadImage
  }

  private func bind(_ block: @escaping (URL, @escaping (UIImage?) -> Void) -> Void, url: URL) -> Future<UIImage, Error> {
    Future { promise in
      block(url, { image in
        if let image = image {
          promise(.success(image))
        } else {
          promise(.failure(BindError.failedToLoadImage))
        }
      })
    }
  }

  private func loadHLSThumbnail(url: URL, completion: @escaping (UIImage?) -> Void) {
    hlsGenerator = HLSThumbnailGenerator(
      url: url, time: timeout,
      completion: { image, error in
        if let error = error {
          Logger.module.error("\(error.localizedDescription)")
        }

        if let image = image {
          SDImageCache.shared.store(image, forKey: url.absoluteString, completion: nil)
        }

        DispatchQueue.main.async {
          completion(image)
        }
      })
  }

  private func loadAssetThumbnail(url: URL, completion: @escaping (UIImage?) -> Void) {
    let time = CMTimeMakeWithSeconds(timeout, preferredTimescale: 1)
    assetGenerator = AVAssetImageGenerator(asset: AVAsset(url: url))
    assetGenerator?.appliesPreferredTrackTransform = false
    assetGenerator?.generateCGImagesAsynchronously(forTimes: [NSValue(time: time)]) { _, cgImage, _, result, error in
      if let error = error {
        Logger.module.error("\(error.localizedDescription)")
      }

      if result == .succeeded, let cgImage = cgImage {
        let image = UIImage(cgImage: cgImage)
        SDImageCache.shared.store(image, forKey: url.absoluteString, completion: nil)

        DispatchQueue.main.async {
          completion(image)
        }
      } else {
        DispatchQueue.main.async {
          completion(nil)
        }
      }
    }
  }

  private func loadFavIconThumbnail(url: URL, completion: @escaping (UIImage?) -> Void) {
    favIconGenerator?.cancel()
    if let favicon = FaviconFetcher.getIconFromCache(for: url) {
      SDImageCache.shared.store(favicon.image, forKey: url.absoluteString, completion: nil)
      completion(favicon.image)
      return
    }
    
    favIconGenerator = Task { @MainActor in
      do {
        let favicon = try await FaviconFetcher.loadIcon(url: url, persistent: true)
        await SDImageCache.shared.store(favicon.image, forKey: url.absoluteString)
        completion(favicon.image)
      } catch {
        completion(nil)
      }
    }
  }
}

// MARK: - HLSThumbnailGenerator

/// A class for generating Thumbnails from HLS Streams
private class HLSThumbnailGenerator {
  private enum State {
    case loading
    case ready
    case failed
  }

  private let asset: AVAsset
  private let sourceURL: URL
  private var player: AVPlayer?
  private var currentItem: AVPlayerItem?
  private var videoOutput: AVPlayerItemVideoOutput?
  private var observer: NSKeyValueObservation?
  private var state: State = .loading
  private let completion: (UIImage?, HLSThumbnailGeneratorError?) -> Void
  private let queue = DispatchQueue(label: "com.brave.hls-thumbnail-generator")

  init(url: URL, time: TimeInterval, completion: @escaping (UIImage?, HLSThumbnailGeneratorError?) -> Void) {
    self.asset = AVAsset(url: url)
    self.sourceURL = url
    self.completion = completion

    let item = AVPlayerItem(asset: asset, automaticallyLoadedAssetKeys: nil)
    self.player = AVPlayer(playerItem: item).then {
      $0.rate = 0
    }

    self.currentItem = player?.currentItem
    self.videoOutput = AVPlayerItemVideoOutput(pixelBufferAttributes: [
      kCVPixelBufferPixelFormatTypeKey as String: kCVPixelFormatType_32BGRA
    ])

    self.observer = self.currentItem?.observe(\.status) { [weak self] item, _ in
      guard let self = self else { return }

      if item.status == .readyToPlay && self.state == .loading {
        self.state = .ready
        self.generateThumbnail(at: time)
      } else if item.status == .failed {
        self.state = .failed
        DispatchQueue.main.async {
          self.completion(nil, .cannotLoadItem(url: url))
        }
      }
    }

    if let videoOutput = self.videoOutput {
      self.player?.currentItem?.add(videoOutput)
    }
  }

  deinit {
    // Must call in this order.
    // Bug in observers in iOS where the item can be deinit before the observer
    // In such a case, it wrongly throws an exception
    // KVO_IS_RETAINING_ALL_OBSERVERS_OF_THIS_OBJECT_IF_IT_CRASHES_AN_OBSERVER_WAS_OVERRELEASED_OR_SMASHED
    // This happens even with block-based observers
    // So we must call invalidate FIRST, then release the object being observed
    // (Aka reverse stack order release)
    if let videoOutput = videoOutput {
      currentItem?.remove(videoOutput)
    }

    videoOutput = nil
    observer?.invalidate()
    observer = nil
    currentItem = nil
    player = nil
  }

  private func generateThumbnail(at time: TimeInterval) {
    queue.async {
      let time = CMTimeMakeWithSeconds(time, preferredTimescale: 1)
      self.player?.seek(to: time) { [weak self] finished in
        guard let self = self else { return }

        if finished {
          self.queue.async {
            if let buffer = self.videoOutput?.copyPixelBuffer(forItemTime: time, itemTimeForDisplay: nil) {
              self.snapshotPixelBuffer(buffer, atTime: time.seconds)
            } else {
              DispatchQueue.main.async {
                self.completion(nil, .cannotCopyPixelBuffer)
              }
            }
          }
        } else {
          DispatchQueue.main.async {
            self.completion(nil, .cannotSeekToSpecifiedTimeInterval(interval: time))
          }
        }
      }
    }
  }

  private func snapshotPixelBuffer(_ buffer: CVPixelBuffer, atTime time: TimeInterval) {
    let ciImage = CIImage(cvPixelBuffer: buffer)
    let quartzFrame = CGRect(
      x: 0, y: 0,
      width: CVPixelBufferGetWidth(buffer),
      height: CVPixelBufferGetHeight(buffer))

    if let cgImage = CIContext().createCGImage(ciImage, from: quartzFrame) {
      let result = UIImage(cgImage: cgImage)

      DispatchQueue.main.async {
        self.completion(result, nil)
      }
    } else {
      DispatchQueue.main.async {
        self.completion(nil, .invalidPixelBuffer)
      }
    }
  }

}

public enum HLSThumbnailGeneratorError: Error {
  case cannotLoadItem(url: URL)
  case cannotCopyPixelBuffer
  case cannotSeekToSpecifiedTimeInterval(interval: CMTime)
  case invalidPixelBuffer
}
