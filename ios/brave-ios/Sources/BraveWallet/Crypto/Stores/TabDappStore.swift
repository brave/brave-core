// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Combine
import Foundation

/// This Store will be created for each Tab
public class TabDappStore: ObservableObject, WalletObserverStore {
  /// A set of solana account addresses that are currently connected to the dapp
  @Published public var solConnectedAddresses: Set<String> = .init()
  /// The latest pending dapp permission request. A permission request will be created right after
  /// the account creation request has been fullfilled. We store the request here for `WalletPanelView` observing
  /// the change of this value.
  @Published public var latestPendingPermissionRequest: WebpagePermissionRequest?

  var isObserving: Bool = false

  public init() {}
}
