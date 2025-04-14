// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveShared
import Foundation

public protocol DownloadDelegate: AnyObject {
  func download(_ download: Download, didCompleteWithError error: Error?)
  func downloadDidUpgradeProgress(_ download: Download)
  func download(_ download: Download, didFinishDownloadingTo location: URL)
}

open class Download: NSObject {
  public weak var delegate: DownloadDelegate?

  public internal(set) var filename: String
  public internal(set) var mimeType: String
  public internal(set) var originalURL: URL?
  public internal(set) var destinationURL: URL?

  public internal(set) var isStarted: Bool = false
  public internal(set) var isComplete = false

  public internal(set) var totalBytesExpected: Int64?
  public internal(set) var bytesDownloaded: Int64

  init(
    suggestedFilename: String,
    originalURL: URL?,
    mimeType: String? = nil
  ) {

    // Strip out Unicode Bidi Control characters for RLO — Right-to-Left Override & LRO overrides
    self.filename = String(
      suggestedFilename.unicodeScalars.filter {
        !$0.properties.isBidiControl
      }.map(Character.init)
    )

    self.originalURL = originalURL
    self.mimeType = mimeType ?? "application/octet-stream"

    self.bytesDownloaded = 0

    super.init()
  }

  public func startDownloadToLocalFileAtPath(_ path: String) {
    if isStarted {
      return
    }
    isStarted = true
    let destination = URL(fileURLWithPath: path)
    self.destinationURL = destination
  }
  public func cancel() {}
  public func pause() {}
  public func resume() {}

  public static func uniqueDownloadPathForFilename(_ filename: String) async throws -> URL {
    let downloadsPath = try await AsyncFileManager.default.downloadsPath()
    let basePath = downloadsPath.appending(path: filename)
    let fileExtension = basePath.pathExtension
    let filenameWithoutExtension =
      !fileExtension.isEmpty ? String(filename.dropLast(fileExtension.count + 1)) : filename

    var proposedPath = basePath
    var count = 0

    while await AsyncFileManager.default.fileExists(atPath: proposedPath.path) {
      count += 1

      let proposedFilenameWithoutExtension = "\(filenameWithoutExtension) (\(count))"
      proposedPath = downloadsPath.appending(path: proposedFilenameWithoutExtension)
        .appendingPathExtension(fileExtension)
    }

    return proposedPath
  }

  // Used to avoid name spoofing using Unicode RTL char to change file extension
  public static func stripUnicode(fromFilename string: String) -> String {
    let validFilenameSet = CharacterSet(charactersIn: ":/")
      .union(.newlines)
      .union(.controlCharacters)
      .union(.illegalCharacters)
    return string.components(separatedBy: validFilenameSet).joined()
  }
}
