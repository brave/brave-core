// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Shared

public enum Web3Service: String, CaseIterable {
  case solana
  case ethereum
  case ethereumOffchain
  case unstoppable

  public var id: String { rawValue }
}

@MainActor public class DecentralizedDNSHelper {

  private let rpcService: BraveWalletJsonRpcService
  private let ipfsApi: IpfsAPI?

  public init?(
    rpcService: BraveWalletJsonRpcService,
    ipfsApi: IpfsAPI?,
    isPrivateMode: Bool
  ) {
    guard !isPrivateMode else { return nil }
    self.rpcService = rpcService
    self.ipfsApi = ipfsApi
  }

  public enum DNSLookupResult: Equatable {
    case none
    case loadInterstitial(Web3Service)
    case load(URL)
  }

  public func lookup(domain: String) async -> DNSLookupResult {
    guard let fixupURL = URIFixup.getURL(domain) else {
      return .none
    }
    if fixupURL.domainURL.schemelessAbsoluteDisplayString.endsWithSupportedUDExtension {
      return await lookupUD(domain: domain)
    } else if fixupURL.domainURL.schemelessAbsoluteDisplayString.endsWithSupportedENSExtension {
      return await lookupENS(domain: domain)
    } else if fixupURL.domainURL.schemelessAbsoluteDisplayString.endsWithSupportedSNSExtension {
      return await lookupSNS(domain: domain)
    }
    return .none
  }

  /// Decentralized DNS lookup for an Unstoppable Domains domain
  private func lookupUD(domain: String) async -> DNSLookupResult {
    let udResolveMethod = await rpcService.unstoppableDomainsResolveMethod()
    switch udResolveMethod {
    case .ask:
      return .loadInterstitial(.unstoppable)
    case .enabled:
      let (url, status, _) = await rpcService.unstoppableDomainsResolveDns(domain: domain)
      guard status == .success, let url else {
        return .none
      }
      return .load(url)
    case .disabled:
      return .none
    @unknown default:
      return .none
    }
  }

  /// Decentralized DNS lookup for an ENS domain
  private func lookupENS(domain: String) async -> DNSLookupResult {
    let ensResolveMethod = await rpcService.ensResolveMethod()
    switch ensResolveMethod {
    case .ask:
      return .loadInterstitial(.ethereum)
    case .enabled:
      let (contentHash, isOffchainConsentRequired, status, _) = await rpcService.ensGetContentHash(
        domain: domain
      )
      if isOffchainConsentRequired {
        return .loadInterstitial(.ethereumOffchain)
      }
      guard status == .success,
        !contentHash.isEmpty,
        let ipfsUrl = ipfsApi?.contentHashToCIDv1URL(for: contentHash),
        !ipfsUrl.isBookmarklet
      else {
        return .none
      }
      return .load(ipfsUrl)
    case .disabled:
      return .none
    @unknown default:
      return .none
    }
  }

  /// Decentralized DNS lookup for an SNS domain
  private func lookupSNS(domain: String) async -> DNSLookupResult {
    let snsResolveMethod = await rpcService.snsResolveMethod()
    switch snsResolveMethod {
    case .ask:
      return .loadInterstitial(.solana)
    case .enabled:
      let (url, status, _) = await rpcService.snsResolveHost(domain: domain)
      guard status == .success,
        let url,
        !url.isBookmarklet
      else {
        return .none
      }
      return .load(url)
    case .disabled:
      return .none
    @unknown default:
      return .none
    }
  }

  public static func isSupported(domain: String) -> Bool {
    domain.endsWithSupportedUDExtension || domain.endsWithSupportedENSExtension
      || domain.endsWithSupportedSNSExtension
  }
}
