// Copyright 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Web

extension TabDataValues {
  private struct BlockedDomainTabHelperKey: TabDataKey {
    static var defaultValue: BlockedDomainTabHelper?
  }
  public var blockedDomainHelper: BlockedDomainTabHelper? {
    get { self[BlockedDomainTabHelperKey.self] }
    set { self[BlockedDomainTabHelperKey.self] = newValue }
  }
}

@MainActor
public class BlockedDomainTabHelper {
  private weak var tab: (any TabState)?
  /// A list of domains that we want to proceed to anyways regardless of
  /// any ad-blocking
  private var proceedAnywaysDomainList: Set<String> = []

  public init(
    tab: some TabState
  ) {
    self.tab = tab
  }
}
