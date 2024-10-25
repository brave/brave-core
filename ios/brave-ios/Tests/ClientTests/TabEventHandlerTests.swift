// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Foundation
import WebKit
import XCTest

@testable import Brave

@MainActor class TabEventHandlerTests: XCTestCase {

  func testEventDelivery() {
    let tab = Tab(
      wkConfiguration: nil,
      configuration: nil,
      contentScriptManager: .init(tabForWebView: { _ in nil })
    )
    let handler = DummyHandler()

    XCTAssertNil(handler.isFocused)

    TabEvent.post(.didGainFocus, for: tab)
    XCTAssertTrue(handler.isFocused!)

    TabEvent.post(.didLoseFocus, for: tab)
    XCTAssertFalse(handler.isFocused!)
  }

  func testUnregistration() {
    let tab = Tab(
      wkConfiguration: nil,
      configuration: nil,
      contentScriptManager: .init(tabForWebView: { _ in nil })
    )
    let handler = DummyHandler()

    XCTAssertNil(handler.isFocused)

    TabEvent.post(.didGainFocus, for: tab)
    XCTAssertTrue(handler.isFocused!)

    handler.doUnregister()
    TabEvent.post(.didLoseFocus, for: tab)
    // The event didn't reach us, so we should still be focused.
    XCTAssertTrue(handler.isFocused!)
  }

  func testOnlyRegisteredForEvents() {
    let tab = Tab(
      wkConfiguration: nil,
      configuration: nil,
      contentScriptManager: .init(tabForWebView: { _ in nil })
    )
    let handler = DummyHandler()
    handler.doUnregister()

    let tabObservers = handler.registerFor(.didGainFocus)

    XCTAssertNil(handler.isFocused)

    TabEvent.post(.didGainFocus, for: tab)
    XCTAssertTrue(handler.isFocused!)

    TabEvent.post(.didLoseFocus, for: tab)
    XCTAssertTrue(handler.isFocused!)

    handler.unregister(tabObservers)
  }

  func testOnlyRegisteredForEvents111() {
    let tab = Tab(
      wkConfiguration: nil,
      configuration: nil,
      contentScriptManager: .init(tabForWebView: { _ in nil })
    )

    let urlTest1 = URL(string: "https://www.brave.com")
    let urlTest2 = URL(string: "http://localhost:8080")
    let urlTest3 = URL(string: "https://127.0.0.1:8080")
    let urlTest4 = URL(string: "https://locallhost.com")

    let titleTest1 = "Brave"
    let wrongTestTitle = "Wrong Title"
    let localHostTitle = "localhost"

    let displaytitle1 = tab.fetchDisplayTitle(using: urlTest1, title: titleTest1)
    let displaytitle2 = tab.fetchDisplayTitle(using: urlTest2, title: wrongTestTitle)
    let displaytitle3 = tab.fetchDisplayTitle(using: urlTest3, title: wrongTestTitle)
    let displaytitle4 = tab.fetchDisplayTitle(using: urlTest4, title: localHostTitle)

    XCTAssertEqual(displaytitle1, titleTest1)
    XCTAssertEqual(displaytitle2, "")
    XCTAssertEqual(displaytitle3, "")
    XCTAssertEqual(displaytitle4, localHostTitle)
  }
}

class DummyHandler: TabEventHandler {
  var tabObservers: TabObservers!

  // This is not how this should be written in production — the handler shouldn't be keeping track
  // of individual tab state.
  var isFocused: Bool? = nil

  init() {
    tabObservers = registerFor(.didGainFocus, .didLoseFocus)
  }

  deinit {
    doUnregister()
  }

  fileprivate func doUnregister() {
    unregister(tabObservers)
  }

  func tabDidGainFocus(_ tab: Tab) {
    isFocused = true
  }

  func tabDidLoseFocus(_ tab: Tab) {
    isFocused = false
  }
}
