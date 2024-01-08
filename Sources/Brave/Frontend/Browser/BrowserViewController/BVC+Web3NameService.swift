// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import BraveWallet
import BraveShared
import BraveCore

extension BrowserViewController: Web3NameServiceScriptHandlerDelegate {
  /// Returns a `DecentralizedDNSHelper` for the given mode if supported and not in private mode.
  func decentralizedDNSHelperFor(url: URL?) -> DecentralizedDNSHelper? {
    let isPrivateMode = privateBrowsingManager.isPrivateBrowsing
    guard !isPrivateMode,
          let url,
          DecentralizedDNSHelper.isSupported(domain: url.domainURL.schemelessAbsoluteDisplayString),
          let rpcService = BraveWallet.JsonRpcServiceFactory.get(privateMode: isPrivateMode) else { return nil }
    return DecentralizedDNSHelper(
      rpcService: rpcService,
      ipfsApi: braveCore.ipfsAPI,
      isPrivateMode: isPrivateMode
    )
  }

  func web3NameServiceDecisionHandler(_ proceed: Bool, web3Service: Web3Service, originalURL: URL) {
    let isPrivateMode = privateBrowsingManager.isPrivateBrowsing
    guard let rpcService = BraveWallet.JsonRpcServiceFactory.get(privateMode: isPrivateMode),
          let decentralizedDNSHelper = self.decentralizedDNSHelperFor(url: originalURL) else {
      finishEditingAndSubmit(originalURL)
      return
    }
    Task { @MainActor in
      switch web3Service {
      case .solana:
        rpcService.setSnsResolveMethod(proceed ? .enabled : .disabled)
      case .ethereum:
        rpcService.setEnsResolveMethod(proceed ? .enabled : .disabled)
      case .ethereumOffchain:
        rpcService.setEnsOffchainLookupResolveMethod(proceed ? .enabled : .disabled)
      case .unstoppable:
        rpcService.setUnstoppableDomainsResolveMethod(proceed ? .enabled : .disabled)
      }
      let result = await decentralizedDNSHelper.lookup(domain: originalURL.host ?? originalURL.absoluteString)
      switch result {
      case let .load(resolvedURL):
        if resolvedURL.isIPFSScheme {
          handleIPFSSchemeURL(resolvedURL)
        } else {
          finishEditingAndSubmit(resolvedURL)
        }
      case let .loadInterstitial(service):
        // ENS interstitial -> ENS Offchain interstitial possible
        showWeb3ServiceInterstitialPage(service: service, originalURL: originalURL)
      case .none:
        // failed to resolve domain or disabled
        finishEditingAndSubmit(originalURL)
      }
    }
  }
}
