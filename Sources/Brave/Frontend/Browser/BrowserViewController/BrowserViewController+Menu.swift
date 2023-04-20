// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import SwiftUI
import BraveUI
import Preferences
import Shared
import Data
import BraveWallet
import BraveCore
import os.log

extension BrowserViewController {
  func featuresMenuSection(_ menuController: MenuViewController) -> some View {
    VStack(spacing: 0) {
      VPNMenuButton(
        vpnProductInfo: self.vpnProductInfo,
        displayVPNDestination: { [unowned self] vc in
          (self.presentedViewController as? MenuViewController)?
            .pushInnerMenu(vc)
        },
        enableInstalledVPN: { [unowned menuController] in
          // Donate Enable VPN Activity for suggestions
          let enableVPNActivity = ActivityShortcutManager.shared.createShortcutActivity(type: .enableBraveVPN)
          menuController.userActivity = enableVPNActivity
          enableVPNActivity.becomeCurrent()
        }, displayAlert: { [unowned menuController] alert in
          menuController.present(alert, animated: true)
        }, openURL: { [weak self] url in
          self?.openURLInNewTab(url, isPrivate: PrivateBrowsingManager.shared.isPrivateBrowsing,
                               isPrivileged: false)
        })
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
        vpnProductInfo: self.vpnProductInfo,
        description: Strings.OptionsMenu.braveVPNItemDescription,
        displayVPNDestination: { [unowned self] vc in
          (self.presentedViewController as? MenuViewController)?
            .pushInnerMenu(vc)
        },
        enableInstalledVPN: { [unowned menuController] in
          // Donate Enable VPN Activity for suggestions
          let enableVPNActivity = ActivityShortcutManager.shared.createShortcutActivity(type: .enableBraveVPN)
          menuController.userActivity = enableVPNActivity
          enableVPNActivity.becomeCurrent()
        }, displayAlert: { [unowned self] alert in
          self.popToBVC()
          self.present(alert, animated: true)
        }, openURL: { [unowned self] url in
          self.openURLInNewTab(url,
                               isPrivate: PrivateBrowsingManager.shared.isPrivateBrowsing,
                               isPrivileged: false)
        }
      )

      MenuItemFactory.button(for: .playlist(subtitle: Strings.OptionsMenu.bravePlaylistItemDescription)) { [weak self] in
        guard let self = self else { return }
        self.presentPlaylistController()
      }

      // Add Brave Talk and News options only in normal browsing
      if !PrivateBrowsingManager.shared.isPrivateBrowsing {
        // Show Brave News if it is first launch and after first launch If the new is enabled
        if Preferences.General.isFirstLaunch.value || (!Preferences.General.isFirstLaunch.value && Preferences.BraveNews.isEnabled.value) {
          MenuItemFactory.button(for: .news) { [weak self] in
            guard let self = self, let newTabPageController = self.tabManager.selectedTab?.newTabPageViewController else {
              return
            }

            self.popToBVC()
            newTabPageController.scrollToBraveNews()
          }
        }
        MenuItemFactory.button(for: .talk) {
          guard let url = URL(string: "https://talk.brave.com/") else { return }

          self.popToBVC()
          self.finishEditingAndSubmit(url, visitType: .typed)
        }
      }
      
      MenuItemFactory.button(for: .wallet(subtitle: Strings.OptionsMenu.braveWalletItemDescription)) { [unowned self] in
        self.presentWallet()
      }
    }
    .fixedSize(horizontal: false, vertical: true)
    .padding(.top, 10)
    .padding(.bottom, 5)
  }

  func destinationMenuSection(_ menuController: MenuViewController, isShownOnWebPage: Bool) -> some View {
    VStack(spacing: 0) {
      MenuItemFactory.button(for: .bookmarks) { [unowned self, unowned menuController] in
        let vc = BookmarksViewController(
          folder: bookmarkManager.lastVisitedFolder(),
          bookmarkManager: bookmarkManager,
          isPrivateBrowsing: PrivateBrowsingManager.shared.isPrivateBrowsing)
        vc.toolbarUrlActionsDelegate = self
        menuController.presentInnerMenu(vc)
      }
      MenuItemFactory.button(for: .history) { [unowned self, unowned menuController] in
        let vc = HistoryViewController(
          isPrivateBrowsing: PrivateBrowsingManager.shared.isPrivateBrowsing,
          historyAPI: self.braveCore.historyAPI,
          tabManager: self.tabManager)
        vc.toolbarUrlActionsDelegate = self
        menuController.pushInnerMenu(vc)
      }
      MenuItemFactory.button(for: .downloads) {
        FileManager.default.openBraveDownloadsFolder { success in
          if !success {
            self.displayOpenDownloadsError()
          }
        }
      }

      if isShownOnWebPage {
        MenuItemFactory.button(for: .wallet()) {[weak self] in
          self?.presentWallet()
        }
        MenuItemFactory.button(for: .playlist()) { [weak self] in
          guard let self = self else { return }
          self.presentPlaylistController()
        }
      }
      MenuItemFactory.button(for: .settings) { [unowned self, unowned menuController] in
        let isPrivateMode = PrivateBrowsingManager.shared.isPrivateBrowsing
        let keyringService = BraveWallet.KeyringServiceFactory.get(privateMode: isPrivateMode)
        let walletService = BraveWallet.ServiceFactory.get(privateMode: isPrivateMode)
        let rpcService = BraveWallet.JsonRpcServiceFactory.get(privateMode: isPrivateMode)
        
        var keyringStore: KeyringStore?
        if let keyringService = keyringService,
           let walletService = walletService,
           let rpcService = rpcService {
          keyringStore = KeyringStore(
            keyringService: keyringService,
            walletService: walletService,
            rpcService: rpcService
          )
        }
        
        let cryptoStore = CryptoStore.from(ipfsApi: braveCore.ipfsAPI, privateMode: isPrivateMode)

        let vc = SettingsViewController(
          profile: self.profile,
          tabManager: self.tabManager,
          feedDataSource: self.feedDataSource,
          rewards: self.rewards,
          legacyWallet: self.legacyWallet,
          windowProtection: self.windowProtection,
          braveCore: self.braveCore,
          keyringStore: keyringStore,
          cryptoStore: cryptoStore
        )
        vc.settingsDelegate = self
        menuController.pushInnerMenu(vc)
      }
    }
  }

  func presentWallet() {
    guard let walletStore = self.walletStore ?? newWalletStore() else { return }
    let vc = WalletHostingViewController(walletStore: walletStore)
    vc.delegate = self
    self.dismiss(animated: true) {
      self.present(vc, animated: true)
    }
  }

  private func presentPlaylistController() {
    // Present existing playlist controller
    if let playlistController = PlaylistCarplayManager.shared.playlistController {
      dismiss(animated: true) {
        self.present(playlistController, animated: true)
      }
    } else {
      // Retrieve the item and offset-time from the current tab's webview.
      let tab = self.tabManager.selectedTab
      PlaylistCarplayManager.shared.getPlaylistController(tab: tab) { [weak self] playlistController in
        guard let self = self else { return }

        playlistController.modalPresentationStyle = .fullScreen

        self.dismiss(animated: true) {
          self.present(playlistController, animated: true)
        }
      }
    }
  }

  struct PageActionsMenuSection: View {
    var browserViewController: BrowserViewController
    var tabURL: URL
    var activities: [UIActivity]

    @State private var playlistItemAdded: Bool = false

    private var playlistActivity: (enabled: Bool, item: PlaylistInfo?)? {
      browserViewController.addToPlayListActivityItem ?? browserViewController.openInPlaylistActivityItem
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
                  let tab = browserViewController.tabManager.selectedTab

                  if let webView = tab?.webView {
                    PlaylistScriptHandler.getCurrentTime(webView: webView, nodeTag: item.tagId) { [weak browserViewController] currentTime in
                      browserViewController?.openPlaylist(tab: tab, item: item, playbackOffset: currentTime)
                    }
                  } else {
                    browserViewController.openPlaylist(tab: nil, item: item, playbackOffset: 0.0)
                  }
                }
              }
            }
            .animation(.default, value: playlistItemAdded)
          }
          MenuItemButton(icon: UIImage(named: "nav-share", in: .module, compatibleWith: nil)!.template, title: Strings.shareWithMenuItem) {
            browserViewController.dismiss(animated: true)
            browserViewController.tabToolbarDidPressShare()
          }
          NightModeMenuButton(dismiss: {
            browserViewController.dismiss(animated: true)
          })
          MenuItemButton(icon: UIImage(named: "menu-add-bookmark", in: .module, compatibleWith: nil)!.template, title: Strings.addToMenuItem) {
            browserViewController.dismiss(animated: true) {
              browserViewController.openAddBookmark()
            }
          }
          ForEach(activities, id: \.activityTitle) { activity in
            MenuItemButton(icon: activity.activityImage?.template ?? UIImage(), title: activity.activityTitle ?? "") {
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
        Text(verbatim: url.baseDomain ?? url.host ?? url.absoluteDisplayString)
          .font(.footnote)
          .lineLimit(1)
          .foregroundColor(Color(.secondaryBraveLabel))
      }
      .padding(.horizontal, 14)
      .padding(.vertical, 6)
    }
  }
}
