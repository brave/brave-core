// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveCore
import BigNumber
import os.log
import Preferences

extension BraveWalletJsonRpcService {
  /// Obtain the decimal balance of an `BlockchainToken` for a given account
  ///
  /// If the call fails for some reason or the resulting wei cannot be converted,
  /// `completion` will be called with `nil`
  func balance(
    for token: BraveWallet.BlockchainToken,
    in account: BraveWallet.AccountInfo,
    network: BraveWallet.NetworkInfo,
    decimalFormatStyle: WeiFormatter.DecimalFormatStyle = .balance,
    completion: @escaping (Double?) -> Void
  ) {
    switch network.coin {
    case .eth:
      ethBalance(
        for: token,
        in: account.address,
        network: network,
        completion: { wei, status, _ in
          guard status == .success && !wei.isEmpty else {
            completion(nil)
            return
          }
          let formatter = WeiFormatter(decimalFormatStyle: decimalFormatStyle)
          if let valueString = formatter.decimalString(
            for: wei.removingHexPrefix,
            radix: .hex,
            decimals: Int(token.decimals)
          ) {
            completion(Double(valueString))
          } else {
            completion(nil)
          }
        }
      )
    case .sol:
      if network.isNativeAsset(token) {
        solanaBalance(account.address, chainId: network.chainId) { lamports, status, errorMessage in
          guard status == .success else {
            completion(nil)
            return
          }
          let formatter = WeiFormatter(decimalFormatStyle: decimalFormatStyle)
          if let valueString = formatter.decimalString(
            for: "\(lamports)",
            radix: .decimal,
            decimals: Int(token.decimals)
          ) {
            completion(Double(valueString))
          } else {
            completion(nil)
          }
        }
      } else {
        splTokenAccountBalance(
          account.address,
          tokenMintAddress: token.contractAddress,
          chainId: network.chainId
        ) { amount, _, _, status, errorMessage in
          guard status == .success else {
            completion(nil)
            return
          }
          let formatter = WeiFormatter(decimalFormatStyle: decimalFormatStyle)
          if let valueString = formatter.decimalString(
            for: "\(amount)",
            radix: .decimal,
            decimals: Int(token.decimals)
          ) {
            completion(Double(valueString))
          } else {
            completion(nil)
          }
        }
      }
    case .fil:
      balance(account.address, coin: account.coin, chainId: network.chainId) { amount, status, _ in
        guard status == .success && !amount.isEmpty else {
          completion(nil)
          return
        }
        let formatter = WeiFormatter(decimalFormatStyle: decimalFormatStyle)
        if let valueString = formatter.decimalString(
          for: "\(amount)",
          radix: .decimal,
          decimals: Int(token.decimals)
        ) {
          completion(Double(valueString))
        } else {
          completion(nil)
        }
      }
    case .btc:
      completion(nil)
    case .zec:
      completion(nil)
    @unknown default:
      completion(nil)
    }
  }
  
  /// Obtain the decimal balance of an `BlockchainToken` for a given account
  ///
  /// If the call fails for some reason or the resulting wei cannot be converted,
  /// `completion` will be called with `nil`
  @MainActor func balance(
    for token: BraveWallet.BlockchainToken,
    in account: BraveWallet.AccountInfo,
    network: BraveWallet.NetworkInfo
  ) async -> Double? {
    await withCheckedContinuation { continuation in
      balance(for: token, in: account, network: network) { value in
        continuation.resume(returning: value)
      }
    }
  }
  
  /// Obtain the decimal balance in `BDouble` of an `BlockchainToken` for a given account
  /// with certain decimal format style
  ///
  /// If the call fails for some reason or the resulting wei cannot be converted,
  /// `completion` will be called with `nil`
  func balance(
    for token: BraveWallet.BlockchainToken,
    in accountAddress: String,
    network: BraveWallet.NetworkInfo,
    decimalFormatStyle: WeiFormatter.DecimalFormatStyle,
    completion: @escaping (BDouble?) -> Void
  ) {
    switch network.coin {
    case .eth:
      ethBalance(
        for: token,
        in: accountAddress,
        network: network,
        completion: { wei, status, _ in
          guard status == .success && !wei.isEmpty else {
            completion(nil)
            return
          }
          let formatter = WeiFormatter(decimalFormatStyle: decimalFormatStyle)
          if let valueString = formatter.decimalString(
            for: wei.removingHexPrefix,
            radix: .hex,
            decimals: Int(token.decimals)
          ) {
            completion(BDouble(valueString))
          } else {
            completion(nil)
          }
        }
      )
    case .sol:
      if network.isNativeAsset(token) {
        solanaBalance(accountAddress, chainId: network.chainId) { lamports, status, errorMessage in
          guard status == .success else {
            completion(nil)
            return
          }
          let formatter = WeiFormatter(decimalFormatStyle: decimalFormatStyle)
          if let valueString = formatter.decimalString(
            for: "\(lamports)",
            radix: .decimal,
            decimals: Int(token.decimals)
          ) {
            completion(BDouble(valueString))
          } else {
            completion(nil)
          }
        }
      } else {
        splTokenAccountBalance(
          accountAddress,
          tokenMintAddress: token.contractAddress,
          chainId: network.chainId
        ) { amount, _, _, status, errorMessage in
          guard status == .success else {
            completion(nil)
            return
          }
          let formatter = WeiFormatter(decimalFormatStyle: decimalFormatStyle)
          if let valueString = formatter.decimalString(
            for: "\(amount)",
            radix: .decimal,
            decimals: Int(token.decimals)
          ) {
            completion(BDouble(valueString))
          } else {
            completion(nil)
          }
        }
      }
    case .fil:
      balance(accountAddress, coin: token.coin, chainId: network.chainId) { amount, status, _ in
        guard status == .success && !amount.isEmpty else {
          completion(nil)
          return
        }
        let formatter = WeiFormatter(decimalFormatStyle: decimalFormatStyle)
        if let valueString = formatter.decimalString(
          for: "\(amount)",
          radix: .decimal,
          decimals: Int(token.decimals)
        ) {
          completion(BDouble(valueString))
        } else {
          completion(nil)
        }
      }
    case .btc:
      completion(nil)
    @unknown default:
      completion(nil)
    }
  }
  
  @MainActor func balance(
    for token: BraveWallet.BlockchainToken,
    in accountAddress: String,
    network: BraveWallet.NetworkInfo,
    decimalFormatStyle: WeiFormatter.DecimalFormatStyle
  ) async -> BDouble? {
    await withCheckedContinuation { continuation in
      balance(for: token, in: accountAddress, network: network, decimalFormatStyle: decimalFormatStyle) { value in
        continuation.resume(returning: value)
      }
    }
  }
  
  private func ethBalance(
    for token: BraveWallet.BlockchainToken,
    in accountAddress: String,
    network: BraveWallet.NetworkInfo,
    completion: @escaping (String, BraveWallet.ProviderError, String) -> Void
  ) {
    if network.isNativeAsset(token) {
      balance(
        accountAddress,
        coin: .eth,
        chainId: network.chainId,
        completion: completion
      )
    } else if token.isErc20 {
      erc20TokenBalance(
        token.contractAddress(in: network),
        address: accountAddress,
        chainId: network.chainId,
        completion: completion
      )
    } else if token.isErc721 {
      erc721TokenBalance(
        token.contractAddress,
        tokenId: token.tokenId,
        accountAddress: accountAddress,
        chainId: network.chainId,
        completion: completion
      )
    } else {
      let errorMessage = "Unable to fetch ethereum balance for \(token.symbol) token in account address '\(accountAddress)'"
      completion("", .internalError, errorMessage)
    }
  }
  
  /// Returns the total balance for a given token for all of the given accounts
  @MainActor func fetchTotalBalance(
    token: BraveWallet.BlockchainToken,
    network: BraveWallet.NetworkInfo,
    accounts: [BraveWallet.AccountInfo]
  ) async -> Double {
    let balancesForAsset = await withTaskGroup(of: [Double].self, body: { @MainActor group in
      for account in accounts {
        group.addTask { @MainActor in
          let balance = await self.balance(
            for: token,
            in: account,
            network: network
          )
          return [balance ?? 0]
        }
      }
      return await group.reduce([Double](), { $0 + $1 })
    })
    return balancesForAsset.reduce(0, +)
  }
  
  /// Returns the total balance for a given account for all of the given network assets
  func fetchBalancesForTokens(
    account: BraveWallet.AccountInfo,
    networkAssets: [NetworkAssets]
  ) async -> [String: Double] {
    await withTaskGroup(
      of: [String: Double].self,
      body: { group in
        for networkAsset in networkAssets where networkAsset.network.coin == account.coin {
          for token in networkAsset.tokens {
            group.addTask {
              let balance = await self.balance(
                for: token,
                in: account,
                network: networkAsset.network
              )
              if let balance {
                return [token.id: balance]
              } else {
                return [:]
              }
            }
          }
        }
        return await group.reduce(into: [String: Double](), { partialResult, new in
          partialResult.merge(with: new)
        })
      }
    )
  }
  
  /// Returns an array of all networks for the supported coin types. Result will exclude test networks if test networks is set to
  /// not shown in Wallet Settings
  @MainActor func allNetworksForSupportedCoins(respectTestnetPreference: Bool = true) async -> [BraveWallet.NetworkInfo] {
    await allNetworks(for: WalletConstants.supportedCoinTypes().elements, respectTestnetPreference: respectTestnetPreference)
  }

  /// Returns an array of all networks for givin coins. Result will exclude test networks if test networks is set to
  /// not shown in Wallet Settings
  @MainActor func allNetworks(for coins: [BraveWallet.CoinType], respectTestnetPreference: Bool = true) async -> [BraveWallet.NetworkInfo] {
    await withTaskGroup(of: [BraveWallet.NetworkInfo].self) { @MainActor [weak self] group -> [BraveWallet.NetworkInfo] in
      guard let self = self else { return [] }
      for coinType in coins {
        group.addTask { @MainActor in
          let chains = await self.allNetworks(coinType)
          return chains.filter { // localhost not supported
            $0.chainId != BraveWallet.LocalhostChainId
          }
        }
      }
      let allChains = await group.reduce([BraveWallet.NetworkInfo](), { $0 + $1 }).filter { network in
        if !Preferences.Wallet.showTestNetworks.value && respectTestnetPreference { // filter out test networks
          return !WalletConstants.supportedTestNetworkChainIds.contains(where: { $0 == network.chainId })
        }
        return true
      }
      return allChains.sorted { lhs, rhs in
        // sort solana chains to the front of the list
        lhs.coin == .sol && rhs.coin != .sol
      }
    }
  }
  
  /// Returns a nullable NFT metadata
  @MainActor func fetchNFTMetadata(for token: BraveWallet.BlockchainToken, ipfsApi: IpfsAPI) async -> NFTMetadata? {
    var metaDataString = ""
    if token.isErc721 {
      let (_, metaData, result, errMsg) = await self.erc721Metadata(token.contractAddress, tokenId: token.tokenId, chainId: token.chainId)
      
      if result != .success {
        Logger.module.debug("Failed to load ERC721 metadata: \(errMsg)")
      }
      metaDataString = metaData
    } else {
      let (_, metaData, result, errMsg) = await self.solTokenMetadata(token.chainId, tokenMintAddress: token.contractAddress)
      if result != .success {
        Logger.module.debug("Failed to load Solana NFT metadata: \(errMsg)")
      }
      metaDataString = metaData
    }
    if let data = metaDataString.data(using: .utf8),
       let result = try? JSONDecoder().decode(NFTMetadata.self, from: data) {
      return result.httpfyIpfsUrl(ipfsApi: ipfsApi)
    }
    return nil
  }
  
  /// Returns a map of Token.id with its NFT metadata
  @MainActor func fetchNFTMetadata(tokens: [BraveWallet.BlockchainToken], ipfsApi: IpfsAPI) async -> [String: NFTMetadata] {
    await withTaskGroup(of: [String: NFTMetadata].self) {  @MainActor [weak self] group -> [String: NFTMetadata] in
      guard let self = self else { return [:] }
      for token in tokens {
        group.addTask { @MainActor in
          var metaDataString = ""
          if token.isErc721 {
            let (_, metaData, result, errMsg) = await self.erc721Metadata(token.contractAddress, tokenId: token.tokenId, chainId: token.chainId)

            if result != .success {
              Logger.module.debug("Failed to load ERC721 metadata: \(errMsg)")
            }
            metaDataString = metaData
          } else {
            let (_, metaData, result, errMsg) = await self.solTokenMetadata(token.chainId, tokenMintAddress: token.contractAddress)
            if result != .success {
              Logger.module.debug("Failed to load Solana NFT metadata: \(errMsg)")
            }
            metaDataString = metaData
          }
          
          if let data = metaDataString.data(using: .utf8),
             let result = try? JSONDecoder().decode(NFTMetadata.self, from: data) {

            return [token.id: result.httpfyIpfsUrl(ipfsApi: ipfsApi)]
          }
          return [:]
        }
      }
      
      return await group.reduce([:], { $0.merging($1, uniquingKeysWith: { key, _ in key })
      })
    }
  }
  
  /// Fetches the BlockchainToken for the given contract addresses. The token for a given contract
  /// address is not guaranteed to be found, and will not be provided in the result if not found.
  @MainActor func fetchEthTokens(
    for contractAddressesChainIdPairs: [ContractAddressChainIdPair]
  ) async -> [BraveWallet.BlockchainToken] {
    await withTaskGroup(of: [BraveWallet.BlockchainToken?].self) { @MainActor group in
      for contractAddressesChainIdPair in contractAddressesChainIdPairs {
        group.addTask {
          let (token, _, _) = await self.ethTokenInfo(
            contractAddressesChainIdPair.contractAddress,
            chainId: contractAddressesChainIdPair.chainId
          )
          if let token {
            return [token]
          }
          return []
        }
      }
      return await group.reduce([BraveWallet.BlockchainToken?](), { $0 + $1 })
    }.compactMap { $0 }
  }
}

struct ContractAddressChainIdPair: Equatable, Hashable {
  let contractAddress: String
  let chainId: String
}

extension Array where Element == BraveWallet.TransactionInfo {
  func unknownTokenContractAddressChainIdPairs(
    knownTokens: [BraveWallet.BlockchainToken]
  ) -> [ContractAddressChainIdPair] {
    flatMap { transaction in
      transaction.tokenContractAddresses
        .filter { contractAddress in
          !knownTokens.contains(where: { knownToken in
            knownToken.contractAddress.caseInsensitiveCompare(contractAddress) == .orderedSame &&
            knownToken.chainId.caseInsensitiveCompare(knownToken.chainId) == .orderedSame
          })
        }
        .map { contractAddress in
        ContractAddressChainIdPair(contractAddress: contractAddress, chainId: transaction.chainId)
      }
    }
  }
}
