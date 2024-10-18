// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveNews
import BraveVPN
import BrowserIntentsModels
import CoreSpotlight
import Data
import Growth
import Intents
import MobileCoreServices
import PlaylistUI
import Preferences
import Shared
import SwiftUI
import UIKit
import UniformTypeIdentifiers
import os.log

/// Shortcut Activity Types and detailed information to create and perform actions
public enum ActivityType: String {
  case newTab = "NewTab"
  case newPrivateTab = "NewPrivateTab"
  case openBookmarks = "OpenBookmarks"
  case openHistoryList = "OpenHistoryList"
  case clearBrowsingHistory = "ClearBrowsingHistory"
  case enableBraveVPN = "EnableBraveVPN"
  case openBraveNews = "OpenBraveNews"
  case openPlayList = "OpenPlayList"
  case openSyncedTabs = "OpenSyncedTabs"

  public var identifier: String {
    return "\(Bundle.main.bundleIdentifier ?? "").\(self.rawValue)"
  }

  /// The activity title for designated  type
  public var title: String {
    switch self {
    case .newTab:
      return Strings.Shortcuts.activityTypeNewTabTitle
    case .newPrivateTab:
      return Strings.Shortcuts.activityTypeNewPrivateTabTitle
    case .openBookmarks:
      return Strings.Shortcuts.activityTypeOpenBookmarksTitle
    case .openHistoryList:
      return Strings.Shortcuts.activityTypeOpenHistoryListTitle
    case .clearBrowsingHistory:
      return Strings.Shortcuts.activityTypeClearHistoryTitle
    case .enableBraveVPN:
      return Strings.Shortcuts.activityTypeEnableVPNTitle
    case .openBraveNews:
      return Strings.Shortcuts.activityTypeOpenBraveNewsTitle
    case .openPlayList:
      return Strings.Shortcuts.activityTypeOpenPlaylistTitle
    case .openSyncedTabs:
      return Strings.Shortcuts.activityTypeOpenSyncedTabsTitle
    }
  }

  /// The content description for designated activity  type
  public var description: String {
    switch self {
    case .newTab, .newPrivateTab:
      return Strings.Shortcuts.activityTypeTabDescription
    case .openHistoryList:
      return Strings.Shortcuts.activityTypeOpenHistoryListDescription
    case .openBookmarks:
      return Strings.Shortcuts.activityTypeOpenBookmarksDescription
    case .clearBrowsingHistory:
      return Strings.Shortcuts.activityTypeClearHistoryDescription
    case .enableBraveVPN:
      return Strings.Shortcuts.activityTypeEnableVPNDescription
    case .openBraveNews:
      return Strings.Shortcuts.activityTypeBraveNewsDescription
    case .openPlayList:
      return Strings.Shortcuts.activityTypeOpenPlaylistDescription
    case .openSyncedTabs:
      return Strings.Shortcuts.activityTypeOpenSyncedTabsDescription
    }
  }

  /// The phrase suggested to the user when they create a shortcut for the activity
  public var suggestedPhrase: String {
    switch self {
    case .newTab:
      return Strings.Shortcuts.activityTypeNewTabSuggestedPhrase
    case .newPrivateTab:
      return Strings.Shortcuts.activityTypeNewPrivateTabSuggestedPhrase
    case .openBookmarks:
      return Strings.Shortcuts.activityTypeOpenBookmarksSuggestedPhrase
    case .openHistoryList:
      return Strings.Shortcuts.activityTypeOpenHistoryListSuggestedPhrase
    case .clearBrowsingHistory:
      return Strings.Shortcuts.activityTypeClearHistorySuggestedPhrase
    case .enableBraveVPN:
      return Strings.Shortcuts.activityTypeEnableVPNSuggestedPhrase
    case .openBraveNews:
      return Strings.Shortcuts.activityTypeOpenBraveNewsSuggestedPhrase
    case .openPlayList:
      return Strings.Shortcuts.activityTypeOpenPlaylistSuggestedPhrase
    case .openSyncedTabs:
      return Strings.Shortcuts.activityTypeOpenSyncedTabsSuggestedPhrase
    }
  }
}

/// Singleton Manager handles creation and action for Activities
public class ActivityShortcutManager: NSObject {

  /// Custom Intent Types
  public enum IntentType {
    case openWebsite
    case openHistory
    case openBookmarks
  }

  // MARK: Lifecycle

  public static var shared = ActivityShortcutManager()

  // MARK: Activity Creation Methods

  public func createShortcutActivity(type: ActivityType) -> NSUserActivity {
    let attributes = CSSearchableItemAttributeSet(itemContentType: UTType.item.identifier)
    attributes.contentDescription = type.description

    let activity = NSUserActivity(activityType: type.identifier)
    activity.persistentIdentifier = NSUserActivityPersistentIdentifier(type.identifier)

    activity.isEligibleForSearch = true
    activity.isEligibleForPrediction = true

    activity.title = type.title
    activity.suggestedInvocationPhrase = type.suggestedPhrase
    activity.contentAttributeSet = attributes

    return activity
  }

  // MARK: Activity Action Methods

  public func performShortcutActivity(type: ActivityType, using bvc: BrowserViewController) {
    // Adding a slight delay to overcome concurency issues with bvc setup
    DispatchQueue.main.asyncAfter(deadline: .now() + 0.25) {
      self.handleActivityDetails(type: type, using: bvc)
    }
  }

  private func handleActivityDetails(type: ActivityType, using bvc: BrowserViewController) {
    switch type {
    case .newTab:
      bvc.openBlankNewTab(
        attemptLocationFieldFocus: false,
        isPrivate: bvc.privateBrowsingManager.isPrivateBrowsing,
        isExternal: true
      )
      bvc.popToBVC()
    case .newPrivateTab:
      bvc.openBlankNewTab(attemptLocationFieldFocus: false, isPrivate: true, isExternal: true)
      bvc.popToBVC()
    case .openBookmarks:
      bvc.popToBVC()
      bvc.navigationHelper.openBookmarks()
    case .openHistoryList:
      bvc.popToBVC()
      bvc.navigationHelper.openHistory(isModal: true)
    case .clearBrowsingHistory:
      bvc.clearHistoryAndOpenNewTab()
    case .enableBraveVPN:
      bvc.openBlankNewTab(
        attemptLocationFieldFocus: false,
        isPrivate: bvc.privateBrowsingManager.isPrivateBrowsing,
        isExternal: true
      )
      bvc.popToBVC()

      switch BraveVPN.vpnState {
      case .notPurchased, .expired:
        guard BraveVPN.vpnState.isPaywallEnabled else { return }

        let vpnPaywallView = BraveVPNPaywallView(openVPNAuthenticationInNewTab: { [weak bvc] in
          guard let bvc = bvc else { return }

          bvc.popToBVC()

          bvc.openURLInNewTab(
            .brave.braveVPNRefreshCredentials,
            isPrivate: bvc.privateBrowsingManager.isPrivateBrowsing,
            isPrivileged: false
          )
        })

        bvc.openInsideSettingsNavigation(with: UIHostingController(rootView: vpnPaywallView))
      case .purchased(let connected):
        if !connected {
          BraveVPN.reconnect()
        }
      }
    case .openBraveNews:
      // Do nothing as browser when browser to PB only and Brave News isn't available on private tabs
      guard !Preferences.Privacy.privateBrowsingOnly.value else {
        return
      }

      if Preferences.BraveNews.isEnabled.value {
        bvc.openBlankNewTab(attemptLocationFieldFocus: false, isPrivate: false, isExternal: true)
        bvc.popToBVC()

        guard let newTabPageController = bvc.tabManager.selectedTab?.newTabPageViewController else {
          return
        }
        newTabPageController.scrollToBraveNews()
      } else {
        let controller = NewsSettingsViewController(
          dataSource: bvc.feedDataSource,
          openURL: { url in
            bvc.dismiss(animated: true)
            bvc.select(url: url, isUserDefinedURLNavigation: false)
          }
        )
        controller.viewDidDisappear = {
          if Preferences.Review.braveNewsCriteriaPassed.value {
            AppReviewManager.shared.isRevisedReviewRequired = true
            Preferences.Review.braveNewsCriteriaPassed.value = false
          }
        }
        let container = UINavigationController(rootViewController: controller)
        bvc.present(container, animated: true)
      }
    case .openPlayList:
      bvc.popToBVC()

      let tab = bvc.tabManager.selectedTab
      PlaylistCoordinator.shared.getPlaylistController(tab: tab) { playlistController in
        PlaylistP3A.recordUsage()
        bvc.present(playlistController, animated: true)
      }
    case .openSyncedTabs:
      bvc.popToBVC()
      bvc.showTabTray(isExternallyPresented: true)
    }
  }

  // MARK: Intent Creation Methods

  private func createCustomIntent(for type: IntentType, with urlString: String) -> INIntent {
    switch type {
    case .openWebsite:
      let intent = OpenWebsiteIntent()
      intent.websiteURL = urlString
      intent.suggestedInvocationPhrase = Strings.Shortcuts.customIntentOpenWebsiteSuggestedPhrase

      return intent
    case .openHistory:
      let intent = OpenHistoryWebsiteIntent()
      intent.websiteURL = urlString
      intent.suggestedInvocationPhrase = Strings.Shortcuts.customIntentOpenHistorySuggestedPhrase

      return intent
    case .openBookmarks:
      let intent = OpenBookmarkWebsiteIntent()
      intent.websiteURL = urlString
      intent.suggestedInvocationPhrase = Strings.Shortcuts.customIntentOpenBookmarkSuggestedPhrase

      return intent
    }
  }

  // MARK: Intent Donation Methods

  public func donateCustomIntent(for type: IntentType, with urlString: String) {
    guard !urlString.isEmpty,
      URL(string: urlString) != nil
    else {
      return
    }

    let intent = createCustomIntent(for: type, with: urlString)

    let interaction = INInteraction(intent: intent, response: nil)
    interaction.donate { error in
      guard let error = error else {
        return
      }

      Logger.module.error(
        "Failed to donate shortcut open website, error: \(error.localizedDescription)"
      )
    }
  }
}
