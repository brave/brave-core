// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import LinkPresentation
import UIKit
import UniformTypeIdentifiers

class URLActivityItemProvider: NSObject, UIActivityItemSource {
  let title: String
  let url: URL
  private let linkMetadata: LPLinkMetadata?

  init(title: String, url: URL, includeLocalMetadata: Bool = false) {
    self.title = title
    self.url = url
    if includeLocalMetadata {
      let metadata = LPLinkMetadata()
      metadata.title = title
      metadata.originalURL = url
      metadata.url = url
      self.linkMetadata = metadata
    } else {
      self.linkMetadata = nil
    }
    super.init()
  }

  func activityViewControllerLinkMetadata(_ activityViewController: UIActivityViewController) -> LPLinkMetadata? {
    linkMetadata
  }

  func activityViewControllerPlaceholderItem(
    _ activityViewController: UIActivityViewController
  ) -> Any {
    return url
  }

  func activityViewController(
    _ activityViewController: UIActivityViewController,
    itemForActivityType activityType: UIActivity.ActivityType?
  ) -> Any? {
    return url
  }

  func activityViewController(
    _ activityViewController: UIActivityViewController,
    subjectForActivityType activityType: UIActivity.ActivityType?
  ) -> String {
    return title
  }

  func activityViewController(
    _ activityViewController: UIActivityViewController,
    dataTypeIdentifierForActivityType activityType: UIActivity.ActivityType?
  ) -> String {
    return UTType.url.identifier
  }
}
