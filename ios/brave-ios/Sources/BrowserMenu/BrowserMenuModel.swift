// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import Combine
import Foundation
import GuardianConnect
import NetworkExtension
import Preferences
import SwiftUI

/// Represents an instance of the browser menu
@MainActor class BrowserMenuModel: ObservableObject {
  /// The list of visible actions shown on the menu
  @Published var visibleActions: [Action] = []
  /// The list of hidden actions shown only when the user expands the "More Actions"
  @Published var hiddenActions: [Action] = []
  /// The connected VPN region
  @Published private(set) var vpnStatus: VPNStatus = .disconnected

  private var actions: [Action]
  private var actionVisibility: Preferences.Option<[String: Bool]>
  private var actionRanks: Preferences.Option<[String: Double]>
  private var cancellables: Set<AnyCancellable> = []

  init(
    actions: [Action],
    vpnStatus: VPNStatus = .disconnected,
    vpnStatusPublisher: AnyPublisher<VPNStatus, Never>? = nil,
    actionVisibility: Preferences.Option<[String: Bool]> = Preferences.BrowserMenu
      .actionVisibility,
    actionRanks: Preferences.Option<[String: Double]> = Preferences.BrowserMenu.actionRanks
  ) {
    self.actions = actions
    self.vpnStatus = vpnStatus
    self.actionVisibility = actionVisibility
    self.actionRanks = actionRanks

    #if DEBUG
    let duplicates = Dictionary(grouping: actions.map(\.id), by: \.defaultRank)
      .filter { $1.count > 1 }
    assert(duplicates.isEmpty, "Multiple action IDs have the same default rank: \(duplicates)")
    #endif

    vpnStatusPublisher?
      .receive(on: RunLoop.main)
      .sink(receiveValue: { status in
        MainActor.assumeIsolated {
          withAnimation {
            self.vpnStatus = status
          }
        }
      })
      .store(in: &cancellables)

    reloadActions()
  }

  // MARK: -

  private func reloadActions() {
    let visibleActionOverrides = actionVisibility.value
    let visibleActions = actions.filter {
      if let override = visibleActionOverrides[$0.id.id] {
        return override
      }
      return $0.id.defaultVisibility == .visible
    }
    let sortedActions: [Action] = visibleActions.sorted(by: { first, second in
      let firstRank = rank(for: first)
      let secondRank = rank(for: second)
      return firstRank < secondRank
    })
    let hiddenActionsIDs = Array(Set(actions.map(\.id)).subtracting(sortedActions.map(\.id)))
    let hiddenActions = actions.filter({ hiddenActionsIDs.contains($0.id) }).sorted(
      using: KeyPathComparator(\Action.id.defaultRank)
    )

    self.visibleActions = sortedActions
    self.hiddenActions = hiddenActions
  }

  private func rank(for action: Action) -> Double {
    actionRanks.value[action.id.id] ?? Double(action.id.defaultRank)
  }

  func updateActionVisibility(_ action: Action, visibility: Action.Visibility) {
    actionVisibility.value[action.id.id] = visibility == .visible ? true : false
    if visibility == .visible {
      // If the user is making a hidden item visible and the visible list is empty, we won't
      // give it an overridden rank to ensure it ends at the bottom
      if !visibleActions.isEmpty {
        // Adding a new item from the hidden list, append it to the end if the list is non-empty
        actionRanks.value[action.id.id] = rank(for: visibleActions.last!) + 1
      }
    } else {
      // Hiding it, reset any custom rank
      actionRanks.value[action.id.id] = nil
    }
    reloadActions()
  }

  func reorderVisibleAction(_ action: Action, to index: Int) {
    if visibleActions.isEmpty || !visibleActions.contains(where: { action.id == $0.id }) {
      return
    }
    var newRank: Double = 0
    if index == 0 {
      // First item
      newRank = rank(for: visibleActions.first!) / 2
    } else if index == visibleActions.count - 1 {
      // Last item
      newRank = rank(for: visibleActions.last!) + 1
    } else {
      // Insert between items
      let previousRank = rank(for: visibleActions[index - 1])
      let nextRank = rank(for: visibleActions[index])
      newRank = (previousRank + nextRank) / 2
    }
    actionRanks.value[action.id.id] = newRank
    reloadActions()
  }
}

extension BrowserMenuModel {
  static var mock: BrowserMenuModel {
    let mockStatus: VPNStatus = .connected(
      activeRegion: .init(countryCode: "CA", displayName: "Canada")
    )
    let vpnStatusPublisher = CurrentValueSubject<VPNStatus, Never>(mockStatus)
    let model = BrowserMenuModel(
      actions: [
        .init(
          id: .vpn,
          title: "VPN On",
          traits: .init(badgeColor: UIColor(braveSystemName: .primary50)),
          state: true
        ) { action in
          var actionCopy = action
          actionCopy.state?.toggle()
          actionCopy.title = "VPN \(actionCopy.state! ? "On" : "Off")"
          vpnStatusPublisher.send(actionCopy.state! ? mockStatus : .disconnected)
          return .updateAction(actionCopy)
        },
        .init(id: .addBookmark, attributes: .disabled),
        .init(id: .history),
        .init(id: .braveLeo),
        .init(id: .playlist),
        .init(id: .addFavourites),
        .init(
          id: .toggleNightMode,
          traits: .init(badgeColor: UIColor(braveSystemName: .primary50)),
          state: false
        ) { action in
          var actionCopy = action
          actionCopy.state?.toggle()
          return .updateAction(actionCopy)
        },
        .init(id: .bookmarks, attributes: .disabled),
        .init(id: .braveNews),
        .init(id: .createPDF),
        .init(id: .pageZoom),
      ],
      vpnStatusPublisher: vpnStatusPublisher.eraseToAnyPublisher(),
      actionVisibility: {
        let pref = Preferences.Option<[String: Bool]>(
          key: "browser-menu-mock-action-visibility",
          default: [:]
        )
        pref.reset()
        pref.value[Action.Identifier.createPDF.id] = true
        return pref
      }(),
      actionRanks: {
        let pref = Preferences.Option<[String: Double]>(
          key: "browser-menu-mock-action-ranks",
          default: [:]
        )
        pref.reset()
        return pref
      }()
    )
    return model
  }
}
