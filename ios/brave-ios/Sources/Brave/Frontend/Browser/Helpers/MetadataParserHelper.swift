// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import Shared
import Storage
import WebKit
import os.log

class MetadataParserHelper: TabEventHandler {
  private var tabObservers: TabObservers!

  init() {
    self.tabObservers = registerFor(
      .didChangeURL,
      queue: .main
    )
  }

  deinit {
    unregister(tabObservers)
  }

  func tab(_ tab: Tab, didChangeURL url: URL) {
    // Get the metadata out of the page-metadata-parser, and into a type safe struct as soon
    // as possible.
    guard let webView = tab.webView,
      let url = webView.lastCommittedURL, url.isWebPage(includeDataURIs: false),
      !InternalURL.isValid(url: url)
    else {
      // TabEvent.post(.pageMetadataNotAvailable, for: tab)
      tab.pageMetadata = nil
      return
    }

    webView.evaluateSafeJavaScript(
      functionName: "__firefox__.metadata && __firefox__.metadata.getMetadata()",
      contentWorld: .defaultClient,
      asFunction: false
    ) { (result, error) in
      guard error == nil else {
        // TabEvent.post(.pageMetadataNotAvailable, for: tab)
        tab.pageMetadata = nil
        return
      }

      guard let dict = result as? [String: Any],
        let data = try? JSONSerialization.data(withJSONObject: dict, options: [])
      else {
        Logger.module.debug("Page contains no metadata!")
        //                TabEvent.post(.pageMetadataNotAvailable, for: tab)
        tab.pageMetadata = nil
        return
      }

      do {
        let pageMetadata = try JSONDecoder().decode(PageMetadata.self, from: data)
        tab.pageMetadata = pageMetadata
        TabEvent.post(.didLoadPageMetadata(pageMetadata), for: tab)
      } catch {
        Logger.module.error(
          "Failed to parse metadata: \(error.localizedDescription, privacy: .public)"
        )
        // To avoid issues where `pageMetadata` points to the last website to successfully
        // parse metadata, set to nil
        tab.pageMetadata = nil
      }
    }
  }
}

class MediaImageLoader: TabEventHandler {
  private var tabObservers: TabObservers!
  private let prefs: Prefs

  init(_ prefs: Prefs) {
    self.prefs = prefs
    self.tabObservers = registerFor(
      .didLoadPageMetadata,
      queue: .main
    )
  }

  deinit {
    unregister(tabObservers)
  }

  func tab(_ tab: Tab, didLoadPageMetadata metadata: PageMetadata) {
    if let mediaURL = metadata.mediaURL, let url = URL(string: mediaURL) {
      loadImage(from: url)
    }
  }

  fileprivate func loadImage(from url: URL) {
    WebImageCacheManager.shared.load(from: url)
  }

}
