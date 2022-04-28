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

private let log = Logger.browserLogger

public class PlaylistThumbnailRenderer {
  private let timeout: TimeInterval = 3
  private var hlsGenerator: HLSThumbnailGenerator?
  private var assetGenerator: AVAssetImageGenerator?
  private var favIconGenerator: FavIconImageRenderer?
  private var thumbnailGenerator = Set<AnyCancellable>()

  func loadThumbnail(assetUrl: URL?, favIconUrl: URL?, completion: @escaping (UIImage?) -> Void) {
    if let assetUrl = assetUrl, let cachedImage = SDImageCache.shared.imageFromCache(forKey: assetUrl.absoluteString) {
      self.destroy()
      completion(cachedImage)
    } else {
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
            log.error(error)
            completion(nil)
          }
        },
        receiveValue: {
          completion($0)
        }
      ).store(in: &thumbnailGenerator)
    }
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

  private func bind(_ block: @escaping (URL, @escaping (UIImage?) -> Void) -> Void, url: URL) -> Future<UIImage, Error> {
    Future { promise in
      block(url, { image in
        if let image = image {
          promise(.success(image))
        } else {
          promise(.failure("Image could not be loaded"))
        }
      })
    }
  }

  private func loadHLSThumbnail(url: URL, completion: @escaping (UIImage?) -> Void) {
    hlsGenerator = HLSThumbnailGenerator(
      url: url, time: timeout,
      completion: { image, error in
        if let error = error {
          log.error(error)
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
        log.error(error)
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
    favIconGenerator = FavIconImageRenderer()
    favIconGenerator?.loadIcon(siteURL: url, persistent: !PrivateBrowsingManager.shared.isPrivateBrowsing) { icon in
      DispatchQueue.main.async {
        completion(icon)
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
  private let completion: (UIImage?, Error?) -> Void
  private let queue = DispatchQueue(label: "com.brave.hls-thumbnail-generator")

  init(url: URL, time: TimeInterval, completion: @escaping (UIImage?, Error?) -> Void) {
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
          self.completion(nil, "Failed to load item")
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
                self.completion(nil, "Cannot copy pixel-buffer (PBO)")
              }
            }
          }
        } else {
          DispatchQueue.main.async {
            self.completion(nil, "Failed to seek to specified time")
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
        self.completion(nil, "Failed to create image from pixel-buffer frame.")
      }
    }
  }

}

// MARK: - FavIconImageRenderer

/// A class for rendering a FavIcon onto a `UIImage`
class FavIconImageRenderer {
  private var task: DispatchWorkItem?

  deinit {
    task?.cancel()
  }

  func loadIcon(siteURL: URL, persistent: Bool, completion: ((UIImage?) -> Void)?) {
    task?.cancel()
    task = DispatchWorkItem {
      let domain = Domain.getOrCreate(forUrl: siteURL, persistent: persistent)
      var faviconFetcher: FaviconFetcher? = FaviconFetcher(siteURL: siteURL, kind: .favicon, domain: domain)
      faviconFetcher?.load() { [weak self] _, attributes in
        faviconFetcher = nil

        guard let self = self,
          let cancellable = self.task,
          !cancellable.isCancelled
        else {
          completion?(nil)
          return
        }

        if let image = attributes.image {
          if let backgroundColor = attributes.backgroundColor,
            let cgImage = image.cgImage {
            // attributes.includesPadding sometimes returns 0 for icons that should. It's better this way to always include the padding.
            let padding = 4.0
            let size = CGSize(
              width: image.size.width + padding,
              height: image.size.height + padding)

            let finalImage = self.renderOnImageContext(size: size) { context, rect in
              context.saveGState()
              context.setFillColor(backgroundColor.cgColor)
              context.fill(rect)

              context.translateBy(x: 0.0, y: rect.size.height)
              context.scaleBy(x: 1.0, y: -1.0)

              context.draw(cgImage, in: rect.insetBy(dx: padding, dy: padding))
              context.restoreGState()
            }
            completion?(finalImage)
          } else {
            completion?(image)
          }
        } else {
          // Monogram favicon attributes
          let label = UILabel().then {
            $0.textColor = .white
            $0.backgroundColor = .clear
            $0.minimumScaleFactor = 0.5
          }

          let text =
            FaviconFetcher.monogramLetter(
              for: siteURL,
              fallbackCharacter: nil
            ) as NSString

          let padding = 4.0
          let finalImage = self.renderOnImageContext { context, rect in
            guard let font = label.font else { return }
            var fontSize = font.pointSize

            // Estimate the size of the font required to fit the context's bounds + padding
            // Usually we can do this by iterating and calculating the size that fits
            // But this is a very good estimated size
            let newSize = text.size(withAttributes: [.font: font.withSize(fontSize)])
            guard newSize.width > 0.0 && newSize.height > 0.0 else { return }

            let ratio = min(
              (rect.size.width - padding) / newSize.width,
              (rect.size.height - padding) / newSize.height)
            fontSize *= ratio

            if fontSize < label.font.pointSize * 0.5 {
              fontSize = label.font.pointSize * 0.5
            }

            if let backgroundColor = attributes.backgroundColor?.cgColor {
              context.setFillColor(backgroundColor)
              context.fill(rect)
            }

            let newFont = font.withSize(fontSize)
            let size = text.size(withAttributes: [.font: newFont])

            // Center the text drawing in the CGContext
            let x = (rect.size.width - size.width) / 2.0
            let y = (rect.size.height - size.height) / 2.0

            text.draw(
              in: rect.insetBy(dx: x, dy: y),
              withAttributes: [
                .font: newFont,
                .foregroundColor: UIColor.white,
              ])
          }

          completion?(finalImage)
        }
      }
    }

    if let task = task {
      DispatchQueue.main.async(execute: task)
    }
  }

  private func renderOnImageContext(size: CGSize, _ draw: (CGContext, CGRect) -> Void) -> UIImage? {
    let size = CGSize(width: size.width, height: size.height)
    UIGraphicsBeginImageContextWithOptions(size, false, UIScreen.main.scale)
    draw(UIGraphicsGetCurrentContext()!, CGRect(size: size))
    let img = UIGraphicsGetImageFromCurrentImageContext()
    UIGraphicsEndImageContext()
    return img
  }

  private func renderOnImageContext(_ draw: (CGContext, CGRect) -> Void) -> UIImage? {
    renderOnImageContext(size: CGSize(width: 16.0, height: 16.0), draw)
  }
}
