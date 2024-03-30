// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveUI

extension BraveCore.WebImageDownloader: WebImageDownloaderType {
  @MainActor public func downloadImage(url: URL) async -> UIImage? {
    guard url.isWebPage() else { return nil }
    return await withCheckedContinuation { continuation in
      downloadImage(url) { image, _, _ in
        continuation.resume(returning: image)
      }
    }
  }

  @MainActor public func imageFromData(data: Data) async -> UIImage? {
    WebImageDownloader.image(from: data)
  }
}
