// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveWallet
import Foundation

extension Action.Identifier {

  public static let vpn: Self = .init(
    id: "ToggleVPN",
    title: Strings.ActionTitles.vpn,
    braveSystemImage: "leo.product.vpn",
    defaultRank: 100,
    defaultVisibility: .visible
  )

  public static let bookmarks: Self = .init(
    id: "Bookmarks",
    title: Strings.bookmarksMenuItem,
    braveSystemImage: "leo.product.bookmarks",
    defaultRank: 1600,
    defaultVisibility: .hidden
  )

  public static let downloads: Self = .init(
    id: "Downloads",
    title: Strings.downloadsMenuItem,
    braveSystemImage: "leo.folder.download",
    defaultRank: 1700,
    defaultVisibility: .hidden
  )

  public static let history: Self = .init(
    id: "History",
    title: Strings.historyMenuItem,
    braveSystemImage: "leo.history",
    defaultRank: 300,
    defaultVisibility: .visible
  )

  public static let braveLeo: Self = .init(
    id: "BraveLeo",
    title: Strings.ActionTitles.leoAIChat,
    braveSystemImage: "leo.product.brave-leo",
    defaultRank: 400,
    defaultVisibility: .visible
  )

  public static let braveNews: Self = .init(
    id: "BraveNews",
    title: Strings.ActionTitles.braveNews,
    braveSystemImage: "leo.product.brave-news",
    defaultRank: 2500,
    defaultVisibility: .hidden
  )

  public static let braveTalk: Self = .init(
    id: "BraveTalk",
    title: Strings.ActionTitles.braveTalk,
    braveSystemImage: "leo.product.brave-talk",
    defaultRank: 2400,
    defaultVisibility: .hidden
  )

  public static let braveWallet: Self = .init(
    id: "BraveWallet",
    title: Strings.Wallet.wallet,
    braveSystemImage: "leo.product.brave-wallet",
    defaultRank: 900,
    defaultVisibility: .visible
  )

  public static let playlist: Self = .init(
    id: "Playlist",
    title: Strings.ActionTitles.playlist,
    braveSystemImage: "leo.product.playlist",
    defaultRank: 500,
    defaultVisibility: .visible
  )

  public static let braveRewards: Self = .init(
    id: "BraveRewards",
    title: Strings.ActionTitles.rewards,
    braveSystemImage: "leo.product.bat-outline",
    defaultRank: 800,
    defaultVisibility: .visible
  )

  // MARK: - Page Actions

  public static let share: Self = .init(
    id: "Share",
    title: Strings.ActionTitles.share,
    braveSystemImage: "leo.share.macos",
    defaultRank: 600,
    defaultVisibility: .visible
  )
  public static let addToPlaylist: Self = .init(
    id: "AddToPlaylist",
    title: Strings.PlayList.toastAddToPlaylistTitle,
    braveSystemImage: "leo.product.playlist-add",
    defaultRank: 1200,
    defaultVisibility: .hidden
  )
  public static let toggleNightMode: Self = .init(
    id: "ToggleNightMode",
    title: Strings.NightMode.settingsTitle,
    braveSystemImage: "leo.theme.dark",
    defaultRank: 2000,
    defaultVisibility: .hidden
  )
  public static let shredData: Self = .init(
    id: "ShredData",
    title: Strings.Shields.shredSiteData,
    braveSystemImage: "leo.shred.data",
    defaultRank: 1000,
    defaultVisibility: .visible
  )
  public static let print: Self = .init(
    id: "PrintPage",
    title: Strings.ActionTitles.print,
    braveSystemImage: "leo.print",
    defaultRank: 1800,
    defaultVisibility: .hidden
  )

  // MARK: - Page Activities
  // This list is essentially the same as the ones defined in ShareExtensionHelper

  // A set of all of the page activities defined below.
  public static let allPageActivites: Set<Action.Identifier> = [
    .copyCleanLink,
    .sendURL,
    .toggleReaderMode,
    .findInPage,
    .pageZoom,
    .addFavourites,
    .requestDesktopSite,
    .addSourceNews,
    .createPDF,
    .addSearchEngine,
    .displaySecurityCertificate,
    .reportBrokenSite,
    // Ensure any additional Page activity ID's added below are added to this list since they are
    // not created during menu presentation manually and instead and explicit UIActivity types
    // passed into the share sheet so they will not all exist at all times when creating the menu
  ]

  public static let copyCleanLink: Self = .init(
    id: "CopyCleanLink",
    title: Strings.copyCleanLink,
    braveSystemImage: "leo.copy.clean",
    defaultRank: 1300,
    defaultVisibility: .hidden
  )
  public static let sendURL: Self = .init(
    id: "SendURL",
    title: Strings.OpenTabs.sendWebsiteShareActionTitle,
    braveSystemImage: "leo.smartphone.laptop",
    defaultRank: 2100,
    defaultVisibility: .hidden
  )
  public static let toggleReaderMode: Self = .init(
    id: "ToggleReaderMode",
    title: Strings.toggleReaderMode,
    braveSystemImage: "leo.product.speedreader",
    defaultRank: 1400,
    defaultVisibility: .hidden
  )
  public static let findInPage: Self = .init(
    id: "FindInPage",
    title: Strings.findInPage,
    braveSystemImage: "leo.search",
    defaultRank: 700,
    defaultVisibility: .visible
  )
  public static let pageZoom: Self = .init(
    id: "PageZoom",
    title: Strings.PageZoom.settingsTitle,
    braveSystemImage: "leo.font.size",
    defaultRank: 1500,
    defaultVisibility: .hidden
  )
  public static let addFavourites: Self = .init(
    id: "AddFavourites",
    title: Strings.addToFavorites,
    braveSystemImage: "leo.star.outline",
    defaultRank: 1100,
    defaultVisibility: .hidden
  )
  public static let addBookmark: Self = .init(
    id: "AddBookmark",
    title: Strings.addToMenuItem,
    braveSystemImage: "leo.browser.bookmark-add",
    defaultRank: 200,
    defaultVisibility: .visible
  )
  public static let requestDesktopSite: Self = .init(
    id: "ToggleUserAgent",
    title: Strings.appMenuViewDesktopSiteTitleString,
    braveSystemImage: "leo.monitor",
    defaultRank: 2200,
    defaultVisibility: .hidden
  )
  public static let addSourceNews: Self = .init(
    id: "AddSourceNews",
    title: Strings.BraveNews.addSourceShareTitle,
    braveSystemImage: "leo.rss",
    defaultRank: 2600,
    defaultVisibility: .hidden
  )
  public static let createPDF: Self = .init(
    id: "CreatePDF",
    title: Strings.createPDF,
    braveSystemImage: "leo.file.new",
    defaultRank: 1900,
    defaultVisibility: .hidden
  )
  public static let addSearchEngine: Self = .init(
    id: "AddSearchEngine",
    title: Strings.CustomSearchEngine.customEngineNavigationTitle,
    braveSystemImage: "leo.internet.search",
    defaultRank: 2700,
    defaultVisibility: .hidden
  )
  public static let displaySecurityCertificate: Self = .init(
    id: "DisplaySecurityCertificate",
    title: Strings.displayCertificate,
    braveSystemImage: "leo.certificate.valid",
    defaultRank: 2800,
    defaultVisibility: .hidden
  )
  public static let reportBrokenSite: Self = .init(
    id: "ReportBrokenSite",
    title: Strings.Shields.reportABrokenSite,
    braveSystemImage: "leo.file.warning",
    defaultRank: 2300,
    defaultVisibility: .hidden
  )
}
