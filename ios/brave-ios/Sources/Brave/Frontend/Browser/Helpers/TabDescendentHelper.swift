// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import Web

extension TabDataValues {
  private struct ParentHelperKey: TabDataKey {
    static var defaultValue: (any TabState)?
  }

  /// The tab that a user opened the current tab from. Use this to improve ordering of tabs in the
  /// tab tray & tab bar.
  ///
  /// - note: This is different than `opener` which is used when the tab is opened by the web page
  var parent: (any TabState)? {
    get { self[ParentHelperKey.self] }
    set { self[ParentHelperKey.self] = newValue }
  }
}

extension TabState {
  func isDescendentOf(_ ancestor: any TabState) -> Bool {
    return sequence(first: opener) { $0?.opener }.contains { $0 === ancestor }
      || sequence(first: data.parent) { $0?.data.parent }.contains { $0 === ancestor }
  }
}
