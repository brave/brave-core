// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveNews
import FeedKit
import Growth
import Preferences
import Shared
import UIKit
import os.log

extension BrowserViewController {
  func makeShareActivities(
    for url: URL,
    tab: Tab?,
    sourceView: UIView?,
    sourceRect: CGRect,
    arrowDirection: UIPopoverArrowDirection
  ) -> [UIActivity] {
    var activities = [UIActivity]()

    // Copy Clean URL Activity
    if !url.isLocal, !InternalURL.isValid(url: url), !url.isInternalURL(for: .readermode) {
      let cleanedURL =
        URLSanitizerServiceFactory.get(
          privateMode: tab?.isPrivate ?? true
        )?.sanitizeURL(url) ?? url

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
    if braveCore.syncAPI.isSendTabToSelfVisible, !privateBrowsingManager.isPrivateBrowsing,
      !url.isLocal, !InternalURL.isValid(url: url), !url.isInternalURL(for: .readermode)
    {
      activities.append(
        BasicMenuActivity(
          activityType: .sendURL,
          callback: { [weak self] in
            guard let self = self else { return }

            let deviceList = self.braveCore.sendTabAPI.getListOfSyncedDevices()
            let dataSource = SendableTabInfoDataSource(
              with: deviceList,
              displayTitle: tab?.displayTitle ?? "",
              sendableURL: url
            )

            let controller = SendTabToSelfController(
              sendTabAPI: self.braveCore.sendTabAPI,
              dataSource: dataSource
            )

            controller.sendWebSiteHandler = { [weak self] dataSource in
              guard let self = self else { return }

              self.present(
                SendTabProcessController(
                  type: .progress,
                  data: dataSource,
                  sendTabAPI: self.braveCore.sendTabAPI
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
    // If the reader mode button is occluded due to a secure content state warning add it as an activity
    if let tab = tabManager.selectedTab, tab.lastKnownSecureContentState.shouldDisplayWarning {
      if tab.readerModeAvailableOrActive {
        activities.append(
          BasicMenuActivity(
            activityType: .toggleReaderMode,
            callback: { [weak self] in
              self?.toggleReaderMode()
            }
          )
        )
      }
      // Any other buttons on the leading side of the location view should be added here as well
    }

    // Translate Activity
    if let translationState = tab?.translationState, translationState != .unavailable,
      Preferences.Translate.translateEnabled.value
    {
      activities.append(
        BasicMenuActivity(
          activityType: .translatePage,
          callback: { [weak self] in
            guard let self = self, let tab = tab else { return }

            let scriptHandler =
              tab.getContentScript(name: BraveTranslateScriptHandler.scriptName)
              as? BraveTranslateScriptHandler
            if let tabHelper = scriptHandler?.tabHelper {
              tabHelper.presentUI(on: self)

              if tab.translationState == .active {
                tabHelper.revertTranslation()
              } else if tab.translationState != .active {
                tabHelper.startTranslation(canShowToast: true)
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
        callback: { [weak self] in
          guard let self = self else { return }

          if let findInteraction = self.tabManager.selectedTab?.webView?.findInteraction {
            findInteraction.searchText = ""
            findInteraction.presentFindNavigator(showingReplace: false)
          }
        }
      )
    )

    // Page Zoom Activity
    activities.append(
      BasicMenuActivity(
        activityType: .pageZoom,
        callback: { [weak self] in
          guard let self = self else { return }

          self.displayPageZoom(visible: true)
        }
      )
    )

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
      let isDesktopSite = tab?.isDesktopSite ?? false
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
      if Preferences.BraveNews.isEnabled.value, let metadata = tab?.pageMetadata,
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
                  dataSource: self.feedDataSource,
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
      if let webView = tab?.webView, tab?.temporaryDocument == nil {
        activities.append(
          BasicMenuActivity(
            activityType: .createPDF,
            callback: {
              webView.createPDF { [weak self] result in
                dispatchPrecondition(condition: .onQueue(.main))
                guard let self = self else {
                  return
                }
                switch result {
                case .success(let pdfData):
                  Task {
                    // Create a valid filename
                    let validFilenameSet = CharacterSet(charactersIn: ":/")
                      .union(.newlines)
                      .union(.controlCharacters)
                      .union(.illegalCharacters)
                    let filename = webView.title?.components(separatedBy: validFilenameSet).joined()
                    let url = URL(fileURLWithPath: NSTemporaryDirectory())
                      .appendingPathComponent("\(filename ?? "Untitled").pdf")
                    do {
                      try await Task.detached {
                        try pdfData.write(to: url)
                      }.value
                      let pdfActivityController = UIActivityViewController(
                        activityItems: [url],
                        applicationActivities: nil
                      )
                      if let popoverPresentationController = pdfActivityController
                        .popoverPresentationController
                      {
                        popoverPresentationController.sourceView = sourceView
                        popoverPresentationController.sourceRect = sourceRect
                        popoverPresentationController.permittedArrowDirections = arrowDirection
                        popoverPresentationController.delegate = self
                      }
                      self.present(pdfActivityController, animated: true)
                    } catch {
                      Logger.module.error(
                        "Failed to write PDF to disk: \(error.localizedDescription, privacy: .public)"
                      )
                    }
                  }

                case .failure(let error):
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
      if let selectedTab = tabManager.selectedTab,
        selectedTab.mimeType == "application/xml" || selectedTab.mimeType == "application/json",
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
                  dataSource: self.feedDataSource,
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
    if let webView = tabManager.selectedTab?.webView,
      evaluateWebsiteSupportOpenSearchEngine(webView)
    {
      activities.append(
        BasicMenuActivity(
          activityType: .addSearchEngine,
          callback: { [weak self] in
            self?.addCustomSearchEngineForFocusedElement()
          }
        )
      )
    }

    // Display Certificate Activity
    if let tabURL = tabManager.selectedTab?.webView?.url,
      tabManager.selectedTab?.webView?.serverTrust != nil
        || ErrorPageHelper.hasCertificates(for: tabURL)
    {
      activities.append(
        BasicMenuActivity(
          activityType: .displaySecurityCertificate
        ) { [weak self] in
          self?.displayPageCertificateInfo()
        }
      )
    }

    // Report Web-compat Issue Activity
    activities.append(
      BasicMenuActivity(
        activityType: .reportBrokenSite
      ) { [weak self] in
        self?.showSubmitReportView(for: url)
      }
    )

    return activities
  }

  func presentActivityViewController(
    _ url: URL,
    tab: Tab? = nil,
    sourceView: UIView?,
    sourceRect: CGRect,
    arrowDirection: UIPopoverArrowDirection
  ) {
    let shareExtesionHelper = ShareExtensionHelper(url: url, tab: tab)

    let activities: [UIActivity] = makeShareActivities(
      for: url,
      tab: tab,
      sourceView: sourceView,
      sourceRect: sourceRect,
      arrowDirection: arrowDirection
    )

    let controller = shareExtesionHelper.createActivityViewController(
      applicationActivities:
        activities
    )

    controller.completionWithItemsHandler = { [weak self] _, _, _, _ in
      self?.cleanUpCreateActivity()
    }

    if let popoverPresentationController = controller.popoverPresentationController {
      popoverPresentationController.sourceView = sourceView
      popoverPresentationController.sourceRect = sourceRect
      popoverPresentationController.permittedArrowDirections = arrowDirection
      popoverPresentationController.delegate = self
    }

    present(controller, animated: true, completion: nil)
  }

  private func cleanUpCreateActivity() {
    // After dismissing, check to see if there were any prompts we queued up
    showQueuedAlertIfAvailable()

    // Usually the popover delegate would handle nil'ing out the references we have to it
    // on the BVC when displaying as a popover but the delegate method doesn't seem to be
    // invoked on iOS 10. See Bug 1297768 for additional details.
    displayedPopoverController = nil
    updateDisplayedPopoverProperties = nil
  }
}
