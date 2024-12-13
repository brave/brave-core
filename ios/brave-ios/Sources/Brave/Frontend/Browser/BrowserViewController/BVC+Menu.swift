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
import os.log

extension BrowserViewController {
  func featuresMenuSection(_ menuController: MenuViewController) -> some View {
    VStack(alignment: .leading, spacing: 5) {
      VPNMenuButton(
        retryStateActive: Preferences.VPN.vpnReceiptStatus.value
          == BraveVPN.ReceiptResponse.Status.retryPeriod.rawValue,
        vpnProductInfo: self.vpnProductInfo,
        displayVPNDestination: { [unowned self] vc in
          self.dismiss(animated: true) {
            self.present(UINavigationController(rootViewController: vc), animated: true)
          }
        },
        enableInstalledVPN: { [unowned menuController] in
          // Donate Enable VPN Activity for suggestions
          let enableVPNActivity = ActivityShortcutManager.shared.createShortcutActivity(
            type: .enableBraveVPN
          )
          menuController.userActivity = enableVPNActivity
          enableVPNActivity.becomeCurrent()
        },
        displayAlert: { [unowned menuController] alert in
          menuController.present(alert, animated: true)
        },
        openURL: { [weak self] url in
          guard let self = self else { return }

          popToBVC()

          self.openURLInNewTab(
            url,
            isPrivate: self.privateBrowsingManager.isPrivateBrowsing,
            isPrivileged: false
          )
        },
        installVPNProfile: { [unowned self] in
          self.popToBVC(isAnimated: true) {
            self.present(BraveVPNInstallViewController(), animated: true)
          }
        }
      )

      // Region Button is populated without current selected detail title for features menu
      RegionMenuButton(
        settingTitleEnabled: false,
        regionSelectAction: { [unowned menuController] in
          let vpnRegionListView = BraveVPNRegionListView(
            onServerRegionSet: { _ in
              self.presentVPNServerRegionPopup()
            }
          )
          let vc = UIHostingController(rootView: vpnRegionListView)
          vc.title = Strings.VPN.vpnRegionListServerScreenTitle

          menuController.pushInnerMenu(vc)
        }
      )
    }
  }

  func privacyFeaturesMenuSection(_ menuController: MenuViewController) -> some View {
    VStack(alignment: .leading, spacing: 5) {
      Text(Strings.OptionsMenu.menuSectionTitle.capitalized)
        .font(.callout.weight(.semibold))
        .foregroundColor(Color(.braveLabel))
        .padding(.horizontal, 14)
        .padding(.bottom, 5)

      VPNMenuButton(
        retryStateActive: Preferences.VPN.vpnReceiptStatus.value
          == BraveVPN.ReceiptResponse.Status.retryPeriod.rawValue,
        vpnProductInfo: self.vpnProductInfo,
        description: Strings.OptionsMenu.braveVPNItemDescription,
        displayVPNDestination: { [unowned self] vc in
          self.dismiss(animated: true) {
            self.present(UINavigationController(rootViewController: vc), animated: true)
          }
        },
        enableInstalledVPN: { [unowned menuController] in
          // Donate Enable VPN Activity for suggestions
          let enableVPNActivity = ActivityShortcutManager.shared.createShortcutActivity(
            type: .enableBraveVPN
          )
          menuController.userActivity = enableVPNActivity
          enableVPNActivity.becomeCurrent()
        },
        displayAlert: { [unowned self] alert in
          self.popToBVC()
          self.present(alert, animated: true)
        },
        openURL: { [unowned self] url in
          self.popToBVC()
          self.openURLInNewTab(
            url,
            isPrivate: self.privateBrowsingManager.isPrivateBrowsing,
            isPrivileged: false
          )
        },
        installVPNProfile: { [unowned self] in
          self.popToBVC(isAnimated: true) {
            self.present(BraveVPNInstallViewController(), animated: true)
          }
        }
      )

      // Region Button is populated including the details for privacy feature menu
      RegionMenuButton(
        regionSelectAction: { [unowned menuController] in
          let vpnRegionListView = BraveVPNRegionListView(
            onServerRegionSet: { _ in
              self.presentVPNServerRegionPopup()
            }
          )
          let vc = UIHostingController(rootView: vpnRegionListView)
          vc.title = Strings.VPN.vpnRegionListServerScreenTitle

          menuController.pushInnerMenu(vc)
        }
      )

      Divider()

      MenuItemFactory.button(
        for: .playlist(subtitle: Strings.OptionsMenu.bravePlaylistItemDescription)
      ) { [weak self] in
        guard let self = self else { return }
        self.presentPlaylistController()
      }

      // Add Brave Talk and News options only in normal browsing
      if !privateBrowsingManager.isPrivateBrowsing {
        // Show Brave News if it is first launch and after first launch If the new is enabled
        if Preferences.General.isFirstLaunch.value
          || (!Preferences.General.isFirstLaunch.value && Preferences.BraveNews.isEnabled.value)
        {
          MenuItemFactory.button(for: .news) { [weak self] in
            guard let self = self,
              let newTabPageController = self.tabManager.selectedTab?.newTabPageViewController
            else {
              return
            }

            self.popToBVC()
            newTabPageController.scrollToBraveNews()
          }
        }
        MenuItemFactory.button(for: .talk) {
          guard let url = URL(string: "https://talk.brave.com/") else { return }

          self.popToBVC()
          self.finishEditingAndSubmit(url)
        }
      }

      MenuItemFactory.button(for: .wallet(subtitle: Strings.OptionsMenu.braveWalletItemDescription))
      { [unowned self] in
        self.presentWallet()
      }

      // Add Brave-Leo options only in normal browsing
      if !privateBrowsingManager.isPrivateBrowsing && FeatureList.kAIChat.enabled {
        MenuItemFactory.button(for: .leo) { [unowned self] in
          self.popToBVC()
          self.openBraveLeo()
        }
      }
    }
    .fixedSize(horizontal: false, vertical: true)
    .padding(.top, 10)
    .padding(.bottom, 5)
  }

  func destinationMenuSection(
    _ menuController: MenuViewController,
    isShownOnWebPage: Bool
  ) -> some View {
    VStack(spacing: 0) {
      MenuItemFactory.button(for: .bookmarks) { [unowned self, unowned menuController] in
        let vc = BookmarksViewController(
          folder: bookmarkManager.lastVisitedFolder(),
          bookmarkManager: bookmarkManager,
          isPrivateBrowsing: privateBrowsingManager.isPrivateBrowsing
        )
        vc.toolbarUrlActionsDelegate = self
        menuController.presentInnerMenu(vc)
      }
      MenuItemFactory.button(for: .history) { [unowned self, unowned menuController] in
        let vc = UIHostingController(
          rootView: HistoryView(
            model: HistoryModel(
              api: self.braveCore.historyAPI,
              tabManager: self.tabManager,
              toolbarUrlActionsDelegate: self,
              dismiss: { [weak self] in self?.dismiss(animated: true) },
              askForAuthentication: self.askForLocalAuthentication
            )
          )
        )
        menuController.pushInnerMenu(vc)
      }
      MenuItemFactory.button(for: .downloads) {
        UIApplication.shared.openBraveDownloadsFolder { success in
          if !success {
            self.displayOpenDownloadsError()
          }
        }
      }

      if isShownOnWebPage {
        MenuItemFactory.button(for: .wallet()) { [weak self] in
          self?.presentWallet()
        }
        MenuItemFactory.button(for: .playlist()) { [weak self] in
          guard let self = self else { return }
          self.presentPlaylistController()
        }
      }
      MenuItemFactory.button(for: .settings) { [unowned self, unowned menuController] in
        menuController.pushInnerMenu(self.settingsController)
      }
    }
  }

  private var settingsController: SettingsViewController {
    let isPrivateMode = privateBrowsingManager.isPrivateBrowsing
    let keyringService = BraveWallet.KeyringServiceFactory.get(privateMode: isPrivateMode)
    let walletService = BraveWallet.ServiceFactory.get(privateMode: isPrivateMode)
    let rpcService = BraveWallet.JsonRpcServiceFactory.get(privateMode: isPrivateMode)
    let walletP3A = braveCore.braveWalletAPI.walletP3A()

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
        ipfsApi: braveCore.ipfsAPI,
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
      braveCore: self.braveCore,
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
      webImageDownloader: braveCore.webImageDownloader
    )
    vc.delegate = self
    self.dismiss(animated: true) {
      self.present(vc, animated: true)
    }
  }

  public func presentPlaylistController() {
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

    // Present existing playlist controller
    if let playlistController = PlaylistCoordinator.shared.playlistController {
      PlaylistP3A.recordUsage()

      dismiss(animated: true) {
        PlaylistCoordinator.shared.isPlaylistControllerPresented = true
        self.present(playlistController, animated: true)
      }
    } else {
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

  struct PageActionsMenuSection: View {
    var browserViewController: BrowserViewController
    var tabURL: URL
    var activities: [UIActivity]

    @State private var playlistItemAdded: Bool = false

    private var playlistActivity: (enabled: Bool, item: PlaylistInfo?)? {
      browserViewController.addToPlayListActivityItem
        ?? browserViewController.openInPlaylistActivityItem
    }

    private var isPlaylistItemAdded: Bool {
      browserViewController.openInPlaylistActivityItem != nil
    }

    var body: some View {
      VStack(alignment: .leading, spacing: 0) {
        MenuTabDetailsView(tab: browserViewController.tabManager.selectedTab, url: tabURL)
        VStack(spacing: 0) {
          if let activity = playlistActivity, activity.enabled, let item = activity.item {
            PlaylistMenuButton(isAdded: isPlaylistItemAdded) {
              if !isPlaylistItemAdded {
                // Add to playlist
                browserViewController.addToPlaylist(item: item) { didAddItem in
                  Logger.module.debug("Playlist Item Added")
                  if didAddItem {
                    playlistItemAdded = true
                  }
                }
              } else {
                browserViewController.dismiss(animated: true) {
                  browserViewController.openPlaylist(
                    tab: browserViewController.tabManager.selectedTab,
                    item: item
                  )
                }
              }
            }
            .animation(.default, value: playlistItemAdded)
          }

          // Add Brave-Leo options only in normal browsing
          if !browserViewController.tabManager.privateBrowsingManager.isPrivateBrowsing
            && FeatureList.kAIChat.enabled
          {
            MenuItemButton(
              icon: Image(braveSystemName: "leo.product.brave-leo"),
              title: Strings.leoMenuItem
            ) {
              browserViewController.dismiss(animated: true)
              browserViewController.openBraveLeo()
            }
          }

          MenuItemButton(
            icon: Image(braveSystemName: "leo.share.macos"),
            title: Strings.shareWithMenuItem
          ) {
            browserViewController.dismiss(animated: true)
            browserViewController.tabToolbarDidPressShare()
          }
          MenuItemButton(
            icon: Image(braveSystemName: "leo.shred.data"),
            title: Strings.Shields.shredSiteData
          ) {
            browserViewController.dismiss(animated: true) {
              guard let tab = self.browserViewController.tabManager.selectedTab,
                let url = tab.url
              else { return }
              let alert = UIAlertController.shredDataAlert(url: url) { _ in
                self.browserViewController.shredData(for: url, in: tab)
              }
              browserViewController.present(alert, animated: true)
            }
          }
          NightModeMenuButton(dismiss: {
            browserViewController.dismiss(animated: true)
          })
          MenuItemButton(
            icon: Image(braveSystemName: "leo.browser.bookmark-add"),
            title: Strings.addToMenuItem
          ) {
            browserViewController.dismiss(animated: true) {
              browserViewController.openAddBookmark()
            }
          }
          ForEach(activities.compactMap({ $0 as? MenuActivity }), id: \.activityTitle) { activity in
            MenuItemButton(icon: activity.menuImage, title: activity.activityTitle ?? "") {
              browserViewController.dismiss(animated: true) {
                activity.perform()
              }
            }
          }
        }
      }
    }
  }

  struct MenuTabDetailsView: View {
    @SwiftUI.Environment(\.colorScheme) var colorScheme: ColorScheme
    weak var tab: Tab?
    var url: URL

    var body: some View {
      VStack(alignment: .leading, spacing: 2) {
        if let tab = tab {
          Text(verbatim: tab.displayTitle)
            .font(.callout)
            .fontWeight(.medium)
            .lineLimit(1)
            .foregroundColor(Color(.braveLabel))
        }

        URLElidedText(
          text: URLFormatter.formatURLOrigin(
            forDisplayOmitSchemePathAndTrivialSubdomains: url.absoluteString
          )
        )
        .font(.footnote)
        .lineLimit(1)
        .foregroundColor(Color(.secondaryBraveLabel))
        .truncationMode(.head)
      }
      .padding(.horizontal, 14)
      .padding(.vertical, 6)
    }
  }
}

extension BrowserViewController {
  func presentBrowserMenu(
    from sourceView: UIView,
    activities: [UIActivity],
    tab: Tab?,
    pageURL: URL?,
    webView: WKWebView?
  ) {
    var actions: [Action] = []
    actions.append(vpnMenuAction)
    actions.append(contentsOf: destinationMenuActions(for: pageURL))
    actions.append(contentsOf: pageActions(for: pageURL, webView: webView))
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
          title: tab.isDesktopSite ? Strings.appMenuViewMobileSiteTitleString : nil,
          image: tab.isDesktopSite ? "leo.smartphone" : nil,
          handler: { @MainActor [unowned self, weak tab] _ in
            tab?.switchUserAgent()
            self.dismiss(animated: true)
            return .none
          }
        )
      )
    }
    // Sets up empty actions for any page actions that weren't setup as UIActivity's
    let remainingPageActivities: [Action] = Action.ID.allPageActivites
      .subtracting(pageActivities.map(\.id))
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

  private func pageActions(for pageURL: URL?, webView: WKWebView?) -> [Action] {
    let playlistActivity = addToPlayListActivityItem ?? openInPlaylistActivityItem
    let isPlaylistItemAdded = openInPlaylistActivityItem != nil
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
      .init(id: .shredData) { @MainActor [unowned self] _ in
        self.dismiss(animated: true) {
          guard let tab = self.tabManager.selectedTab, let url = tab.url else { return }
          let alert = UIAlertController.shredDataAlert(url: url) { _ in
            self.shredData(for: url, in: tab)
          }
          self.present(alert, animated: true)
        }
        return .none
      },
      .init(id: .print) { @MainActor [unowned self, weak webView] _ in
        guard let webView else { return .none }
        self.dismiss(animated: true) {
          let printController = UIPrintInteractionController.shared
          printController.printFormatter = webView.viewPrintFormatter()
          printController.present(animated: true)
        }
        return .none
      },
    ]
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
    return [
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
              api: self.braveCore.historyAPI,
              tabManager: self.tabManager,
              toolbarUrlActionsDelegate: self,
              dismiss: { [weak self] in self?.dismiss(animated: true) },
              askForAuthentication: self.askForLocalAuthentication
            )
          )
        )
        let container = UINavigationController(rootViewController: vc)
        // TODO: Move the button into HistoryView when old menu is removed
        vc.navigationItem.rightBarButtonItem = UIBarButtonItem(
          systemItem: .done,
          primaryAction: .init(handler: { [unowned self] _ in
            self.dismiss(animated: true)
          })
        )
        self.dismiss(animated: true) {
          self.present(container, animated: true)
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
      .init(id: .braveWallet) { @MainActor [unowned self] _ in
        self.dismiss(animated: true) {
          self.presentWallet()
        }
        return .none
      },
      .init(
        id: .braveLeo,
        attributes: isPrivateBrowsing ? .disabled : []
      ) { @MainActor [unowned self] _ in
        self.dismiss(animated: true) {
          self.openBraveLeo()
        }
        return .none
      },
      .init(id: .playlist) { @MainActor [unowned self] _ in
        self.dismiss(animated: true) {
          self.presentPlaylistController()
        }
        return .none
      },
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
      },
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
      },
    ]
  }

}
