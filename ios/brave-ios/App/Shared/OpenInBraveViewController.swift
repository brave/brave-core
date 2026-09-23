// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import MobileCoreServices
import UIKit
import UniformTypeIdentifiers

/// A shared view controller used for both the share extension and action extension
class OpenInBraveViewController: UIViewController {

  private enum SchemeType {
    case url, query
  }

  override func viewWillAppear(_ animated: Bool) {
    super.viewWillAppear(animated)

    // Reduce all input items down to a single list of item providers
    let attachments: [NSItemProvider] =
      (extensionContext?.inputItems as? [NSExtensionItem] ?? [])
      .compactMap { $0.attachments }
      .flatMap { $0 }

    // Look for the first URL the host application is sharing.
    // If there isn't a URL grab the first text item
    guard
      let provider = attachments.first(where: { $0.isUrl })
        ?? attachments.first(where: { $0.isText })
    else {
      done()
      return
    }

    provider.loadItem(
      forTypeIdentifier: provider.isUrl ? UTType.url.identifier : UTType.text.identifier
    ) { item, error in
      DispatchQueue.main.async {
        guard
          let schemeUrl = self.constructSchemeURL(for: provider.isUrl ? .url : .query, with: item)
        else {
          self.done()
          return
        }

        self.openBrowser(with: schemeUrl)
      }
    }
  }

  func done() {
    // Return any edited content to the host app, in this case empty
    extensionContext?.completeRequest(returningItems: [], completionHandler: nil)
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

extension NSItemProvider {
  var isText: Bool {
    return hasItemConformingToTypeIdentifier(UTType.text.identifier)
  }

  var isUrl: Bool {
    return hasItemConformingToTypeIdentifier(UTType.url.identifier)
  }
}
