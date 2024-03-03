// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveStrings
import Foundation
import Shared
import SwiftUI
import UIKit

protocol MenuActivity: UIActivity {
  /// The image to use when shown on the menu.
  var menuImage: Image { get }
}

/// A standard activity that will appear in the apps menu and executes a callback when the user selects it
class BasicMenuActivity: UIActivity, MenuActivity {

  enum ActivityType: String {
    case copyCleanLink
    case sendURL
    case toggleReaderMode
    case findInPage
    case pageZoom
    case addToFavourites
    case requestDesktopSite
    case requestMobileSite
    case addSourceBraveNews
    case createPDF
    case addSearchEngine
    case displayCertificate
    case reportBrokenSite

    var title: String {
      switch self {
      case .copyCleanLink:
        return Strings.copyCleanLink
      case .sendURL:
        return Strings.OpenTabs.sendWebsiteShareActionTitle
      case .toggleReaderMode:
        return Strings.toggleReaderMode
      case .findInPage:
        return Strings.findInPage
      case .pageZoom:
        return Strings.PageZoom.settingsTitle
      case .addToFavourites:
        return Strings.addToFavorites
      case .requestDesktopSite:
        return Strings.appMenuViewDesktopSiteTitleString
      case .requestMobileSite:
        return Strings.appMenuViewMobileSiteTitleString
      case .addSourceBraveNews:
        return Strings.BraveNews.addSourceShareTitle
      case .createPDF:
        return Strings.createPDF
      case .addSearchEngine:
        return Strings.CustomSearchEngine.customEngineNavigationTitle
      case .displayCertificate:
        return Strings.displayCertificate
      case .reportBrokenSite:
        return Strings.Shields.reportABrokenSite
      }
    }

    var braveSystemImage: String {
      switch self {
      case .copyCleanLink:
        return "leo.broom"
      case .sendURL:
        return "leo.smartphone.laptop"
      case .toggleReaderMode:
        return "leo.product.speedreader"
      case .findInPage:
        return "leo.search"
      case .pageZoom:
        return "leo.font.size"
      case .addToFavourites:
        return "leo.widget.generic"
      case .requestDesktopSite:
        return "leo.monitor"
      case .requestMobileSite:
        return "leo.smartphone"
      case .addSourceBraveNews:
        return "leo.rss"
      case .createPDF:
        return "leo.file"
      case .addSearchEngine:
        return "leo.search.zoom-in"
      case .displayCertificate:
        return "leo.lock.plain"
      case .reportBrokenSite:
        return "leo.warning.triangle-outline"
      }
    }
  }

  private let title: String
  private let braveSystemImage: String
  private let activityTypeID: String
  private let callback: () -> Void

  init(
    type: ActivityType,
    callback: @escaping () -> Void
  ) {
    self.title = type.title
    self.braveSystemImage = type.braveSystemImage
    self.activityTypeID = type.rawValue
    self.callback = callback
  }

  // MARK: - UIActivity

  override var activityTitle: String? {
    return title
  }

  override var activityImage: UIImage? {
    return UIImage(braveSystemNamed: braveSystemImage)?.applyingSymbolConfiguration(
      .init(scale: .large)
    )
  }

  override func perform() {
    callback()
    activityDidFinish(true)
  }

  override func canPerform(withActivityItems activityItems: [Any]) -> Bool {
    return true
  }

  override var activityType: UIActivity.ActivityType {
    let bundleId =
      AppInfo.applicationBundle.object(forInfoDictionaryKey: "CFBundleIdentifier") as? String

    return UIActivity.ActivityType(rawValue: "\(bundleId ?? "")\(".\(activityTypeID)")")
  }

  // MARK: - MenuActivity

  var menuImage: Image {
    Image(braveSystemName: braveSystemImage)
  }
}
