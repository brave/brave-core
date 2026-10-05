// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShared
import BraveVPN
import Foundation
import Preferences
import Shared
import SwiftUI
import UIKit

/// Handles displaying controllers such as settings, bookmarks, etc. on top of
/// the browser.
class BrowserNavigationHelper {
  private weak var bvc: BrowserViewController?

  /// The in-flight task preparing the share sheet, if any
  private var shareTask: Task<Void, Never>?

  init(_ browserViewController: BrowserViewController) {
    bvc = browserViewController
  }

  private enum DoneButtonPosition { case left, right }
  private typealias DoneButton = (style: UIBarButtonItem.SystemItem, position: DoneButtonPosition)

  private func open(
    _ viewController: UIViewController,
    doneButton: DoneButton,
    allowSwipeToDismiss: Bool = true
  ) {
    let nav = SettingsNavigationController(rootViewController: viewController).then {
      $0.isModalInPresentation = !allowSwipeToDismiss
      $0.modalPresentationStyle =
        UIDevice.current.userInterfaceIdiom == .phone ? .pageSheet : .formSheet
    }

    let button = UIBarButtonItem(
      barButtonSystemItem: doneButton.style,
      target: nav,
      action: #selector(nav.done)
    )

    switch doneButton.position {
    case .left: nav.navigationBar.topItem?.leftBarButtonItem = button
    case .right: nav.navigationBar.topItem?.rightBarButtonItem = button
    }

    dismissView()
    bvc?.present(nav, animated: true)
  }

  @MainActor
  func openBookmarks() {
    guard let bvc = bvc else { return }
    let vc = BookmarksViewController(
      folder: bvc.bookmarkManager.lastVisitedFolder(),
      bookmarkManager: bvc.bookmarkManager,
      isPrivateBrowsing: bvc.privateBrowsingManager.isPrivateBrowsing
    )
    vc.toolbarUrlActionsDelegate = bvc

    open(vc, doneButton: DoneButton(style: .done, position: .right))
  }

  @MainActor
  func openSyncedTabsList() {
    guard let bvc = bvc else { return }
    let controller = UIHostingController(
      rootView: SyncedTabsView(
        openTabs: bvc.profileController.openTabsAPI,
        urlActionHandler: bvc,
        openSyncSettings: { [unowned bvc] in
          let controller: UIViewController
          if Preferences.Chromium.syncEnabled.value {
            controller = SyncSettingsTableViewController(
              isModallyPresented: true,
              braveCoreMain: bvc.profileController,
              windowProtection: bvc.windowProtection
            )
          } else {
            controller = SyncWelcomeViewController(
              braveCore: bvc.profileController,
              windowProtection: bvc.windowProtection,
              isModallyPresented: true
            )
          }
          let container = UINavigationController(rootViewController: controller)
          bvc.presentedViewController?.present(container, animated: true)
        }
      )
    )
    bvc.present(controller, animated: true)
  }

  func openDownloads(_ completion: @escaping (Bool) -> Void) {
    UIApplication.shared.openBraveDownloadsFolder(completion)
  }

  @MainActor
  func openHistory(isModal: Bool = false) {
    guard let bvc = bvc else { return }
    let vc = UIHostingController(
      rootView: HistoryView(
        model: HistoryModel(
          api: bvc.profileController.historyAPI,
          tabManager: bvc.tabManager,
          toolbarUrlActionsDelegate: bvc,
          dismiss: { [weak bvc] in bvc?.dismiss(animated: true) },
          askForAuthentication: bvc.askForLocalAuthentication
        )
      )
    )
    bvc.present(vc, animated: true)
  }

  func openVPNBuyScreen(iapObserver: BraveVPNInAppPurchaseObserver) {
    guard BraveVPN.vpnState.isPaywallEnabled else { return }

    let vpnPaywallView = BraveVPNPaywallView(
      openVPNAuthenticationInNewTab: { [weak bvc] in
        guard let bvc = bvc else { return }

        bvc.popToBVC()

        bvc.openURLInNewTab(
          .brave.braveVPNRefreshCredentials,
          isPrivate: bvc.privateBrowsingManager.isPrivateBrowsing,
          isPrivileged: false
        )
      },
      openDirectCheckoutInNewTab: { [weak bvc] in
        guard let bvc else { return }
        bvc.popToBVC()
        bvc.openURLInNewTab(
          .brave.braveVPNCheckoutURL,
          isPrivate: bvc.privateBrowsingManager.isPrivateBrowsing,
          isPrivileged: false
        )
      },
      openLearnMoreInNewTab: { [weak bvc] in
        guard let bvc else { return }
        bvc.popToBVC()
        bvc.openURLInNewTab(
          .brave.braveVPNLearnMoreURL,
          isPrivate: bvc.privateBrowsingManager.isPrivateBrowsing,
          isPrivileged: false
        )
      },
      installVPNProfile: { [weak bvc] in
        guard let bvc = bvc else { return }
        bvc.popToBVC()
        bvc.present(UIHostingController(rootView: InstallVPNProfileView()), animated: true)
      }
    )

    let vpnPaywallHostingVC = UIHostingController(rootView: vpnPaywallView)
    bvc?.present(vpnPaywallHostingVC, animated: true)
  }

  func openShareSheet() {
    guard let bvc = bvc else { return }
    dismissView()

    guard let tab = bvc.tabManager.selectedTab, let url = tab.visibleURL else { return }

    // Cancel any previous request that has not yet presented its share sheet
    shareTask?.cancel()
    shareTask = Task { @MainActor [weak bvc, weak tab] in
      guard let bvc, let tab else { return }
      @MainActor func share(url: URL, document: ShareableDocument? = nil) {
        bvc.presentActivityViewController(
          url,
          tab: url.isFileURL ? nil : tab,
          source: .init(
            view: bvc.view,
            rect: bvc.view.convert(
              bvc.topToolbar.menuButton.frame,
              from: bvc.topToolbar.menuButton.superview
            ),
            arrowDirection: [.up]
          ),
          onDismiss: {
            // Retain the document until the share sheet is dismissed which deletes the file
            _ = document
          }
        )
      }

      if let document = tab.shareableDocumentHelper?.document {
        // The user may have switched tabs or navigated while the document was downloading
        @MainActor func isStillCurrent() -> Bool {
          !Task.isCancelled && bvc.tabManager.selectedTab === tab
            && tab.shareableDocumentHelper?.document === document
        }
        do {
          let fileURL = try await document.fileURL()
          guard isStillCurrent() else { return }
          share(url: fileURL, document: document)
        } catch is CancellationError {
          return
        } catch {
          guard isStillCurrent() else { return }
          // Fallback to sharing the web URL if the document could not be downloaded
          share(url: url)
        }
      } else if let readerSourceURL = url.self.decodeEmbeddedInternalURL(for: .readermode) {
        // We want to decode the underlying url that generated the reader mode file and share that instead
        // This way we avoid sharing a url of a local file
        share(url: readerSourceURL)
      } else {
        // Otherwise share the tab url
        share(url: url)
      }
    }
  }

  func openPlaylist() {
    bvc?.openPlaylist(tab: nil, item: nil)
  }

  func openWallet() {
    bvc?.presentWallet()
  }

  @objc private func dismissView() {
    guard let bvc = bvc else { return }
    bvc.presentedViewController?.dismiss(animated: true)
  }
}
