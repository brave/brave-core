// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import MobileCoreServices
import UIKit
import UniformTypeIdentifiers

class ActionToBraveViewController: UIViewController {

  private enum SchemeType {
    case url, query
  }

  override func viewWillAppear(_ animated: Bool) {
    super.viewWillAppear(animated)

    // Get the item[s] for handling from the extension context
    for item in extensionContext?.inputItems as? [NSExtensionItem] ?? [] {
      for provider in item.attachments ?? [] {

        // Opening browser with site
        if provider.hasItemConformingToTypeIdentifier(UTType.url.identifier) {
          loadAttachmentFor(type: .url, using: provider)
          break
        } else {
          // Opening browser with search url
          loadAttachmentFor(type: .query, using: provider)
          break
        }
      }
    }
  }

  func done() {
    // Return any edited content to the host app, in this case empty
    extensionContext?.completeRequest(returningItems: [], completionHandler: nil)
  }

  private func loadAttachmentFor(type: SchemeType, using provider: NSItemProvider) {
    let typeIdentifier: String = type == .query ? UTType.text.identifier : UTType.url.identifier

    provider.loadItem(forTypeIdentifier: typeIdentifier) { item, error in
      DispatchQueue.main.async {

        guard let schemeUrl = self.constructSchemeURL(for: type, with: item) else {
          self.done()
          return
        }

        self.openBrowser(with: schemeUrl)
      }
    }
  }

  private func constructSchemeURL(for schemeType: SchemeType, with item: NSSecureCoding?) -> URL? {
    switch schemeType {
    case .query:
      guard let item = item as? String else {
        return nil
      }

      return createURL(for: .query, with: item)

    case .url:
      // The first URL found within item url absolute string
      guard let item = (item as? URL)?.absoluteString,
        let detector = try? NSDataDetector(types: NSTextCheckingResult.CheckingType.link.rawValue),
        let match = detector.firstMatch(
          in: item,
          options: [],
          range: NSRange(location: 0, length: item.count)
        ),
        let range = Range(match.range, in: item),
        let queryURL = URL(string: String(item[range]))?.absoluteString
      else {
        return nil
      }

      return createURL(for: .url, with: queryURL)
    }

  }

  private func createURL(for schemeType: SchemeType, with value: String) -> URL? {
    var queryItem: URLQueryItem
    var components = URLComponents()
    components.scheme = Bundle.main.infoDictionary?["BRAVE_URL_SCHEME"] as? String ?? "brave"

    switch schemeType {
    case .query:
      queryItem = URLQueryItem(name: "q", value: value)
      components.host = "search"
    case .url:
      queryItem = URLQueryItem(name: "url", value: value)
      components.host = "open-url"
    }

    components.queryItems = [queryItem]
    return components.url
  }

  private func openBrowser(with url: URL) {
    var responder = self as UIResponder?

    while let currentResponder = responder {
      if let application = currentResponder as? UIApplication {
        DispatchQueue.main.async {
          application.open(url, options: [:], completionHandler: nil)
        }
      }
      responder = currentResponder.next
    }

    DispatchQueue.main.asyncAfter(deadline: .now() + 0.1) {
      self.done()
    }
  }
}
