// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import Shared
import UIKit

/// A helper class that aids in the creation of share sheets
class ShareExtensionHelper {
  /// URL and Tab to be shared
  private let selectedURL: URL
  private weak var selectedTab: Tab?

  init(url: URL, tab: Tab?) {
    self.selectedURL = url
    self.selectedTab = tab
  }

  /// Create a activity view controller with the given elements.
  /// - Parameters:
  ///   - selectedURL: The url or url content to share. May include an internal file or a link
  ///   - selectedTab: The provided tab is used for additional info such as a print renderer and title
  ///   - applicationActivities: The application activities to include in this share sheet.
  ///   - completionHandler: This will be triggered once the share sheet is dismissed and can be used to cleanup any lingering data
  /// - Returns: An `UIActivityViewController` prepped and ready to present.
  func createActivityViewController(
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

    let activityViewController = UIActivityViewController(
      activityItems: activityItems,
      applicationActivities: applicationActivities
    )

    // Hide 'Add to Reading List' which currently uses Safari.
    // We would also hide View Later, if possible, but the exclusion list doesn't currently support
    // third-party activity types (rdar://19430419).
    activityViewController.excludedActivityTypes = [
      UIActivity.ActivityType.addToReadingList
    ]

    return activityViewController
  }
}

extension BasicMenuActivity.ActivityType {
  static let copyCleanLink: Self = .init(
    id: "CopyCleanLink",
    title: Strings.copyCleanLink,
    braveSystemImage: "leo.broom"
  )
  static let sendURL: Self = .init(
    id: "SendURL",
    title: Strings.OpenTabs.sendWebsiteShareActionTitle,
    braveSystemImage: "leo.smartphone.laptop"
  )
  static let toggleReaderMode: Self = .init(
    id: "ToggleReaderMode",
    title: Strings.toggleReaderMode,
    braveSystemImage: "leo.product.speedreader"
  )
  static let translatePage: Self = .init(
    id: "TranslatePage",
    title: Strings.translatePage,
    braveSystemImage: "leo.product.translate"
  )
  static let findInPage: Self = .init(
    id: "FindInPage",
    title: Strings.findInPage,
    braveSystemImage: "leo.search"
  )
  static let pageZoom: Self = .init(
    id: "PageZoom",
    title: Strings.PageZoom.settingsTitle,
    braveSystemImage: "leo.font.size"
  )
  static let addFavourites: Self = .init(
    id: "AddFavourites",
    title: Strings.addToFavorites,
    braveSystemImage: "leo.widget.generic"
  )
  static let requestDesktopSite: Self = .init(
    id: "ToggleUserAgent",
    title: Strings.appMenuViewDesktopSiteTitleString,
    braveSystemImage: "leo.monitor"
  )
  static let addSourceNews: Self = .init(
    id: "AddSourceNews",
    title: Strings.BraveNews.addSourceShareTitle,
    braveSystemImage: "leo.rss"
  )
  static let createPDF: Self = .init(
    id: "CreatePDF",
    title: Strings.createPDF,
    braveSystemImage: "leo.file.new"
  )
  static let addSearchEngine: Self = .init(
    id: "AddSearchEngine",
    title: Strings.CustomSearchEngine.customEngineNavigationTitle,
    braveSystemImage: "leo.search.zoom-in"
  )
  static let displaySecurityCertificate: Self = .init(
    id: "DisplaySecurityCertificate",
    title: Strings.displayCertificate,
    braveSystemImage: "leo.lock.plain"
  )
  static let reportBrokenSite: Self = .init(
    id: "ReportBrokenSite",
    title: Strings.Shields.reportABrokenSite,
    braveSystemImage: "leo.warning.triangle-outline"
  )
}
