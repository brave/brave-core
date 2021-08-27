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

private let log = Logger.browserLogger

public class PlaylistThumbnailRenderer {
    private let timeout: TimeInterval = 3
    private var hlsGenerator: HLSThumbnailGenerator?
    private var assetGenerator: AVAssetImageGenerator?
    private var favIconGenerator: FavIconImageRenderer?
    private var thumbnailGenerator = Set<AnyCancellable>()
    
    func loadThumbnail(assetUrl: URL, favIconUrl: URL, completion: @escaping (UIImage?) -> Void) {
        if let cachedImage = SDImageCache.shared.imageFromCache(forKey: assetUrl.absoluteString) {
            self.destroy()
            completion(cachedImage)
        } else {
            var generators = [bind(loadHLSThumbnail, url: assetUrl),
                              bind(loadAssetThumbnail, url: assetUrl),
                              bind(loadFavIconThumbnail, url: favIconUrl)]
            
            var chainedGenerator = generators.removeFirst().eraseToAnyPublisher()
            for generator in generators {
                chainedGenerator = chainedGenerator.catch { _ in
                    generator
                }.eraseToAnyPublisher()
            }
            
            chainedGenerator.receive(on: RunLoop.main).sink(receiveCompletion: {
                if case .failure(let error) = $0 {
                    log.error(error)
                    completion(nil)
                }
            }, receiveValue: {
                completion($0)
            }).store(in: &thumbnailGenerator)
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
        hlsGenerator = HLSThumbnailGenerator(url: url, time: timeout, completion: { image, error in
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
        let time = CMTime(seconds: timeout, preferredTimescale: CMTimeScale(1))
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
        favIconGenerator?.loadIcon(siteURL: url) { icon in
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
    private let player: AVPlayer?
    private let videoOutput: AVPlayerItemVideoOutput?
    private var observer: NSKeyValueObservation?
    private var state: State = .loading
    private let queue = DispatchQueue(label: "com.brave.hls-thumbnail-generator")
    private let completion: (UIImage?, Error?) -> Void

    init(url: URL, time: TimeInterval, completion: @escaping (UIImage?, Error?) -> Void) {
        self.asset = AVAsset(url: url)
        self.sourceURL = url
        self.completion = completion

        let item = AVPlayerItem(asset: asset, automaticallyLoadedAssetKeys: [])
        self.player = AVPlayer(playerItem: item).then {
            $0.rate = 0
        }
        
        self.videoOutput = AVPlayerItemVideoOutput(pixelBufferAttributes: [
            kCVPixelBufferPixelFormatTypeKey as String: kCVPixelFormatType_32BGRA
        ])
        
        self.observer = self.player?.currentItem?.observe(\.status) { [weak self] item, _ in
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

    private func generateThumbnail(at time: TimeInterval) {
        queue.async {
            let time = CMTime(seconds: time, preferredTimescale: 1)
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
        let quartzFrame = CGRect(x: 0, y: 0,
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
private class FavIconImageRenderer {
    private var task: DispatchWorkItem?
    
    deinit {
        task?.cancel()
    }

    func loadIcon(siteURL: URL, completion: ((UIImage?) -> Void)?) {
        task?.cancel()
        task = DispatchWorkItem {
            let faviconFetcher: FaviconFetcher? = FaviconFetcher(siteURL: siteURL, kind: .favicon, domain: nil)
            faviconFetcher?.load() { [weak self] _, attributes in
                guard let self = self,
                      let cancellable = self.task,
                      !cancellable.isCancelled  else {
                    completion?(nil)
                    return
                }
                
                if let image = attributes.image {
                    let finalImage = self.renderOnImageContext { context, rect in
                        if let backgroundColor = attributes.backgroundColor {
                            context.setFillColor(backgroundColor.cgColor)
                        }
                        
                        if let image = image.cgImage {
                            context.draw(image, in: rect)
                        }
                    }
                    
                    completion?(finalImage)
                } else {
                    // Monogram favicon attributes
                    let label = UILabel().then {
                        $0.textColor = .white
                        $0.backgroundColor = .clear
                        $0.minimumScaleFactor = 0.5
                    }
                    
                    label.text = FaviconFetcher.monogramLetter(
                        for: siteURL,
                        fallbackCharacter: nil
                    )
                    
                    let finalImage = self.renderOnImageContext { context, _ in
                        label.layer.render(in: context)
                    }
                    
                    completion?(finalImage)
                }
            }
        }
    }
    
    private func renderOnImageContext(_ draw: (CGContext, CGRect) -> Void) -> UIImage? {
        let size = CGSize(width: 100.0, height: 100.0)
        UIGraphicsBeginImageContextWithOptions(size, false, 0.0)
        draw(UIGraphicsGetCurrentContext()!, CGRect(size: size))
        let img = UIGraphicsGetImageFromCurrentImageContext()
        UIGraphicsEndImageContext()
        return img
    }
}
