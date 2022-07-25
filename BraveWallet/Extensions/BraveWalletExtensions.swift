// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveCore

extension BraveWallet.TransactionInfo {
  var isSwap: Bool {
    ethTxToAddress.caseInsensitiveCompare(NamedAddresses.swapExchangeProxyAddress) == .orderedSame
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
    @unknown default:
      return ""
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
