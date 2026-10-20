// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveTransferEngine
import Foundation
import ImageIO
import Photos
import UniformTypeIdentifiers

/// Saves received images and videos to Photos with add-only access, and
/// everything else, or media Photos refuses, to Files › Brave › Downloads
/// (design plan §4.3). Never reads the photo library.
public final class ReceivedItemFinalizer: ReceivedFileFinalizer {
  static let thumbnailPixelSize = 160

  private let files: DirectoryFinalizer

  /// `downloadsDirectory` is the app's Documents/Downloads, which Files shows
  /// as On My iPhone › Brave › Downloads.
  public init(downloadsDirectory: @escaping () throws -> URL) {
    files = DirectoryFinalizer(directory: downloadsDirectory)
  }

  public static var photosStatus: PermissionStatus {
    switch PHPhotoLibrary.authorizationStatus(for: .addOnly) {
    case .authorized: return .granted
    case .limited: return .limited
    case .denied, .restricted: return .denied
    case .notDetermined: return .notDetermined
    @unknown default: return .notDetermined
    }
  }

  public static func requestPhotos(_ completion: @escaping (PermissionStatus) -> Void) {
    PHPhotoLibrary.requestAuthorization(for: .addOnly) { _ in
      DispatchQueue.main.async { completion(photosStatus) }
    }
  }

  public func finalize(
    _ file: IncomingFile,
    temporaryURL: URL,
    completion: @escaping (SavedFile?) -> Void
  ) {
    guard MimeTypes.isImageOrVideo(file.mimeType) else {
      files.finalize(file, temporaryURL: temporaryURL, completion: completion)
      return
    }
    let isVideo = file.mimeType.hasPrefix("video/")
    let thumbnail = isVideo ? "" : Self.thumbnailDataURL(for: temporaryURL)
    let status = Self.photosStatus
    guard status == .granted || status == .limited else {
      saveToFiles(file, temporaryURL: temporaryURL, thumbnail: thumbnail, completion: completion)
      return
    }
    // Photos keys the resource type off the file extension.
    let named = temporaryURL.deletingLastPathComponent()
      .appendingPathComponent(UUID().uuidString)
      .appendingPathExtension((file.name as NSString).pathExtension)
    do {
      try FileManager.default.moveItem(at: temporaryURL, to: named)
    } catch {
      saveToFiles(file, temporaryURL: temporaryURL, thumbnail: thumbnail, completion: completion)
      return
    }
    PHPhotoLibrary.shared().performChanges {
      let request = PHAssetCreationRequest.forAsset()
      let options = PHAssetResourceCreationOptions()
      options.originalFilename = file.name
      request.addResource(with: isVideo ? .video : .photo, fileURL: named, options: options)
    } completionHandler: { [self] success, _ in
      if success {
        try? FileManager.default.removeItem(at: named)
        completion(
          SavedFile(url: nil, fileName: file.name, savedToPhotos: true, thumbnailURL: thumbnail)
        )
      } else {
        saveToFiles(file, temporaryURL: named, thumbnail: thumbnail, completion: completion)
      }
    }
  }

  private func saveToFiles(
    _ file: IncomingFile,
    temporaryURL: URL,
    thumbnail: String,
    completion: @escaping (SavedFile?) -> Void
  ) {
    files.finalize(file, temporaryURL: temporaryURL) { saved in
      guard var saved else {
        try? FileManager.default.removeItem(at: temporaryURL)
        completion(nil)
        return
      }
      saved.usedFallback = true
      saved.thumbnailURL = thumbnail
      completion(saved)
    }
  }

  /// A small JPEG thumbnail as a data: URL, made before the file is handed
  /// to Photos because add-only access cannot read it back.
  static func thumbnailDataURL(for url: URL) -> String {
    guard let source = CGImageSourceCreateWithURL(url as CFURL, nil) else { return "" }
    let options: [CFString: Any] = [
      kCGImageSourceCreateThumbnailFromImageAlways: true,
      kCGImageSourceCreateThumbnailWithTransform: true,
      kCGImageSourceThumbnailMaxPixelSize: thumbnailPixelSize,
    ]
    guard let image = CGImageSourceCreateThumbnailAtIndex(source, 0, options as CFDictionary)
    else {
      return ""
    }
    let data = NSMutableData()
    guard
      let destination = CGImageDestinationCreateWithData(
        data as CFMutableData,
        UTType.jpeg.identifier as CFString,
        1,
        nil
      )
    else {
      return ""
    }
    CGImageDestinationAddImage(
      destination,
      image,
      [kCGImageDestinationLossyCompressionQuality: 0.7] as CFDictionary
    )
    guard CGImageDestinationFinalize(destination) else { return "" }
    return "data:image/jpeg;base64," + (data as Data).base64EncodedString()
  }
}
