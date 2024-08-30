// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import Combine
import Foundation
import Preferences
import XCTest

@testable import BrowserMenu

extension Action {
  static func test(
    id: String = UUID().uuidString,
    rank: Int,
    visibility: Action.Visibility
  ) -> Action {
    .init(
      id: .init(
        id: id,
        title: id,
        braveSystemImage: "",
        defaultRank: rank,
        defaultVisibility: visibility
      )
    )
  }
}

extension Action: CustomDebugStringConvertible {
  public var debugDescription: String {
    "\(id.defaultRank) \(id.defaultVisibility == .visible ? "visible" : "hidden") (\(id.id))"
  }
}

class BrowserMenuTests: XCTestCase {

  private let actionRanks: Preferences.Option<[String: Double]> = .init(key: "ranks", default: [:])
  private let actionVisibility: Preferences.Option<[String: Bool]> = .init(
    key: "visibilities",
    default: [:]
  )

  override func tearDown() {
    super.tearDown()

    actionRanks.reset()
    actionVisibility.reset()
  }

  /// Tests setting up the model with actions that are all visible and all pre-sorted
  @MainActor func testPresortedAllVisibleActions() {
    let actions: [Action] = [
      .test(rank: 1, visibility: .visible),
      .test(rank: 2, visibility: .visible),
      .test(rank: 3, visibility: .visible),
    ]
    let model = BrowserMenuModel(
      actions: actions,
      actionVisibility: actionVisibility,
      actionRanks: actionRanks
    )

    XCTAssertEqual(model.visibleActions, actions)  // Already in order
    XCTAssertTrue(model.hiddenActions.isEmpty)  // Nothing hidden
  }

  /// Tests adding a hidden item to be visible and vice-versa
  @MainActor func testVisibilityChange() {
    let actions: [Action] = [
      .test(rank: 1, visibility: .visible),
      .test(rank: 2, visibility: .visible),
      .test(rank: 3, visibility: .hidden),
    ]
    let model = BrowserMenuModel(
      actions: actions,
      actionVisibility: actionVisibility,
      actionRanks: actionRanks
    )

    XCTAssertEqual(model.visibleActions, [actions[0], actions[1]])
    XCTAssertEqual(model.hiddenActions, [actions[2]])

    model.updateActionVisibility(actions[0], visibility: .hidden)

    XCTAssertEqual(model.visibleActions, [actions[1]])
    XCTAssertEqual(model.hiddenActions, [actions[0], actions[2]])

    model.updateActionVisibility(actions[2], visibility: .visible)
    XCTAssertEqual(model.visibleActions, [actions[1], actions[2]])
    XCTAssertEqual(model.hiddenActions, [actions[0]])
  }

  /// Tests that altering default visibility of an action will adjust its overriden rank such
  /// that a hidden action made visible will be appended to the end of the list (final rank + 1),
  /// and that a visible action made hidden will fallback to its original default ranking
  @MainActor func testSortOverrideAfterVisbilityChange() {
    let actions: [Action] = [
      .test(rank: 1, visibility: .visible),
      .test(rank: 2, visibility: .hidden),
      .test(rank: 3, visibility: .visible),
      .test(rank: 4, visibility: .hidden),
    ]
    let model = BrowserMenuModel(
      actions: actions,
      actionVisibility: actionVisibility,
      actionRanks: actionRanks
    )
    model.updateActionVisibility(actions[1], visibility: .visible)
    XCTAssertEqual(model.visibleActions, [actions[0], actions[2], actions[1]])
    model.updateActionVisibility(actions[1], visibility: .hidden)
    XCTAssertNil(actionRanks.value[actions[1].id.id])
    XCTAssertEqual(model.hiddenActions, [actions[1], actions[3]])
  }

  /// This tests the edge case where the user has hidden every action, and is now adding one
  /// to the visible list
  @MainActor func testHiddenToVisibleWithNoVisibleActions() {
    let actions: [Action] = [
      .test(rank: 1, visibility: .hidden),
      .test(rank: 2, visibility: .hidden),
      .test(rank: 3, visibility: .hidden),
      .test(rank: 4, visibility: .hidden),
    ]
    let model = BrowserMenuModel(
      actions: actions,
      actionVisibility: actionVisibility,
      actionRanks: actionRanks
    )
    model.updateActionVisibility(actions[0], visibility: .visible)
    XCTAssertEqual(model.visibleActions, [actions[0]])
    // No need to give it a custom rank since there are no other items in the visible list
    XCTAssertNil(actionRanks.value[actions[0].id.id])
  }

  @MainActor func testVisibleToHiddenWithNoHiddenActions() {
    let actions: [Action] = [
      .test(rank: 1, visibility: .visible),
      .test(rank: 2, visibility: .visible),
      .test(rank: 3, visibility: .visible),
      .test(rank: 4, visibility: .visible),
    ]
    let model = BrowserMenuModel(
      actions: actions,
      actionVisibility: actionVisibility,
      actionRanks: actionRanks
    )
    model.updateActionVisibility(actions[0], visibility: .hidden)
    XCTAssertEqual(model.hiddenActions, [actions[0]])
    // No need to give it a custom rank since there are no other items in the visible list
    XCTAssertNil(actionRanks.value[actions[0].id.id])
  }

  @MainActor func testSortByDefaultRankings() {
    let actions: [Action] = [
      .test(rank: 3, visibility: .visible),
      .test(rank: 1, visibility: .visible),
      .test(rank: 2, visibility: .visible),
      .test(rank: 5, visibility: .hidden),
      .test(rank: 9, visibility: .hidden),
      .test(rank: 6, visibility: .hidden),
    ]
    let model = BrowserMenuModel(
      actions: actions,
      actionVisibility: actionVisibility,
      actionRanks: actionRanks
    )

    XCTAssertEqual(model.visibleActions, [actions[1], actions[2], actions[0]])
    XCTAssertEqual(model.hiddenActions, [actions[3], actions[5], actions[4]])
  }

  @MainActor func testReorderLastItemToFirstIndex() {
    let actions: [Action] = [
      .test(rank: 1, visibility: .visible),
      .test(rank: 2, visibility: .visible),
      .test(rank: 3, visibility: .visible),
      .test(rank: 4, visibility: .hidden),
    ]
    let model = BrowserMenuModel(
      actions: actions,
      actionVisibility: actionVisibility,
      actionRanks: actionRanks
    )

    model.reorderVisibleAction(actions[2], to: 0)
    XCTAssertEqual(model.visibleActions, [actions[2], actions[0], actions[1]])
  }

  @MainActor func testReorderFirstItemToLastIndex() {
    let actions: [Action] = [
      .test(rank: 1, visibility: .visible),
      .test(rank: 2, visibility: .visible),
      .test(rank: 3, visibility: .visible),
      .test(rank: 4, visibility: .hidden),
    ]
    let model = BrowserMenuModel(
      actions: actions,
      actionVisibility: actionVisibility,
      actionRanks: actionRanks
    )

    model.reorderVisibleAction(actions[0], to: 2)
    XCTAssertEqual(model.visibleActions, [actions[1], actions[2], actions[0]])
  }

  @MainActor func testReorderLastItemToMiddleIndex() {
    let actions: [Action] = [
      .test(rank: 1, visibility: .visible),
      .test(rank: 2, visibility: .visible),
      .test(rank: 3, visibility: .visible),
      .test(rank: 4, visibility: .hidden),
    ]
    let model = BrowserMenuModel(
      actions: actions,
      actionVisibility: actionVisibility,
      actionRanks: actionRanks
    )

    model.reorderVisibleAction(actions[2], to: 1)
    XCTAssertEqual(model.visibleActions, [actions[0], actions[2], actions[1]])
  }

  @MainActor func testNewActionAddedAfterReorder() {
    var actions: [Action] = [
      .test(rank: 10, visibility: .visible),
      .test(rank: 20, visibility: .visible),
      .test(rank: 30, visibility: .visible),
      .test(rank: 40, visibility: .hidden),
    ]
    var model = BrowserMenuModel(
      actions: actions,
      actionVisibility: actionVisibility,
      actionRanks: actionRanks
    )

    model.reorderVisibleAction(actions[2], to: 1)  // actions[2] rank is now 15

    XCTAssertEqual(model.visibleActions, [actions[0], actions[2], actions[1]])

    // Re-set up model with a new action
    actions.append(.test(rank: 25, visibility: .visible))
    model = BrowserMenuModel(
      actions: actions,
      actionVisibility: actionVisibility,
      actionRanks: actionRanks
    )

    XCTAssertEqual(model.visibleActions, [actions[0], actions[2], actions[1], actions[4]])
  }

  @MainActor func testMultipleReorders() {
    let actions: [Action] = [
      .test(id: "alpha", rank: 1, visibility: .visible),
      .test(id: "beta", rank: 2, visibility: .visible),
      .test(id: "gamma", rank: 3, visibility: .visible),
      .test(id: "delta", rank: 4, visibility: .visible),
    ]
    let model = BrowserMenuModel(
      actions: actions,
      actionVisibility: actionVisibility,
      actionRanks: actionRanks
    )

    // move "gamma" above "beta"
    model.reorderVisibleAction(actions[2], to: 1)
    XCTAssertEqual(model.visibleActions.map(\.id.id), ["alpha", "gamma", "beta", "delta"])

    // move "delta" above "alpha"
    model.reorderVisibleAction(actions[3], to: 0)
    XCTAssertEqual(model.visibleActions.map(\.id.id), ["delta", "alpha", "gamma", "beta"])

    // move "beta" back below "delta" to its original spot
    model.reorderVisibleAction(actions[1], to: 1)
    XCTAssertEqual(model.visibleActions.map(\.id.id), ["delta", "beta", "alpha", "gamma"])

    // move "alpha" below "gamma"
    model.reorderVisibleAction(actions[0], to: 3)
    XCTAssertEqual(model.visibleActions.map(\.id.id), ["delta", "beta", "gamma", "alpha"])
  }

  @MainActor func testReorderingHiddenActionsDoesNothing() {
    let actions: [Action] = [
      .test(rank: 1, visibility: .visible),
      .test(rank: 2, visibility: .visible),
      .test(rank: 3, visibility: .visible),
      .test(rank: 4, visibility: .hidden),
    ]
    let model = BrowserMenuModel(
      actions: actions,
      actionVisibility: actionVisibility,
      actionRanks: actionRanks
    )

    model.reorderVisibleAction(actions[3], to: 1)
    XCTAssertEqual(model.visibleActions, Array(actions[0..<3]))
  }

  @MainActor func testVPNRegionPublishing() async throws {
    let vpnStatusPublisher = CurrentValueSubject<VPNStatus, Never>(.disconnected)
    let model = BrowserMenuModel(
      actions: [],
      vpnStatusPublisher: vpnStatusPublisher.eraseToAnyPublisher(),
      actionVisibility: actionVisibility,
      actionRanks: actionRanks
    )
    XCTAssertEqual(model.vpnStatus, .disconnected)
    vpnStatusPublisher.send(
      .connected(activeRegion: .init(countryCode: "CA", displayName: "ca-east"))
    )
    // CurrentValueSubject vends its current value immediately to the stream so we want to ignore
    // it as we already asserted the state of that before
    for await _ in model.$vpnStatus.values.dropFirst() {
      switch model.vpnStatus {
      case .connected(let region):
        XCTAssertEqual(region.flag, "🇨🇦")
        XCTAssertEqual(region.displayName, "ca-east")
      case .disconnected:
        XCTFail()
      }
      break
    }
  }
}
