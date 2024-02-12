/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Shared
import UIKit
import os.log

private class DiskImageStoreErrorType: Error {
  let description: String
  init(description: String) {
    self.description = description
  }
}

/**
 * Disk-backed key-value image store.
 */
open class DiskImageStore {
  fileprivate let files: FileAccessor
  fileprivate let filesDir: String
  fileprivate let queue = DispatchQueue(label: "DiskImageStore")
  fileprivate let quality: CGFloat
  fileprivate var keys: Set<String>

  required public init(files: FileAccessor, namespace: String, quality: Float) throws {
    self.files = files
    self.filesDir = try files.getAndEnsureDirectory(namespace)
    self.quality = CGFloat(quality)

    // Build an in-memory set of keys from the existing images on disk.
    var keys = [String]()
    if let fileEnumerator = FileManager.default.enumerator(atPath: filesDir) {
      for file in fileEnumerator {
        keys.append(file as! String)
      }
    }
    self.keys = Set(keys)
  }

  /// Gets an image for the given key if it is in the store.
  open func get(_ key: String) async throws -> UIImage {
    return try await withCheckedThrowingContinuation { continuation in
      queue.async {
        if !self.keys.contains(key) {
          continuation.resume(throwing: DiskImageStoreErrorType(description: "Image key not found"))
          return
        }
        let imagePath = URL(fileURLWithPath: self.filesDir).appendingPathComponent(key)
        if let data = try? Data(contentsOf: imagePath),
           let image = UIImage.imageFromDataThreadSafe(data) {
          continuation.resume(returning: image)
          return
        }
        
        continuation.resume(throwing: DiskImageStoreErrorType(description: "Invalid image data"))
      }
    }
  }
  
  /// Gets an image for the given key if it is in the store.
  open func getSynchronously(_ key: String) throws -> UIImage {
    try queue.sync {
      if !self.keys.contains(key) {
        throw DiskImageStoreErrorType(description: "Image key not found")
      }
      let imagePath = URL(fileURLWithPath: self.filesDir).appendingPathComponent(key)
      if let data = try? Data(contentsOf: imagePath),
         let image = UIImage.imageFromDataThreadSafe(data) {
        return image
      }
      
      throw DiskImageStoreErrorType(description: "Invalid image data")
    }
  }

  /// Adds an image for the given key.
  /// This put is asynchronous; the image is not recorded in the cache until the write completes.
  /// Does nothing if this key already exists in the store.
  open func put(_ key: String, image: UIImage) async throws {
    return try await withCheckedThrowingContinuation { continuation in
      queue.async {
        if self.keys.contains(key) {
          continuation.resume(throwing: DiskImageStoreErrorType(description: "Key already in store"))
          return
        }
        
        let imageURL = URL(fileURLWithPath: self.filesDir).appendingPathComponent(key)
        if let data = image.jpegData(compressionQuality: self.quality) {
          do {
            try data.write(to: imageURL, options: .noFileProtection)
            self.keys.insert(key)
            continuation.resume()
            return
          } catch {
            Logger.module.error("Unable to write image to disk: \(error.localizedDescription)")
          }
        }
        
        continuation.resume(throwing: DiskImageStoreErrorType(description: "Could not write image to file"))
      }
    }
  }

  /// Clears all images from the cache, excluding the given set of keys.
  open func clearExcluding(_ keys: Set<String>) async {
    return await withCheckedContinuation { continuation in
      queue.async {
        let keysToDelete = self.keys.subtracting(keys)
        
        for key in keysToDelete {
          let url = URL(fileURLWithPath: self.filesDir).appendingPathComponent(key)
          do {
            try FileManager.default.removeItem(at: url)
          } catch {
            Logger.module.warning("Failed to remove DiskImageStore item at \(url.absoluteString): \(error.localizedDescription)")
          }
        }
        
        self.keys = self.keys.intersection(keys)
        continuation.resume()
      }
    }
  }
}
