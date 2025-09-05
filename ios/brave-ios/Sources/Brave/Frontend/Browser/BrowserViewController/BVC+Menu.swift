// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShared
import BraveUI
import BraveVPN
import BraveWallet
import BrowserMenu
import Data
import Foundation
import PlaylistUI
import Preferences
import Shared
import SwiftUI
import Web
import os.log

extension BrowserViewController {
  private var settingsController: SettingsViewController {
    let isPrivateMode = privateBrowsingManager.isPrivateBrowsing
    let keyringService = BraveWallet.KeyringServiceFactory.get(privateMode: isPrivateMode)
    let walletService = BraveWallet.ServiceFactory.get(privateMode: isPrivateMode)
    let rpcService = BraveWallet.JsonRpcServiceFactory.get(privateMode: isPrivateMode)
    let walletP3A = profileController.braveWalletAPI.walletP3A()

    var keyringStore: KeyringStore? = walletStore?.keyringStore
    if keyringStore == nil {
      if let keyringService = keyringService,
        let walletService = walletService,
        let rpcService = rpcService,
        let walletP3A
      {
        keyringStore = KeyringStore(
          keyringService: keyringService,
          walletService: walletService,
          rpcService: rpcService,
          walletP3A: walletP3A
        )
      }
    }

    var cryptoStore: CryptoStore? = walletStore?.cryptoStore
    if cryptoStore == nil {
      cryptoStore = CryptoStore.from(
        ipfsApi: profileController.ipfsAPI,
        walletP3A: walletP3A,
        privateMode: isPrivateMode
      )
    }

    let vc = SettingsViewController(
      profile: self.profile,
      tabManager: self.tabManager,
      feedDataSource: self.feedDataSource,
      rewards: self.rewards,
      windowProtection: self.windowProtection,
      p3aUtils: self.braveCore.p3aUtils,
      braveCore: self.profileController,
      localState: self.braveCore.localState,
      attributionManager: attributionManager,
      keyringStore: keyringStore,
      cryptoStore: cryptoStore
    )
    vc.settingsDelegate = self
    return vc
  }

  /// Presents Wallet without an origin (ex. from menu)
  func presentWallet() {
    guard let walletStore = self.walletStore ?? newWalletStore() else { return }
    walletStore.origin = nil
    let vc = WalletHostingViewController(
      walletStore: walletStore,
      webImageDownloader: profileController.webImageDownloader
    )
    vc.delegate = self
    self.dismiss(animated: true) {
      self.present(vc, animated: true)
    }
  }

  public func presentPlaylistController() {
    if !profileController.profile.prefs.isPlaylistAvailable {
      return
    }
    if PlaylistCoordinator.shared.isPlaylistControllerPresented {
      let alert = UIAlertController(
        title: Strings.PlayList.playlistAlreadyShowingTitle,
        message: Strings.PlayList.playlistAlreadyShowingBody,
        preferredStyle: .alert
      )
      alert.addAction(UIAlertAction(title: Strings.OKString, style: .default))
      dismiss(animated: true) {
        self.present(alert, animated: true)
      }
      return
    }

    // Retrieve the item and offset-time from the current tab's webview.
    let tab = self.tabManager.selectedTab
    PlaylistCoordinator.shared.getPlaylistController(tab: tab) {
      [weak self] playlistController in
      guard let self = self else { return }

      PlaylistP3A.recordUsage()

      self.dismiss(animated: true) {
        PlaylistCoordinator.shared.isPlaylistControllerPresented = true
        self.present(playlistController, animated: true)
      }
    }
  }

  // Present a popup when VPN server region has been changed
  private func presentVPNServerRegionPopup() {
    let controller = PopupViewController(
      rootView: BraveVPNRegionConfirmationView(
        country: BraveVPN.serverLocationDetailed.country,
        city: BraveVPN.serverLocationDetailed.city,
        countryISOCode: BraveVPN.serverLocation.isoCode
      ),
      isDismissable: true
    )
    if let presentedViewController {
      presentedViewController.present(controller, animated: true)
    } else {
      present(controller, animated: true)
    }
    Timer.scheduledTimer(withTimeInterval: 2, repeats: false) { [weak controller] _ in
      controller?.dismiss(animated: true)
    }
  }

  func presentBrowserMenu(
    from sourceView: UIView,
    activities: [UIActivity],
    tab: (any TabState)?,
    pageURL: URL?
  ) {
    var actions: [Action] = []
    if profileController.profile.prefs.isBraveVPNAvailable {
      actions.append(vpnMenuAction)
    }
    actions.append(contentsOf: destinationMenuActions(for: pageURL))
    actions.append(contentsOf: pageActions(for: pageURL, tab: tab))
    var pageActivities: Set<Action> = Set(
      activities
        .compactMap { activity in
          guard let id = (activity as? MenuActivity)?.id,
            let actionID = Action.Identifier.allPageActivites.first(where: { $0.id == id })
          else {
            return nil
          }
          return (activity, actionID)
        }
        .map { (activity: UIActivity, actionID: Action.Identifier) in
          .init(id: actionID) { @MainActor [unowned self] _ in
            self.dismiss(animated: true) {
              activity.perform()
            }
            return .none
          }
        }
    )
    if let tab,
      let requestDesktopPageActivity = pageActivities.first(where: { $0.id == .requestDesktopSite })
    {
      // Remove the UIActivity version and replace it with a manual version.
      // The request desktop activity is special in the sense that it is dynamic based on the
      // current tab user agent, but we don't use rely on the UIActivity information to populate
      // actions in the new menu UI, so this replaces it with how we would compose it manually
      pageActivities.remove(requestDesktopPageActivity)
      pageActivities.insert(
        .init(
          id: .requestDesktopSite,
          title: tab.currentUserAgentType == .desktop
            ? Strings.appMenuViewMobileSiteTitleString : nil,
          image: tab.currentUserAgentType == .desktop ? "leo.smartphone" : nil,
          handler: { @MainActor [unowned self, weak tab] _ in
            tab?.switchUserAgent()
            self.dismiss(animated: true)
            return .none
          }
        )
      )
    }
    // Sets up empty actions for any page actions that weren't setup as UIActivity's excluding any
    // that should be hidden due to admin policies
    var pageActivitiesRemovedByAdminPolicies: Set<Action.Identifier> = []
    if !profileController.profile.prefs.isBraveNewsAvailable {
      pageActivitiesRemovedByAdminPolicies.insert(.addSourceNews)
    }
    let remainingPageActivities: [Action] = Action.ID.allPageActivites
      .subtracting(pageActivities.map(\.id))
      .subtracting(pageActivitiesRemovedByAdminPolicies)
      .map { .init(id: $0, attributes: .disabled) }
    actions.append(contentsOf: pageActivities)
    actions.append(contentsOf: remainingPageActivities)
    let browserMenu = BrowserMenuController(
      actions: actions,
      handlePresentation: { [unowned self] action in
        switch action {
        case .settings:
          let vc = self.settingsController
          self.dismiss(animated: true) {
            self.presentSettingsNavigation(with: vc)
          }
        case .vpnRegionPicker:
          let vc = UIHostingController(
            rootView: BraveVPNRegionListView(
              onServerRegionSet: { _ in
                self.presentVPNServerRegionPopup()
              }
            )
          )
          vc.title = Strings.VPN.vpnRegionListServerScreenTitle
          self.dismiss(animated: true) {
            self.presentSettingsNavigation(with: vc)
          }
        }
      }
    )
    if UIDevice.current.userInterfaceIdiom == .pad {
      browserMenu.modalPresentationStyle = .popover
    }
    browserMenu.popoverPresentationController?.sourceView = sourceView
    browserMenu.popoverPresentationController?.sourceRect = sourceView.bounds
    browserMenu.popoverPresentationController?.popoverLayoutMargins = .init(equalInset: 4)
    browserMenu.popoverPresentationController?.permittedArrowDirections = [.up, .down]
    present(browserMenu, animated: true)
    return
  }

  private func pageActions(for pageURL: URL?, tab: (any TabState)?) -> [Action] {
    var actions: [Action] = [
      .init(id: .share) { @MainActor [unowned self] _ in
        self.dismiss(animated: true) {
          self.tabToolbarDidPressShare()
        }
        return .none
      },
      .init(id: .addBookmark) { @MainActor [unowned self] _ in
        self.dismiss(animated: true) {
          self.openAddBookmark()
        }
        return .none
      },
      .init(
        id: .toggleNightMode,
        state: Preferences.General.nightModeEnabled.value
      ) { @MainActor action in
        var actionCopy = action
        Preferences.General.nightModeEnabled.value.toggle()
        actionCopy.state = Preferences.General.nightModeEnabled.value
        return .updateAction(actionCopy)
      },
    ]
    if profileController.profile.prefs.isPlaylistAvailable {
      let playlistActivity = addToPlayListActivityItem ?? openInPlaylistActivityItem
      let isPlaylistItemAdded = openInPlaylistActivityItem != nil
      actions.append(
        .init(
          id: .addToPlaylist,
          title: isPlaylistItemAdded ? Strings.PlayList.toastAddedToPlaylistTitle : nil,
          image: isPlaylistItemAdded ? "leo.product.playlist-added" : nil,
          attributes: playlistActivity?.enabled == true ? [] : .disabled
        ) { @MainActor [unowned self] action in
          let playlistActivity = addToPlayListActivityItem ?? openInPlaylistActivityItem
          let isPlaylistItemAdded = openInPlaylistActivityItem != nil
          guard let item = playlistActivity?.item else { return .none }
          if !isPlaylistItemAdded {
            // Add to playlist
            // TODO: Need to be able to return something that will update the underlying action
            let addedItem = await withCheckedContinuation { continuation in
              self.addToPlaylist(item: item) { didAddItem in
                continuation.resume(returning: didAddItem)
              }
            }
            if addedItem {
              var actionCopy = action
              actionCopy.title = Strings.PlayList.toastAddedToPlaylistTitle
              actionCopy.image = "leo.product.playlist-added"
              return .updateAction(actionCopy)
            }
          } else {
            self.dismiss(animated: true) {
              self.openPlaylist(
                tab: self.tabManager.selectedTab,
                item: item
              )
            }
          }
          return .none
        }
      )
    }
    if BraveCore.FeatureList.kBraveShredFeature.enabled {
      let isShredAvailable = tabManager.selectedTab?.visibleURL?.isShredAvailable ?? false
      actions.append(
        .init(id: .shredData, attributes: isShredAvailable ? [] : [.disabled]) {
          @MainActor [unowned self] _ in
          self.dismiss(animated: true) {
            guard let tab = self.tabManager.selectedTab, let url = tab.visibleURL else { return }
            let alert = UIAlertController.shredDataAlert(url: url) { _ in
              self.shredData(for: url, in: tab)
            }
            self.present(alert, animated: true)
          }
          return .none
        }
      )
    }
    let printFormatter = tab?.viewPrintFormatter
    actions.append(
      .init(id: .print, attributes: printFormatter == nil ? .disabled : []) {
        @MainActor [unowned self] _ in
        self.dismiss(animated: true) {
          let printController = UIPrintInteractionController.shared
          printController.printFormatter = printFormatter
          printController.present(animated: true)
        }
        return .none
      }
    )
    if pageURL == nil {
      for index in actions.indices {
        actions[index].attributes.insert(.disabled)
      }
    }
    return actions
  }

  private var vpnMenuAction: Action {
    func alertForExpiredState() -> UIAlertController? {
      if !BraveSkusManager.keepShowingSessionExpiredState {
        return nil
      }
      return BraveSkusManager.sessionExpiredStateAlert(loginCallback: { _ in
        self.openURLInNewTab(
          .brave.account,
          isPrivate: self.privateBrowsingManager.isPrivateBrowsing,
          isPrivileged: false
        )
      })
    }

    let vpnState = BraveVPN.vpnState
    switch vpnState {
    case .notPurchased, .expired:
      return .init(id: .vpn) { @MainActor [unowned self] _ in
        if !BraveVPNProductInfo.isComplete {
          // Reattempt to connect to the App Store to get VPN prices.
          vpnProductInfo.load()
          return .none
        }

        if let alert = alertForExpiredState() {
          self.dismiss(animated: true) {
            self.present(alert, animated: true)
          }
          return .none
        }

        // Expired Subcriptions can cause glitch because of connect on demand
        // Disconnect VPN before showing Purchase
        BraveVPN.disconnect(skipChecks: true)
        guard BraveVPN.vpnState.isPaywallEnabled else { return .none }

        let vpnPaywallView = BraveVPNPaywallView(
          openVPNAuthenticationInNewTab: { [weak self] in
            guard let self else { return }
            self.popToBVC()
            self.openURLInNewTab(
              .brave.braveVPNRefreshCredentials,
              isPrivate: self.privateBrowsingManager.isPrivateBrowsing,
              isPrivileged: false
            )
          },
          openDirectCheckoutInNewTab: { [weak self] in
            guard let self else { return }
            popToBVC()
            openURLInNewTab(
              .brave.braveVPNCheckoutURL,
              isPrivate: self.privateBrowsingManager.isPrivateBrowsing,
              isPrivileged: false
            )
          },
          openLearnMoreInNewTab: { [weak self] in
            guard let self else { return }
            popToBVC()
            openURLInNewTab(
              .brave.braveVPNLearnMoreURL,
              isPrivate: self.privateBrowsingManager.isPrivateBrowsing,
              isPrivileged: false
            )
          },
          installVPNProfile: { [weak self] in
            guard let self else { return }
            self.popToBVC()
            self.openInsideSettingsNavigation(with: BraveVPNInstallViewController())
          }
        )

        let vc = BraveVPNPaywallHostingController(paywallView: vpnPaywallView)
        let container = UINavigationController(rootViewController: vc)
        self.dismiss(animated: true) {
          self.present(container, animated: true)
        }
        return .none
      }
    case .purchased:
      let isConnected = BraveVPN.isConnected || BraveVPN.isConnecting
      return .init(
        id: .vpn,
        title: isConnected ? Strings.VPN.vpnOnMenuButtonTitle : Strings.VPN.vpnOffMenuButtonTitle,
        state: isConnected
      ) { @MainActor [unowned self] _ in
        if let alert = alertForExpiredState() {
          self.dismiss(animated: true) {
            self.present(alert, animated: true)
          }
          return .none
        }

        if BraveVPN.isConnected || BraveVPN.isConnecting {
          await withCheckedContinuation { continuation in
            BraveVPN.disconnect { error in
              continuation.resume()
            }
          }
        } else {
          await withCheckedContinuation { continuation in
            BraveVPN.reconnect { success in
              continuation.resume()
            }
          }
          // FIXME: VPN activity donation
          // Donate Enable VPN Activity for suggestions
          // let enableVPNActivity = ActivityShortcutManager.shared.createShortcutActivity(
          //   type: .enableBraveVPN
          // )
          // Does this need to be attached to the menu specifically?
          // browserMenuController.userActivity = enableVPNActivity
          // enableVPNActivity.becomeCurrent()
        }
        try? await Task.sleep(for: .milliseconds(100))
        return .updateAction(vpnMenuAction)
      }
    }
  }

  private func destinationMenuActions(for pageURL: URL?) -> [Action] {
    let isPrivateBrowsing = privateBrowsingManager.isPrivateBrowsing
    var actions: [Action] = [
      .init(id: .bookmarks) { @MainActor [unowned self] _ in
        let vc = BookmarksViewController(
          folder: bookmarkManager.lastVisitedFolder(),
          bookmarkManager: bookmarkManager,
          isPrivateBrowsing: privateBrowsingManager.isPrivateBrowsing
        )
        vc.toolbarUrlActionsDelegate = self
        let container = UINavigationController(rootViewController: vc)
        self.dismiss(animated: true) {
          self.present(container, animated: true)
        }
        return .none
      },
      .init(id: .history) { @MainActor [unowned self] _ in
        let vc = UIHostingController(
          rootView: HistoryView(
            model: HistoryModel(
              api: self.profileController.historyAPI,
              tabManager: self.tabManager,
              toolbarUrlActionsDelegate: self,
              dismiss: { [weak self] in self?.dismiss(animated: true) },
              askForAuthentication: self.askForLocalAuthentication
            )
          )
        )
        self.dismiss(animated: true) {
          self.present(vc, animated: true)
        }
        return .none
      },
      .init(id: .downloads) { @MainActor [unowned self] _ in
        UIApplication.shared.openBraveDownloadsFolder { success in
          if !success {
            self.dismiss(animated: true) {
              self.displayOpenDownloadsError()
            }
          }
        }
        return .none
      },
    ]
    if profileController.profile.prefs.isPlaylistAvailable {
      actions.append(
        .init(id: .playlist) { @MainActor [unowned self] _ in
          // presentPlaylistController already handles dismiss + present
          self.presentPlaylistController()
          return .none
        }
      )
    }
    if profileController.braveWalletAPI.isAllowed {
      actions.append(
        .init(id: .braveWallet) { @MainActor [unowned self] _ in
          // Present wallet already handles dismiss + present
          self.presentWallet()
          return .none
        }
      )
    }
    if AIChatUtils.isAIChatEnabled(for: profileController.profile.prefs) {
      actions.append(
        .init(
          id: .braveLeo,
          attributes: isPrivateBrowsing ? .disabled : []
        ) { @MainActor [unowned self] _ in
          self.dismiss(animated: true) {
            self.openBraveLeo()
          }
          return .none
        }
      )
    }
    if profileController.profile.prefs.isBraveTalkAvailable {
      actions.append(
        .init(id: .braveTalk) { @MainActor [unowned self] _ in
          self.dismiss(animated: true) {
            guard let url = URL(string: "https://talk.brave.com/") else { return }
            self.popToBVC()
            if pageURL == nil {
              // Already on NTP
              self.finishEditingAndSubmit(url)
            } else {
              self.openURLInNewTab(url, isPrivileged: false)
            }
          }
          return .none
        }
      )
    }
    if profileController.profile.prefs.isBraveNewsAvailable {
      actions.append(
        .init(id: .braveNews) { @MainActor [unowned self] _ in
          self.dismiss(animated: true) {
            if pageURL == nil,
              let newTabPageController = self.tabManager.selectedTab?.newTabPageViewController
            {
              // Already on NTP
              newTabPageController.scrollToBraveNews()
            } else {
              // Make a new tab and scroll to it
              self.openBlankNewTab(
                attemptLocationFieldFocus: false,
                isPrivate: false,
                isExternal: true
              )
              self.popToBVC()
              if let newTabPageController = self.tabManager.selectedTab?.newTabPageViewController {
                newTabPageController.scrollToBraveNews()
              }
            }
          }
          return .none
        }
      )
    }
    return actions
  }

}
