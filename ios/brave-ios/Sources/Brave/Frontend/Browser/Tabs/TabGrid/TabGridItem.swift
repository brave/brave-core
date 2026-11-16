// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import Web

/// An entity shown in the tab grid: a tab, or a group standing in for the tabs it contains
enum TabGridItem {
  case tab(any TabState)
  case group(TabGroup, tabs: [any TabState])

  enum ID: Hashable {
    case tab(TabState.ID)
    case group(TabGroup.ID)
  }

  var id: ID {
    switch self {
    case .tab(let tab): .tab(tab.id)
    case .group(let group, _): .group(group.id)
    }
  }

  /// The tabs this item stands for
  var tabs: [any TabState] {
    switch self {
    case .tab(let tab): [tab]
    case .group(_, let tabs): tabs
    }
  }

  var isGroup: Bool {
    if case .group = self { true } else { false }
  }

  var containsVisibleTab: Bool {
    tabs.contains(where: \.isVisible)
  }

  /// The tab to open when the item is tapped
  var representativeTab: (any TabState)? {
    tabs.first(where: \.isVisible) ?? tabs.first
  }

  /// Arranges `tabs` for display. Each group is shown once, where its first tab is.
  static func items(for tabs: [any TabState], using tabManager: TabManager) -> [TabGridItem] {
    var items: [TabGridItem] = []
    var emittedGroupIDs: Set<TabGroup.ID> = []
    for tab in tabs {
      guard let group = tabManager.tabGroup(for: tab) else {
        items.append(.tab(tab))
        continue
      }
      if emittedGroupIDs.insert(group.id).inserted {
        let groupTabs = tabs.filter { tabManager.tabGroup(for: $0)?.id == group.id }
        items.append(.group(group, tabs: groupTabs))
      }
    }
    return items
  }
}
