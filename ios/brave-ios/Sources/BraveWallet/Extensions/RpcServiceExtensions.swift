// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BigNumber
import BraveCore
import Foundation
import Preferences
import os.log

extension BraveWalletJsonRpcService {
  /// Obtain the decimal balance of an `BlockchainToken` for a given account
  ///
  /// If the call fails for some reason or the resulting wei cannot be converted,
  /// `completion` will be called with `nil`
  func balance(
    for token: BraveWallet.BlockchainToken,
    in account: BraveWallet.AccountInfo,
    network: BraveWallet.NetworkInfo,
    decimalFormatStyle: WalletAmountFormatter.DecimalFormatStyle = .balance,
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
          let formatter = WalletAmountFormatter(decimalFormatStyle: decimalFormatStyle)
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
        solanaBalance(pubkey: account.address, chainId: network.chainId) {
          lamports,
          status,
          errorMessage in
          guard status == .success else {
            completion(nil)
            return
          }
          let formatter = WalletAmountFormatter(decimalFormatStyle: decimalFormatStyle)
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
      } else if token.isNft {
        nftBalances(
          walletAddress: account.address,
          nftIdentifiers: [
            .init(
              chainId: token.chainId,
              contractAddress: token.contractAddress,
              tokenId: token.tokenId
            )
          ],
          coin: token.coin
        ) { quantities, errMsg in
          if let quantity = quantities.first {
            completion(quantity.doubleValue)
          } else {
            completion(nil)
          }
        }
      } else {
        splTokenAccountBalance(
          walletAddress: account.address,
          tokenMintAddress: token.contractAddress,
          chainId: network.chainId
        ) { amount, _, _, status, errorMessage in
          guard status == .success else {
            completion(nil)
            return
          }
          let formatter = WalletAmountFormatter(decimalFormatStyle: decimalFormatStyle)
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
      balance(address: account.address, coin: account.coin, chainId: network.chainId) {
        amount,
        status,
        _ in
        guard status == .success && !amount.isEmpty else {
          completion(nil)
          return
        }
        let formatter = WalletAmountFormatter(decimalFormatStyle: decimalFormatStyle)
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
    decimalFormatStyle: WalletAmountFormatter.DecimalFormatStyle,
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
          let formatter = WalletAmountFormatter(decimalFormatStyle: decimalFormatStyle)
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
        solanaBalance(pubkey: accountAddress, chainId: network.chainId) {
          lamports,
          status,
          errorMessage in
          guard status == .success else {
            completion(nil)
            return
          }
          let formatter = WalletAmountFormatter(decimalFormatStyle: decimalFormatStyle)
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
          walletAddress: accountAddress,
          tokenMintAddress: token.contractAddress,
          chainId: network.chainId
        ) { amount, _, _, status, errorMessage in
          guard status == .success else {
            completion(nil)
            return
          }
          let formatter = WalletAmountFormatter(decimalFormatStyle: decimalFormatStyle)
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
      balance(address: accountAddress, coin: token.coin, chainId: network.chainId) {
        amount,
        status,
        _ in
        guard status == .success && !amount.isEmpty else {
          completion(nil)
          return
        }
        let formatter = WalletAmountFormatter(decimalFormatStyle: decimalFormatStyle)
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
      // Bitcoin balance should be fetched using `BraveWallet.BitcoinWalletService`
      completion(nil)
    case .zec:
      completion(nil)
    @unknown default:
      completion(nil)
    }
  }

  @MainActor func balance(
    for token: BraveWallet.BlockchainToken,
    in accountAddress: String,
    network: BraveWallet.NetworkInfo,
    decimalFormatStyle: WalletAmountFormatter.DecimalFormatStyle
  ) async -> BDouble? {
    await withCheckedContinuation { continuation in
      balance(
        for: token,
        in: accountAddress,
        network: network,
        decimalFormatStyle: decimalFormatStyle
      ) { value in
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
        address: accountAddress,
        coin: .eth,
        chainId: network.chainId,
        completion: completion
      )
    } else if token.isErc20 {
      erc20TokenBalance(
        contract: token.contractAddress(in: network),
        address: accountAddress,
        chainId: network.chainId,
        completion: completion
      )
    } else if token.isErc721 || token.isErc1155 || token.isNft {
      nftBalances(
        walletAddress: accountAddress,
        nftIdentifiers: [
          .init(
            chainId: token.chainId,
            contractAddress: token.contractAddress,
            tokenId: token.tokenId
          )
        ],
        coin: token.coin
      ) { quantities, errMsg in
        if let quantity = quantities.first {
          completion(quantity.stringValue, .success, "")
        } else {
          completion("", .internalError, errMsg)
        }
      }
    } else {
      let errorMessage =
        "Unable to fetch ethereum balance for \(token.symbol) token in account address '\(accountAddress)'"
      completion("", .internalError, errorMessage)
    }
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
        return await group.reduce(
          into: [String: Double](),
          { partialResult, new in
            partialResult.merge(with: new)
          }
        )
      }
    )
  }

  /// Returns an array of all networks for the supported coin types. Result will exclude hidden networks if some networks are set to
  /// not shown in Wallet Settings
  @MainActor func allNetworksForSupportedCoins(
    respectHiddenNetworksPreference: Bool = true
  ) async -> [BraveWallet.NetworkInfo] {
    await allNetworks(
      for: WalletConstants.supportedCoinTypes().elements,
      respectHiddenNetworksPreference: respectHiddenNetworksPreference
    )
  }

  /// Returns an array of all networks for givin coins. Result will exclude hidden networks if some networks are set to
  /// not shown in Wallet Settings
  @MainActor func allNetworks(
    for coins: [BraveWallet.CoinType],
    respectHiddenNetworksPreference: Bool = true
  ) async -> [BraveWallet.NetworkInfo] {
    let allNetworks = await self.allNetworks().sorted { lhs, rhs in
      // sort solana chains to the front of the list
      lhs.coin == .sol && rhs.coin != .sol
    }
    var allHiddenChainIds: [String] = []
    for coin in coins {
      let hiddenChainIdsForCoin = await self.hiddenNetworks(coin: coin)
      allHiddenChainIds.append(contentsOf: hiddenChainIdsForCoin)
    }
    let filteredNetworks = allNetworks.filter { network in
      if network.chainId == BraveWallet.LocalhostChainId {
        // localhost not supported on iOS
        return false
      }
      if network.chainId == BraveWallet.BitcoinTestnet {
        if respectHiddenNetworksPreference {
          // check bitcoin testnet is enabled and visibility
          return Preferences.Wallet.isBitcoinTestnetEnabled.value
            && !allHiddenChainIds.contains(network.chainId)
        } else {
          return Preferences.Wallet.isBitcoinTestnetEnabled.value
        }
      }
      if respectHiddenNetworksPreference {
        // filter out hidden networks
        return !allHiddenChainIds.contains(network.chainId)
      }
      return true
    }
    return filteredNetworks
  }

  /// Returns a nullable NFT metadata
  @MainActor func fetchNFTMetadata(
    for token: BraveWallet.BlockchainToken,
    ipfsApi: IpfsAPI
  ) async -> BraveWallet.NftMetadata? {
    let nftIdentifier: BraveWallet.NftIdentifier =
      .init(
        chainId: token.chainId,
        contractAddress: token.contractAddress,
        tokenId: token.tokenId
      )
    let result = await self.nftMetadatas(
      coin: token.coin,
      nftIdentifiers: [nftIdentifier]
    ).0.first
    return result?.httpfyIpfsUrl(ipfsApi: ipfsApi)
  }

  /// Returns a map of Token.id with its `BraveWallet.NftMetadata`
  @MainActor func fetchNFTMetadata(
    tokens: [BraveWallet.BlockchainToken],
    ipfsApi: IpfsAPI
  ) async -> [String: BraveWallet.NftMetadata] {
    var tokenCoinMap = [BraveWallet.CoinType: [BraveWallet.BlockchainToken]]()
    for coin in WalletConstants.supportedCoinTypes() {
      let tokensPerCoin = tokens.filter { $0.coin == coin }
      if !tokensPerCoin.isEmpty {
        tokenCoinMap[coin] = tokensPerCoin
      }
    }
    return await withTaskGroup(
      of: [String: BraveWallet.NftMetadata].self
    ) { @MainActor [weak self] group in
      guard let self = self else { return [:] }
      for (coin, tokensPerCoin) in tokenCoinMap {
        group.addTask {
          var uniqueTokensPerCoin: [BraveWallet.BlockchainToken] = []
          for token in tokensPerCoin {
            if !uniqueTokensPerCoin.contains(token) {
              uniqueTokensPerCoin.append(token)
            }
          }
          let nftIdentifiers = uniqueTokensPerCoin.map {
            BraveWallet.NftIdentifier(
              chainId: $0.chainId,
              contractAddress: $0.contractAddress,
              tokenId: $0.tokenId
            )
          }
          let metadatas = await self.nftMetadatas(coin: coin, nftIdentifiers: nftIdentifiers).0
          var result = [String: BraveWallet.NftMetadata]()
          for (index, token) in tokensPerCoin.enumerated() {
            if let metadata = metadatas[safe: index] {
              result[token.id] = metadata.httpfyIpfsUrl(ipfsApi: ipfsApi)
            }
          }
          return result
        }
      }
      return await group.reduce(
        into: [:]
      ) { partialResult, new in
        partialResult.merge(with: new)
      }
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
            contractAddress: contractAddressesChainIdPair.contractAddress,
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

  /// Returns an array of all hidden network's chainId for givin coins.
  @MainActor func allHiddenNetworks(
    for coins: [BraveWallet.CoinType]
  ) async -> [String] {
    await withTaskGroup(of: [String].self) {
      @MainActor [weak self] group -> [String] in
      guard let self = self else { return [] }
      for coinType in coins {
        group.addTask { @MainActor in
          let chains = await self.hiddenNetworks(coin: coinType)
          return chains.filter {  // localhost not supported
            $0 != BraveWallet.LocalhostChainId
          }
        }
      }
      return await group.reduce([String](), { $0 + $1 })
    }
  }

  /// Remove multiple networks based on its chainId and coin type.
  @MainActor func removeHiddenNetworks(
    for networks: [BraveWallet.CoinType: [String]]
  ) async {
    await withTaskGroup(of: Void.self) {
      @MainActor [weak self] group -> Void in
      guard let self = self else { return }
      for (coin, chainIds) in networks {
        for chainId in chainIds {
          group.addTask { @MainActor in
            await self.removeHiddenNetwork(coin: coin, chainId: chainId)
          }
        }
      }
    }
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
            knownToken.contractAddress.caseInsensitiveCompare(contractAddress) == .orderedSame
              && knownToken.chainId.caseInsensitiveCompare(knownToken.chainId) == .orderedSame
          })
        }
        .map { contractAddress in
          ContractAddressChainIdPair(contractAddress: contractAddress, chainId: transaction.chainId)
        }
    }
  }
}
