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

/// Groups the three popover-positioning values into one argument to reduce repetition
/// across makeShareActivities and presentActivityViewController call sites.
struct SharePopoverSource {
  let view: UIView?
  let rect: CGRect
  let arrowDirection: UIPopoverArrowDirection
}

protocol ShareActivityProvider: AnyObject, UIAdaptivePresentationControllerDelegate {
  var shareSelectedTab: (any TabState)? { get }
  var shareProfileController: BraveProfileController { get }
  var shareIsPrivateBrowsing: Bool { get }
  var shareFeedDataSource: FeedDataSource? { get }
  var shareCanAddSearchEngine: Bool { get }

  func shareShowSubmitReport(for url: URL)
  func shareToggleReaderMode()
  func shareDisplayPageZoom()
  func shareAddSearchEngine()
  func shareDisplayCertificate()
  func shareCleanUp()
}

extension ShareActivityProvider where Self: UIViewController {
  var shareFeedDataSource: FeedDataSource? { nil }
  var shareCanAddSearchEngine: Bool { false }
  var shareCanDisplayPageZoom: Bool { true }
  var shareCanDisplayCertificate: Bool { true }
  func shareToggleReaderMode() {}
  func shareDisplayPageZoom() {}
  func shareAddSearchEngine() {}
  func shareDisplayCertificate() {}
  func shareCleanUp() {}
}

extension ShareActivityProvider where Self: UIViewController {
  func makeShareActivities(
    for url: URL,
    tab: (any TabState)?,
    source: SharePopoverSource
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
    if shareProfileController.syncAPI.isSendTabToSelfVisible, !shareIsPrivateBrowsing,
      !url.isLocal, !InternalURL.isValid(url: url), !url.isInternalURL(for: .readermode)
    {
      activities.append(
        BasicMenuActivity(
          activityType: .sendURL,
          callback: { [weak self] in
            guard let self = self else { return }

            let deviceList = self.shareProfileController.sendTabAPI.getListOfSyncedDevices()
            let dataSource = SendableTabInfoDataSource(
              with: deviceList,
              displayTitle: tab?.displayTitle ?? "",
              sendableURL: url
            )

            let controller = SendTabToSelfController(
              sendTabAPI: self.shareProfileController.sendTabAPI,
              dataSource: dataSource
            )

            controller.sendWebSiteHandler = { [weak self] dataSource in
              guard let self = self else { return }

              self.present(
                SendTabProcessController(
                  type: .progress,
                  data: dataSource,
                  sendTabAPI: self.shareProfileController.sendTabAPI
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
    if let tab = shareSelectedTab, tab.readerModeAvailableOrActive == true {
      activities.append(
        BasicMenuActivity(
          activityType: .toggleReaderMode,
          callback: { [weak self] in
            self?.shareToggleReaderMode()
          }
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

              if tab.translationState == .active {
                translateHelper.revertTranslation()
              } else if tab.translationState != .active {
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
        callback: { [weak tab] in
          guard let tab else { return }
          tab.presentFindInteraction(with: "")
        }
      )
    )

    // Page Zoom Activity
    if shareCanDisplayPageZoom {
      activities.append(
        BasicMenuActivity(
          activityType: .pageZoom,
          callback: { [weak self] in
            guard let self = self else { return }
            self.shareDisplayPageZoom()
          }
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
      if let feedDS = shareFeedDataSource {
        if shareProfileController.profile.prefs.isBraveNewsAvailable,
          Preferences.BraveNews.isEnabled.value, let metadata = tab?.pageMetadataHelper?.metadata,
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
                    dataSource: feedDS,
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
      }

      // Create PDF Activity
      if let tab, tab.temporaryDocument == nil, tab.lastCommittedURL?.isWebPage() == true {
        activities.append(
          BasicMenuActivity(
            activityType: .createPDF,
            callback: {
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
                    pdfActivityController.presentationController?.delegate = self
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
      if let feedDS = shareFeedDataSource,
        let selectedTab = shareSelectedTab,
        selectedTab.contentsMimeType == "application/xml"
          || selectedTab.contentsMimeType == "application/json",
        let tabURL = selectedTab.url
      {

        let parser = FeedParser(URL: url)
        if case .success(let feed) = parser.parse() {
          activities.append(
            BasicMenuActivity(
              activityType: .addSourceNews,
              callback: { [weak self] in
                guard let self = self else { return }
                let controller = BraveNewsAddSourceResultsViewController(
                  dataSource: feedDS,
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
    if shareCanAddSearchEngine {
      activities.append(
        BasicMenuActivity(
          activityType: .addSearchEngine,
          callback: { [weak self] in
            self?.shareAddSearchEngine()
          }
        )
      )
    }

    // Display Certificate Activity
    if shareCanDisplayCertificate, shareSelectedTab?.serverTrust != nil {
      activities.append(
        BasicMenuActivity(
          activityType: .displaySecurityCertificate
        ) { [weak self] in
          self?.shareDisplayCertificate()
        }
      )
    }

    // Report Web-compat Issue Activity
    activities.append(
      BasicMenuActivity(
        activityType: .reportBrokenSite
      ) { [weak self] in
        self?.shareShowSubmitReport(for: url)
      }
    )

    return activities
  }

  func presentActivityViewController(
    _ url: URL,
    tab: (any TabState)? = nil,
    source: SharePopoverSource
  ) {

    let shareExtesionHelper = ShareExtensionHelper(url: url, tab: tab)
    let activities: [UIActivity] = makeShareActivities(for: url, tab: tab, source: source)
    let controller = shareExtesionHelper.createActivityViewController(
      applicationActivities: activities
    )

    controller.completionWithItemsHandler = { [weak self] _, _, _, _ in
      self?.shareCleanUp()
    }

    controller.presentationController?.delegate = self
    if let popoverPresentationController = controller.popoverPresentationController {
      popoverPresentationController.sourceView = source.view
      popoverPresentationController.sourceRect = source.rect
      popoverPresentationController.permittedArrowDirections = source.arrowDirection
    }

    present(controller, animated: true, completion: nil)
  }
}
