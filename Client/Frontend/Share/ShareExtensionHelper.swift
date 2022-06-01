/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import Shared
import UIKit

/// A helper class that aids in the creation of share sheets
class ShareExtensionHelper {
  /// Create a activity view controller with the given elements.
  /// - Parameters:
  ///   - selectedURL: The url or url content to share. May include an internal file or a link
  ///   - selectedTab: The provided tab is used for additional info such as a print renderer and title
  ///   - applicationActivities: The application activities to include in this share sheet.
  ///   - completionHandler: This will be triggered once the share sheet is dismissed and can be used to cleanup any lingering data
  /// - Returns: An `UIActivityViewController` prepped and ready to present.
  static func makeActivityViewController(
    selectedURL: URL,
    selectedTab: Tab? = nil,
    applicationActivities: [UIActivity] = []
  ) -> UIActivityViewController {
    let printInfo = UIPrintInfo(dictionary: nil)
    printInfo.jobName = selectedURL.absoluteString
    printInfo.outputType = .general

    var activityItems: [Any] = [
      printInfo, selectedURL,
    ]

    if let tab = selectedTab {
      // Adds the ability to "Print" or "Markup" the page using this custom renderer
      // Without this, the "Print" or "Markup feature would not exist"
      activityItems.append(TabPrintPageRenderer(tab: tab))
    }

    if let title = selectedTab?.title {
      // Makes sure the share sheet shows the same title as the tab
      // Also adds a title to several places, such as the Subject field in Mail
      activityItems.append(TitleActivityItemProvider(title: title))
    }

    let activityViewController = UIActivityViewController(activityItems: activityItems, applicationActivities: applicationActivities)

    // Hide 'Add to Reading List' which currently uses Safari.
    // We would also hide View Later, if possible, but the exclusion list doesn't currently support
    // third-party activity types (rdar://19430419).
    activityViewController.excludedActivityTypes = [
      UIActivity.ActivityType.addToReadingList
    ]

    return activityViewController
  }
}
