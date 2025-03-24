// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import Web

extension TabDataValues {
  struct SnackBarTabHelperKey: TabDataKey {
    static var defaultValue: SnackBarTabHelper?
  }
  var snackBars: SnackBarTabHelper? {
    self[SnackBarTabHelperKey.self]
  }
  fileprivate var _snackBars: SnackBarTabHelper? {
    get { self[SnackBarTabHelperKey.self] }
    set { self[SnackBarTabHelperKey.self] = newValue }
  }
}

protocol SnackBarTabHelperDelegate: AnyObject {
  func tab(_ tab: Tab, didAddSnackbar bar: SnackBar)
  func tab(_ tab: Tab, didRemoveSnackbar bar: SnackBar)
}

final class SnackBarTabHelper {
  weak var tab: Tab?
  weak var delegate: SnackBarTabHelperDelegate?

  var bars = [SnackBar]()

  init(tab: Tab) {
    self.tab = tab
  }

  func addSnackbar(_ bar: SnackBar) {
    guard let tab else { return }
    bars.append(bar)
    delegate?.tab(tab, didAddSnackbar: bar)
  }

  func removeSnackbar(_ bar: SnackBar) {
    guard let tab else { return }
    if let index = bars.firstIndex(of: bar) {
      bars.remove(at: index)
      delegate?.tab(tab, didRemoveSnackbar: bar)
    }
  }

  func removeAllSnackbars() {
    // Enumerate backwards here because we'll remove items from the list as we go.
    bars.reversed().forEach { removeSnackbar($0) }
  }

  func expireSnackbars() {
    guard let tab else { return }
    // Enumerate backwards here because we may remove items from the list as we go.
    bars.reversed().filter({ !$0.shouldPersist(tab) }).forEach({ removeSnackbar($0) })
  }
}

extension SnackBarTabHelper: TabHelper {
  static var keyPath: WritableKeyPath<TabDataValues, SnackBarTabHelper?> { \._snackBars }
}
