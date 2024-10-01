// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import MobileCoreServices
import Social
import UIKit
import UniformTypeIdentifiers

extension String {
  /// The first URL found within this String, or nil if no URL is found
  var firstURL: URL? {
    if let detector = try? NSDataDetector(types: NSTextCheckingResult.CheckingType.link.rawValue),
      let match = detector.firstMatch(
        in: self,
        options: [],
        range: NSRange(location: 0, length: self.count)
      ),
      let range = Range(match.range, in: self)
    {
      return URL(string: String(self[range]))
    }
    return nil
  }
}

class ShareToBraveViewController: SLComposeServiceViewController {
  private struct Scheme {
    private enum SchemeType {
      case url, query
    }

    private let type: SchemeType
    private let urlOrQuery: String

    init?(item: NSSecureCoding) {
      if let text = item as? String {
        urlOrQuery = text
        type = .query
      } else if let url = (item as? URL)?.absoluteString.firstURL?.absoluteString {
        urlOrQuery = url
        type = .url
      } else {
        return nil
      }
    }

    var schemeUrl: URL? {
      var components = URLComponents()
      let queryItem: URLQueryItem

      components.scheme = Bundle.main.infoDictionary?["BRAVE_URL_SCHEME"] as? String ?? "brave"

      switch type {
      case .url:
        components.host = "open-url"
        queryItem = URLQueryItem(name: "url", value: urlOrQuery)
      case .query:
        components.host = "search"
        queryItem = URLQueryItem(name: "q", value: urlOrQuery)
      }

      components.queryItems = [queryItem]
      return components.url
    }
  }

  // TODO: Separate scheme for debug builds, so it can be tested without need to uninstall production app.

  override func configurationItems() -> [Any]! {
    guard let inputItems = extensionContext?.inputItems as? [NSExtensionItem] else {
      return []
    }

    // Reduce all input items down to a single list of item providers
    let attachments: [NSItemProvider] =
      inputItems
      .compactMap { $0.attachments }
      .flatMap { $0 }

    // Look for the first URL the host application is sharing.
    // If there isn't a URL grab the first text item
    guard
      let provider = attachments.first(where: { $0.isUrl })
        ?? attachments.first(where: { $0.isText })
    else {
      // If no item was processed. Cancel the share action to prevent the extension from locking the host application
      // due to the hidden ViewController.
      cancel()
      return []
    }

    provider.loadItem(
      forTypeIdentifier: provider.isUrl ? UTType.url.identifier : UTType.text.identifier
    ) { item, error in
      DispatchQueue.main.async {
        guard let item = item, let schemeUrl = Scheme(item: item)?.schemeUrl else {
          self.cancel()
          return
        }

        self.handleUrl(schemeUrl)
      }
    }

    return []
  }

  private func handleUrl(_ url: URL) {
    // From http://stackoverflow.com/questions/24297273/openurl-not-work-in-action-extension
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
      self.cancel()
    }
  }

  override func viewDidAppear(_ animated: Bool) {
    // Stop keyboard from showing
    textView.resignFirstResponder()
    textView.isEditable = false

    super.viewDidAppear(animated)
  }

  override func willMove(toParent parent: UIViewController?) {
    view.alpha = 0
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
