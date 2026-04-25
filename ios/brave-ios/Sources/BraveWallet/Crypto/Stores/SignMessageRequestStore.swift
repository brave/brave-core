// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import SwiftUI

class SignMessageRequestStore: ObservableObject {

  @Published var requests: [BraveWallet.SignMessageRequest] {
    didSet {
      guard requests != oldValue else { return }
      update()
    }
  }

  /// The current request on display
  var currentRequest: BraveWallet.SignMessageRequest {
    requests[requestIndex]
  }

  /// Current request index
  @Published var requestIndex: Int = 0
  /// A map between request index and a boolean value indicates this request message needs pilcrow formating/
  /// Key is the request id. This property is assigned by the view, because we need the view height to determine.
  @Published var needPilcrowFormatted: [Int32: Bool] = [:]
  /// A map between request index and a boolean value indicates this request message is displayed as
  /// its original content. Key is the request id.
  @Published var showOrignalMessage: [Int32: Bool] = [:]
  /// EthSwapDetails for CoW swap requests. Key is the request id.
  @Published var ethSwapDetails: [Int32: EthSwapDetails] = [:]

  private let keyringService: BraveWalletKeyringService
  private let rpcService: BraveWalletJsonRpcService
  private let assetRatioService: BraveWalletAssetRatioService
  private let blockchainRegistry: BraveWalletBlockchainRegistry
  private let assetManager: WalletUserAssetManagerType

  /// Cancellable for the last running `update()` Task.
  private var updateTask: Task<(), Never>?
  /// Cache for storing `BlockchainToken`s that are not in user assets or our token registry.
  /// This could occur with a dapp creating a transaction.
  private var tokenInfoCache: [BraveWallet.BlockchainToken] = []

  init(
    requests: [BraveWallet.SignMessageRequest],
    keyringService: BraveWalletKeyringService,
    rpcService: BraveWalletJsonRpcService,
    assetRatioService: BraveWalletAssetRatioService,
    blockchainRegistry: BraveWalletBlockchainRegistry,
    userAssetManager: WalletUserAssetManagerType
  ) {
    self.requests = requests
    self.keyringService = keyringService
    self.rpcService = rpcService
    self.assetRatioService = assetRatioService
    self.blockchainRegistry = blockchainRegistry
    self.assetManager = userAssetManager
  }

  func update() {
    self.updateTask?.cancel()
    self.updateTask = Task { @MainActor in
      // setup default values
      for request in requests {
        if showOrignalMessage[request.id] == nil {
          showOrignalMessage[request.id] = true
        }
        if needPilcrowFormatted[request.id] == nil {
          needPilcrowFormatted[request.id] = false
        }
      }

      let cowSwapRequests: [(id: Int32, cowSwapOrder: BraveWallet.CowSwapOrder, chainId: String)] =
        self.requests
        .compactMap { request in
          guard let cowSwapOrder = request.signData.ethSignTypedData?.meta?.cowSwapOrder else {
            return nil
          }
          return (request.id, cowSwapOrder, request.chainId)
        }
      guard !cowSwapRequests.isEmpty else { return }

      let allNetworks = await rpcService.allNetworksForSupportedCoins(
        respectHiddenNetworksPreference: false
      )
      let userAssets = await assetManager.getAllUserAssetsInNetworkAssets(
        networks: allNetworks,
        includingUserDeleted: true
      ).flatMap(\.tokens)
      let allTokens = await blockchainRegistry.allTokens(in: allNetworks).flatMap(\.tokens)

      let findToken: (String, String) async -> BraveWallet.BlockchainToken? = {
        [tokenInfoCache] contractAddress, chainId in
        userAssets.first(where: {
          $0.contractAddress.caseInsensitiveCompare(contractAddress) == .orderedSame
            && $0.chainId.caseInsensitiveCompare(chainId) == .orderedSame
        }) ?? allTokens.first(where: {
          $0.contractAddress.caseInsensitiveCompare(contractAddress) == .orderedSame
            && $0.chainId.caseInsensitiveCompare(chainId) == .orderedSame
        })
          ?? tokenInfoCache.first(where: {
            $0.contractAddress.caseInsensitiveCompare(contractAddress) == .orderedSame
              && $0.chainId.caseInsensitiveCompare(chainId) == .orderedSame
          })
      }

      // Gather unknown token info to fetch if needed.
      var unknownTokenPairs: Set<ContractAddressChainIdPair> = .init()

      for cowSwapRequest in cowSwapRequests {
        let requestId = cowSwapRequest.id
        let cowSwapOrder = cowSwapRequest.cowSwapOrder
        let chainId = cowSwapRequest.chainId
        guard
          let network = allNetworks.first(where: {
            $0.chainId.caseInsensitiveCompare(chainId) == .orderedSame
          })
        else {
          return
        }

        let formatter = WalletAmountFormatter(
          decimalFormatStyle: .decimals(precision: Int(network.decimals))
        )

        let fromToken: BraveWallet.BlockchainToken? = await findToken(
          cowSwapOrder.sellToken,
          chainId
        )
        let fromTokenDecimals = Int(fromToken?.decimals ?? network.decimals)
        if fromToken == nil {
          unknownTokenPairs.insert(.init(contractAddress: cowSwapOrder.sellToken, chainId: chainId))
        }

        let toToken: BraveWallet.BlockchainToken? = await findToken(cowSwapOrder.buyToken, chainId)
        let toTokenDecimals = Int(toToken?.decimals ?? network.decimals)
        if toToken == nil {
          unknownTokenPairs.insert(.init(contractAddress: cowSwapOrder.buyToken, chainId: chainId))
        }

        let formattedSellAmount =
          formatter.decimalString(
            for: cowSwapOrder.sellAmount,
            radix: .decimal,
            decimals: fromTokenDecimals
          )?.trimmingTrailingZeros ?? ""
        let formattedMinBuyAmount =
          formatter.decimalString(
            for: cowSwapOrder.buyAmount,
            radix: .decimal,
            decimals: toTokenDecimals
          )?.trimmingTrailingZeros ?? ""

        let details = EthSwapDetails(
          fromToken: fromToken,
          fromNetwork: network,
          fromValue: cowSwapOrder.sellAmount,
          fromAmount: formattedSellAmount,
          fromFiat: nil,  // not required for display
          toToken: toToken,
          toNetwork: network,
          minBuyValue: cowSwapOrder.buyToken,
          minBuyAmount: formattedMinBuyAmount,
          minBuyAmountFiat: nil,  // not required for display
          gasFee: nil  // sign request, no gas fee
        )
        self.ethSwapDetails[requestId] = details
      }
      if !unknownTokenPairs.isEmpty {
        fetchUnknownTokens(Array(unknownTokenPairs))
      }
    }
  }

  /// Advance to the next (or first if displaying the last) sign message request.
  func next() {
    if requestIndex + 1 < requests.count {
      if let nextRequestId = requests[safe: requestIndex + 1]?.id,
        showOrignalMessage[nextRequestId] == nil
      {
        // if we have not previously assigned a `showOriginalMessage`
        // value for the next request, assign it the default value now.
        showOrignalMessage[nextRequestId] = true
      }
      requestIndex = requestIndex + 1
    } else {
      requestIndex = 0
    }
  }

  private func fetchUnknownTokens(_ pairs: [ContractAddressChainIdPair]) {
    Task { @MainActor in
      // filter out tokens we have already fetched
      let filteredPairs = pairs.filter { pair in
        !tokenInfoCache.contains(where: {
          $0.contractAddress.caseInsensitiveCompare(pair.contractAddress) != .orderedSame
            && $0.chainId.caseInsensitiveCompare(pair.chainId) != .orderedSame
        })
      }
      guard !filteredPairs.isEmpty else {
        return
      }
      let tokens = await rpcService.fetchEthTokens(for: pairs)
      tokenInfoCache.append(contentsOf: tokens)
      update()
    }
  }
}
