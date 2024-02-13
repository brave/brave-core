// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import XCTest
import BraveStrings

@testable import Brave

class MenuItemFactoryTests: XCTestCase {
  func test_buttonType_deliversCorrectTitleAndSubtitle() {
    let bookmarksButton = MenuItemFactory.button(for: .bookmarks, completion: {})
    let downloadsButton = MenuItemFactory.button(for: .downloads, completion: {})
    let historyButton = MenuItemFactory.button(for: .history, completion: {})
    let newsButton = MenuItemFactory.button(for: .news, completion: {})
    let playlistButtonWithoutSubtitle = MenuItemFactory.button(for: .playlist(), completion: {})
    let playlistButtonWithSubtitle = MenuItemFactory.button(for: .playlist(subtitle: "some subtitle"), completion: {})
    let settingsButton = MenuItemFactory.button(for: .settings, completion: {})
    let talkButton = MenuItemFactory.button(for: .talk, completion: {})
    let walletButtonWithoutSubtitle = MenuItemFactory.button(for: .wallet(), completion: {})
    let walletButtonWithSubtitle = MenuItemFactory.button(for: .wallet(subtitle: "some subtitle"), completion: {})
    
    XCTAssertEqual(bookmarksButton.title, Strings.bookmarksMenuItem)
    XCTAssertNil(bookmarksButton.subtitle)
    
    XCTAssertEqual(downloadsButton.title, Strings.downloadsMenuItem)
    XCTAssertNil(downloadsButton.subtitle)
    
    XCTAssertEqual(historyButton.title, Strings.historyMenuItem)
    XCTAssertNil(historyButton.subtitle)
    
    XCTAssertEqual(newsButton.title, Strings.OptionsMenu.braveNewsItemTitle)
    XCTAssertEqual(newsButton.subtitle, Strings.OptionsMenu.braveNewsItemDescription)
    
    XCTAssertEqual(playlistButtonWithoutSubtitle.title, Strings.OptionsMenu.bravePlaylistItemTitle)
    XCTAssertNil(playlistButtonWithoutSubtitle.subtitle)
    
    XCTAssertEqual(playlistButtonWithSubtitle.title, Strings.OptionsMenu.bravePlaylistItemTitle)
    XCTAssertEqual(playlistButtonWithSubtitle.subtitle, "some subtitle")
    
    XCTAssertEqual(settingsButton.title, Strings.settingsMenuItem)
    XCTAssertNil(settingsButton.subtitle)
    
    XCTAssertEqual(talkButton.title, Strings.OptionsMenu.braveTalkItemTitle)
    XCTAssertEqual(talkButton.subtitle, Strings.OptionsMenu.braveTalkItemDescription)
    
    XCTAssertEqual(walletButtonWithoutSubtitle.title, Strings.Wallet.wallet)
    XCTAssertNil(walletButtonWithoutSubtitle.subtitle)
    
    XCTAssertEqual(walletButtonWithSubtitle.title, Strings.Wallet.wallet)
    XCTAssertEqual(walletButtonWithSubtitle.subtitle, "some subtitle")
  }
}
