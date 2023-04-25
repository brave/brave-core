// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveCore

extension BraveWallet.TransactionInfo {
  var isSwap: Bool {
    switch txType {
    case .ethSwap, .solanaSwap:
      return true
    default:
      return false
    }
  }
  var isEIP1559Transaction: Bool {
    guard let ethTxData1559 = txDataUnion.ethTxData1559 else { return false }
    return !ethTxData1559.maxPriorityFeePerGas.isEmpty && !ethTxData1559.maxFeePerGas.isEmpty
  }
  var ethTxToAddress: String {
    // Eth transaction are all coming as `ethTxData1559`
    // Comment below out for future proper eth transaction separation (EIP1559 and non-EIP1559)
    /*if isEIP1559Transaction {
     return txDataUnion.ethTxData1559?.baseData.to ?? ""
     } else {
     return txDataUnion.ethTxData?.to ?? ""
     }*/
    txDataUnion.ethTxData1559?.baseData.to ?? ""
  }
  var ethTxValue: String {
    // Eth transaction are all coming as `ethTxData1559`
    // Comment below out for future proper eth transaction separation (EIP1559 and non-EIP1559)
    /*if isEIP1559Transaction {
     return txDataUnion.ethTxData1559?.baseData.value ?? ""
     } else {
     return txDataUnion.ethTxData?.value ?? ""
     }*/
    txDataUnion.ethTxData1559?.baseData.value ?? ""
  }
  
  var ethTxGasLimit: String {
    // Eth transaction are all coming as `ethTxData1559`
    // Comment below out for future proper eth transaction separation (EIP1559 and non-EIP1559)
    /*if isEIP1559Transaction {
     return txDataUnion.ethTxData1559?.baseData.gasLimit ?? ""
     } else {
     return txDataUnion.ethTxData?.gasLimit ?? ""
     }*/
    txDataUnion.ethTxData1559?.baseData.gasLimit ?? ""
  }
  
  var ethTxGasPrice: String {
    // Eth transaction are all coming as `ethTxData1559`
    // Comment below out for future proper eth transaction separation (EIP1559 and non-EIP1559)
    /*if isEIP1559Transaction {
     return txDataUnion.ethTxData1559?.baseData.gasPrice ?? ""
     } else {
     return txDataUnion.ethTxData?.gasPrice ?? ""
     }*/
    txDataUnion.ethTxData1559?.baseData.gasPrice ?? ""
  }
  
  var ethTxData: [NSNumber] {
    // Eth transaction are all coming as `ethTxData1559`
    // Comment below out for future proper eth transaction separation (EIP1559 and non-EIP1559)
    /*if isEIP1559Transaction {
     return txDataUnion.ethTxData1559?.baseData.data ?? .init()
     } else {
     return txDataUnion.ethTxData?.data ?? .init()
     }*/
    txDataUnion.ethTxData1559?.baseData.data ?? .init()
  }
  
  var ethTxNonce: String {
    // Eth transaction are all coming as `ethTxData1559`
    // Comment below out for future proper eth transaction separation (EIP1559 and non-EIP1559)
    /*if isEIP1559Transaction {
     return txDataUnion.ethTxData1559?.baseData.nonce ?? .init()
     } else {
     return txDataUnion.ethTxData?.nonce ?? .init()
     }*/
    txDataUnion.ethTxData1559?.baseData.nonce ?? ""
  }
}

extension BraveWallet.OriginInfo {
  /// If the current OriginInfo matches the Brave Wallet origin
  var isBraveWalletOrigin: Bool {
    origin == WalletConstants.braveWalletOrigin
  }
}

extension BraveWallet.CoinType {
  var keyringId: String {
    switch self {
    case .eth:
      return BraveWallet.DefaultKeyringId
    case .sol:
      return BraveWallet.SolanaKeyringId
    case .fil:
      return BraveWallet.FilecoinKeyringId
    case .btc:
      return BraveWallet.BitcoinKeyringId
    @unknown default:
      return BraveWallet.DefaultKeyringId
    }
  }
  
  var localizedTitle: String {
    switch self {
    case .eth:
      return Strings.Wallet.coinTypeEthereum
    case .sol:
      return Strings.Wallet.coinTypeSolana
    case .fil:
      return Strings.Wallet.coinTypeFilecoin
    case .btc:
      fallthrough
    @unknown default:
      return Strings.Wallet.coinTypeUnknown
    }
  }
  
  var localizedDescription: String {
    switch self {
    case .eth:
      return Strings.Wallet.coinTypeEthereumDescription
    case .sol:
      return Strings.Wallet.coinTypeSolanaDescription
    case .fil:
      return Strings.Wallet.coinTypeFilecoinDescription
    case .btc:
      fallthrough
    @unknown default:
      return Strings.Wallet.coinTypeUnknown
    }
  }
  
  var iconName: String {
    switch self {
    case .eth:
      return "eth-asset-icon"
    case .sol:
      return "sol-asset-icon"
    case .fil:
      return "filecoin-asset-icon"
    case .btc:
      fallthrough
    @unknown default:
      return ""
    }
  }
  
  var defaultAccountName: String {
    switch self {
    case .eth:
      return Strings.Wallet.defaultEthAccountName
    case .sol:
      return Strings.Wallet.defaultSolAccountName
    case .fil:
      return Strings.Wallet.defaultFilAccountName
    case .btc:
      fallthrough
    @unknown default:
      return ""
    }
  }
  
  var defaultSecondaryAccountName: String {
    switch self {
    case .eth:
      return Strings.Wallet.defaultSecondaryEthAccountName
    case .sol:
      return Strings.Wallet.defaultSecondarySolAccountName
    case .fil:
      return Strings.Wallet.defaultSecondaryFilAccountName
    case .btc:
      fallthrough
    @unknown default:
      return ""
    }
  }
  
  /// Sort order used when sorting by coin types
  var sortOrder: Int {
    switch self {
    case .eth:
      return 1
    case .sol:
      return 2
    case .fil:
      return 3
    case .btc:
      fallthrough
    @unknown default:
      return 10
    }
  }
}

extension BraveWallet.KeyringInfo {
  var coin: BraveWallet.CoinType? {
    accountInfos.first?.coin
  }
}

extension BraveWallet.TransactionInfo {
  var coin: BraveWallet.CoinType {
    if txDataUnion.solanaTxData != nil {
      return .sol
    } else if txDataUnion.filTxData != nil {
      return .fil
    } else {
      return .eth
    }
  }
}

extension BraveWallet.NetworkInfo {
  func isNativeAsset(_ token: BraveWallet.BlockchainToken) -> Bool {
    return nativeToken.contractAddress.caseInsensitiveCompare(token.contractAddress) == .orderedSame
    && nativeToken.symbol.caseInsensitiveCompare(token.symbol) == .orderedSame
    && symbol.caseInsensitiveCompare(token.symbol) == .orderedSame
    && nativeToken.decimals == token.decimals
    && coin == token.coin
  }
}

extension BraveWallet.BlockchainToken {
  /// The id to fetch price and price history.
  var assetRatioId: String {
    if !coingeckoId.isEmpty {
      return coingeckoId
    }
    
    if chainId != BraveWallet.MainnetChainId || contractAddress.isEmpty {
      return symbol
    }
    
    return contractAddress
  }
  
  /// The id to map with the return balance from RPCService
  var assetBalanceId: String {
    contractAddress + symbol + chainId + tokenId
  }
  
  var isAuroraSupportedToken: Bool {
    let isSupportedContractAddress = WalletConstants.supportedAuroraBridgeTokensContractAddresses
      .contains(where: { $0.caseInsensitiveCompare(contractAddress) == .orderedSame })
    return (contractAddress.isEmpty || isSupportedContractAddress) && chainId == BraveWallet.MainnetChainId
  }
  
  var nftTokenTitle: String {
    if isErc721, let tokenId = Int(tokenId.removingHexPrefix, radix: 16) {
      return "\(name) #\(tokenId)"
    } else {
      return name
    }
  }
}

extension BraveWallet.OnRampProvider {
  var name: String {
    switch self {
    case .ramp:
      return Strings.Wallet.rampNetworkProviderName
    case .sardine:
      return Strings.Wallet.sardineProviderName
    case .transak:
      return Strings.Wallet.transakProviderName
    default:
      return ""
    }
  }
  
  var shortName: String {
    switch self {
    case .ramp:
      return Strings.Wallet.rampNetworkProviderShortName
    case .sardine:
      return Strings.Wallet.sardineProviderShortName
    case .transak:
      return Strings.Wallet.transakProviderShortName
    default:
      return ""
    }
  }
  
  var localizedDescription: String {
    switch self {
    case .ramp:
      return Strings.Wallet.rampNetworkProviderDescription
    case .sardine:
      return Strings.Wallet.sardineProviderDescription
    case .transak:
      return Strings.Wallet.transakProviderDescription
    default:
      return ""
    }
  }
  
  var iconName: String {
    switch self {
    case .ramp:
      return "ramp-network-icon"
    case .sardine:
      return "sardine-icon"
    case .transak:
      return "transak-icon"
    default:
      return ""
    }
  }
}

extension BraveWallet.CoinMarket {
  static func abbreviateToBillion(input: Double) -> Double {
    input / 1000000000
  }
}

public extension String {
  /// Returns true if the string ends with a supported ENS extension.
  var endsWithSupportedENSExtension: Bool {
    WalletConstants.supportedENSExtensions.contains(where: hasSuffix)
  }
  
  /// Returns true if the string ends with a supported SNS extension.
  var endsWithSupportedSNSExtension: Bool {
    WalletConstants.supportedSNSExtensions.contains(where: hasSuffix)
  }
  
  /// Returns true if the string ends with a supported UD extension.
  var endsWithSupportedUDExtension: Bool {
    WalletConstants.supportedUDExtensions.contains(where: hasSuffix)
  }
  
  /// Returns true if `Self` is a valid account name
  var isValidAccountName: Bool {
    self.count <= 30
  }
}

public extension URL {
  /// Returns true if url's scheme is supported to be resolved using IPFS public gateway
  var isIPFSScheme: Bool {
    guard let scheme = self.scheme?.lowercased() else { return false }
    return WalletConstants.supportedIPFSSchemes.contains(scheme)
  }
}
