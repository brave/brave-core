// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveNews
import FeedKit
import Growth
import Preferences
import Shared
import UIKit
import Web
import os.log

struct SharePopoverSource {
  let view: UIView?
  let rect: CGRect
  let arrowDirection: UIPopoverArrowDirection
}

struct ShareActivityCallbacks {
  var onToggleReaderMode: (() -> Void)?
  var onDisplayPageZoom: (() -> Void)?
  var onAddSearchEngine: (() -> Void)?
  var onDisplayCertificate: (() -> Void)?
  var onShowSubmitReport: ((URL) -> Void)?
  var onCleanUp: (() -> Void)?
}

extension UIViewController {
  func makeShareActivities(
    url: URL,
    tab: (any TabState)?,
    syncAPI: BraveSyncAPI,
    sendTabAPI: BraveSendTabAPI,
    feedDataSource: FeedDataSource?,
    isBraveNewsAvailable: Bool,
    source: SharePopoverSource,
    callbacks: ShareActivityCallbacks
  ) -> [UIActivity] {

    var activities = [UIActivity]()

    // Copy Clean URL Activity
    if !url.isLocal, !InternalURL.isValid(url: url), !url.isInternalURL(for: .readermode) {
      let cleanedURL =
        URLSanitizerServiceFactory.get(
          privateMode: tab?.isPrivate ?? true
        )?.sanitize(url: url) ?? url

      activities.append(
        BasicMenuActivity(
          activityType: .copyCleanLink,
          callback: {
            UIPasteboard.general.url = cleanedURL
          }
        )
      )
    }

    // Send Tab To Self Activity - Show device selection screen
    if let tab, !tab.isPrivate,
      syncAPI.isSendTabToSelfVisible,
      !url.isLocal, !InternalURL.isValid(url: url), !url.isInternalURL(for: .readermode)
    {
      activities.append(
        BasicMenuActivity(
          activityType: .sendURL,
          callback: { [weak self] in
            guard let self = self else { return }

            let deviceList = sendTabAPI.getListOfSyncedDevices()
            let dataSource = SendableTabInfoDataSource(
              with: deviceList,
              displayTitle: tab.displayTitle,
              sendableURL: url
            )

            let controller = SendTabToSelfController(
              sendTabAPI: sendTabAPI,
              dataSource: dataSource
            )

            controller.sendWebSiteHandler = { [weak self] dataSource in
              guard let self = self else { return }

              self.present(
                SendTabProcessController(
                  type: .progress,
                  data: dataSource,
                  sendTabAPI: sendTabAPI
                ),
                animated: true,
                completion: nil
              )
            }
            self.present(controller, animated: true, completion: nil)
          }
        )
      )
    }

    // Toogle Reader Mode Activity
    if let tab,
      tab.readerModeAvailableOrActive == true,
      let onToggleReaderMode = callbacks.onToggleReaderMode
    {
      activities.append(
        BasicMenuActivity(
          activityType: .toggleReaderMode,
          callback: onToggleReaderMode
        )
      )
    }

    // Translate Activity
    if FeatureList.kBraveTranslateEnabled.enabled,
      let translationState = tab?.translationState,
      translationState != .unavailable,
      Preferences.Translate.translateEnabled.value
    {
      activities.append(
        BasicMenuActivity(
          activityType: .translatePage,
          callback: { [weak self] in
            guard let self = self, let tab = tab else { return }

            if let translateTabHelper = tab.translate {
              translateTabHelper.toggleTranslation()
            }

            if let translateHelper = tab.legacyTranslateHelper {
              translateHelper.presentUI(on: self)

              if translateHelper.translationState == .active {
                translateHelper.revertTranslation()
              } else if translateHelper.translationState != .active {
                translateHelper.startTranslation(canShowToast: true)
              }
            }
          }
        )
      )
    }

    // Find In Page Activity
    activities.append(
      BasicMenuActivity(
        activityType: .findInPage,
        callback: {
          tab?.presentFindInteraction(with: "")
        }
      )
    )

    // Page Zoom Activity
    if let onDisplayPageZoom = callbacks.onDisplayPageZoom {
      activities.append(
        BasicMenuActivity(
          activityType: .pageZoom,
          callback: onDisplayPageZoom
        )
      )
    }

    // Activities when scheme is not `file
    // These actions don't apply if we're sharing a temporary document
    if !url.isFileURL {
      // Add to Favourites Activity
      // We don't allow to have 2 same favorites.
      if !FavoritesHelper.isAlreadyAdded(url) {
        activities.append(
          BasicMenuActivity(
            activityType: .addFavourites,
            callback: { [weak self] in
              guard let self = self else { return }

              FavoritesHelper.add(url: url, title: tab?.displayTitle)
              // Handle App Rating
              // Check for review condition after adding a favorite
              AppReviewManager.shared.handleAppReview(for: .revised, using: self)
            }
          )
        )
      }

      // Request Desktop Site Activity
      let isDesktopSite = tab?.currentUserAgentType == .desktop
      var requestDesktopSite = BasicMenuActivity.ActivityType.requestDesktopSite
      if isDesktopSite {
        requestDesktopSite.title = Strings.appMenuViewMobileSiteTitleString
        requestDesktopSite.braveSystemImage = "leo.smartphone"
      }
      activities.append(
        BasicMenuActivity(
          activityType: requestDesktopSite,
          callback: {
            tab?.switchUserAgent()
          }
        )
      )

      // Add Feed To Brave News Activity
      if let feedDataSource, isBraveNewsAvailable,
        Preferences.BraveNews.isEnabled.value,
        let metadata = tab?.pageMetadataHelper?.metadata,
        !metadata.feeds.isEmpty
      {
        let feeds: [RSSFeedLocation] = metadata.feeds.compactMap { feed in
          guard let url = URL(string: feed.href) else { return nil }
          return RSSFeedLocation(title: feed.title, url: url)
        }
        if !feeds.isEmpty {
          activities.append(
            BasicMenuActivity(
              activityType: .addSourceNews,
              callback: { [weak self] in
                guard let self = self else { return }
                let controller = BraveNewsAddSourceResultsViewController(
                  dataSource: feedDataSource,
                  searchedURL: url,
                  rssFeedLocations: feeds,
                  sourcesAdded: nil
                )
                let container = UINavigationController(rootViewController: controller)
                let idiom = UIDevice.current.userInterfaceIdiom
                container.modalPresentationStyle = idiom == .phone ? .pageSheet : .formSheet
                self.present(container, animated: true)
              }
            )
          )
        }
      }

      // Create PDF Activity
      if let tab, tab.temporaryDocument == nil, tab.lastCommittedURL?.isWebPage() == true {
        activities.append(
          BasicMenuActivity(
            activityType: .createPDF,
            callback: { [weak self] in
              guard let self else { return }
              Task { @MainActor in
                do {
                  guard let pdfData = try await tab.createFullPagePDF() else {
                    return
                  }
                  // Create a valid filename
                  let validFilenameSet = CharacterSet(charactersIn: ":/")
                    .union(.newlines)
                    .union(.controlCharacters)
                    .union(.illegalCharacters)
                  let filename =
                    tab.title?.components(separatedBy: validFilenameSet).joined() ?? ""
                  let url = URL(fileURLWithPath: NSTemporaryDirectory())
                    .appendingPathComponent("\(filename.isEmpty ? "Untitled" : filename).pdf")
                  do {
                    try await Task.detached {
                      try pdfData.write(to: url)
                    }.value
                    let pdfActivityController = UIActivityViewController(
                      activityItems: [url],
                      applicationActivities: nil
                    )
                    pdfActivityController.presentationController?.delegate =
                      self as? UIAdaptivePresentationControllerDelegate
                    if let popoverPresentationController = pdfActivityController
                      .popoverPresentationController
                    {
                      popoverPresentationController.sourceView = source.view
                      popoverPresentationController.sourceRect = source.rect
                      popoverPresentationController.permittedArrowDirections = source.arrowDirection
                    }
                    self.present(pdfActivityController, animated: true)
                  } catch {
                    Logger.module.error(
                      "Failed to write PDF to disk: \(error.localizedDescription, privacy: .public)"
                    )
                  }
                } catch {
                  Logger.module.error(
                    "Failed to create PDF with error: \(error.localizedDescription)"
                  )
                }
              }
            }
          )
        )
      }
    } else {
      // Add Feed To Brave News Activity
      // Check if it's a feed, url is a temp document file URL
      if let feedDataSource, let tab,
        tab.contentsMimeType == "application/xml"
          || tab.contentsMimeType == "application/json",
        let tabURL = tab.visibleURL
      {
        let parser = FeedParser(URL: url)
        if case .success(let feed) = parser.parse() {
          activities.append(
            BasicMenuActivity(
              activityType: .addSourceNews,
              callback: { [weak self] in
                guard let self = self else { return }
                let controller = BraveNewsAddSourceResultsViewController(
                  dataSource: feedDataSource,
                  searchedURL: tabURL,
                  rssFeedLocations: [.init(title: feed.title, url: tabURL)],
                  sourcesAdded: nil
                )
                let container = UINavigationController(rootViewController: controller)
                let idiom = UIDevice.current.userInterfaceIdiom
                container.modalPresentationStyle = idiom == .phone ? .pageSheet : .formSheet
                self.present(container, animated: true)
              }
            )
          )
        }
      }
    }

    // Add Search Engine Activity
    if let onAddSearchEngine = callbacks.onAddSearchEngine,
      tab?.pageMetadataHelper?.metadata?.search != nil,
      tab?.visibleURL?.isSecureWebPage() == true
    {
      activities.append(
        BasicMenuActivity(
          activityType: .addSearchEngine,
          callback: onAddSearchEngine
        )
      )
    }

    // Display Certificate Activity
    if let onDisplayCertificate = callbacks.onDisplayCertificate, tab?.serverTrust != nil {
      activities.append(
        BasicMenuActivity(
          activityType: .displaySecurityCertificate,
          callback: onDisplayCertificate
        )
      )
    }

    // Report Web-compat Issue Activity
    if let onShowSubmitReport = callbacks.onShowSubmitReport {
      activities.append(
        BasicMenuActivity(
          activityType: .reportBrokenSite
        ) { onShowSubmitReport(url) }
      )
    }

    return activities
  }

  func presentShareActivity(
    url: URL,
    tab: (any TabState)?,
    syncAPI: BraveSyncAPI,
    sendTabAPI: BraveSendTabAPI,
    feedDataSource: FeedDataSource?,
    isBraveNewsAvailable: Bool,
    source: SharePopoverSource,
    callbacks: ShareActivityCallbacks
  ) {
    let activities = makeShareActivities(
      url: url,
      tab: tab,
      syncAPI: syncAPI,
      sendTabAPI: sendTabAPI,
      feedDataSource: feedDataSource,
      isBraveNewsAvailable: isBraveNewsAvailable,
      source: source,
      callbacks: callbacks
    )
    let controller = ShareExtensionHelper(url: url, tab: tab)
      .createActivityViewController(applicationActivities: activities)
    controller.completionWithItemsHandler = { _, _, _, _ in
      callbacks.onCleanUp?()
    }
    controller.presentationController?.delegate = self as? UIAdaptivePresentationControllerDelegate
    if let popover = controller.popoverPresentationController {
      popover.sourceView = source.view
      popover.sourceRect = source.rect
      popover.permittedArrowDirections = source.arrowDirection
    }
    present(controller, animated: true, completion: nil)
  }
}
