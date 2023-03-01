// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import BraveWallet
import BraveShared
import BraveCore

extension BrowserViewController: Web3NameServiceScriptHandlerDelegate {
  func web3NameServiceDecisionHandler(_ proceed: Bool, originalURL: URL, visitType: VisitType) {
    let isPrivateMode = PrivateBrowsingManager.shared.isPrivateBrowsing
    guard let rpcService = BraveWallet.JsonRpcServiceFactory.get(privateMode: isPrivateMode) else {
      finishEditingAndSubmit(originalURL, visitType: visitType)
      return
    }
    if proceed {
      Task { @MainActor in
        rpcService.setSnsResolveMethod(.enabled)
        if let host = originalURL.host, let resolvedUrl = await resolveSNSHost(host, rpcService: rpcService) {
          // resolved url
          finishEditingAndSubmit(resolvedUrl, visitType: visitType)
        }
      }
    } else {
      rpcService.setSnsResolveMethod(.disabled)
      finishEditingAndSubmit(originalURL, visitType: visitType)
    }
  }
}
