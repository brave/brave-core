// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import Web

extension TabDataValues {
  private struct OrderingParentHelperKey: TabDataKey {
    static var defaultValue: (any TabState)?
  }

  /// The tab that a user opened the current tab from. This is used to improve ordering of tabs in
  /// the tab tray & tab bar.
  ///
  /// - note: This is different than `opener` which is used when the tab is opened by the web page
  var orderingParent: (any TabState)? {
    get { self[OrderingParentHelperKey.self] }
    set { self[OrderingParentHelperKey.self] = newValue }
  }
}

extension TabState {
  func isDescendentOf(_ ancestor: any TabState) -> Bool {
    return sequence(first: opener) { $0?.opener }.contains { $0 === ancestor }
      || sequence(first: data.orderingParent) { $0?.data.orderingParent }.contains {
        $0 === ancestor
      }
  }
}
