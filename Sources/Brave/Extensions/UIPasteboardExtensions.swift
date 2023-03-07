/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import MobileCoreServices
import Shared
import UIKit
import UniformTypeIdentifiers

extension UIPasteboard {
  fileprivate func imageTypeKey(_ isGIF: Bool) -> String {
    return (isGIF ? UTType.gif : UTType.png).identifier
  }

  private var syncURL: URL? {
    return UIPasteboard.general.string.flatMap {
      guard let url = URL(string: $0), url.isWebPage() else { return nil }
      return url
    }
  }
  
  func asyncURL() async -> URL? {
    return await Task.detached {
      return self.syncURL
    }.value
  }

  @objc func clearPasteboard() {
    UIPasteboard.general.string = ""
  }
}
