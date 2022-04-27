// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import SwiftUI
import BraveCore

extension BraveWallet.AccountInfo: Identifiable {
  public var id: String {
    address
  }
  public var isPrimary: Bool {
    !isImported
  }
}

extension BraveWallet.TransactionInfo: Identifiable {
  // Already has `id` property
}

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

extension BraveWallet.NetworkInfo: Identifiable {
  public var id: String {
    chainId
  }

  public var nativeToken: BraveWallet.BlockchainToken {
    .init(
      contractAddress: "",
      name: symbolName,
      logo: iconUrls.first ?? "",
      isErc20: false,
      isErc721: false,
      symbol: symbol,
      decimals: decimals,
      visible: false,
      tokenId: "",
      coingeckoId: "",
      chainId: "",
      coin: coin
    )
  }

  public var isCustom: Bool {
    let ethNetworks = [
      BraveWallet.MainnetChainId,
      BraveWallet.RinkebyChainId,
      BraveWallet.RopstenChainId,
      BraveWallet.GoerliChainId,
      BraveWallet.KovanChainId,
      BraveWallet.LocalhostChainId,
    ]
    return !ethNetworks.contains(id)
  }
}

extension BraveWallet.BlockchainToken: Identifiable {
  public var id: String {
    symbol.lowercased()
  }

  public func contractAddress(in network: BraveWallet.NetworkInfo) -> String {
    // ETH special swap address
    // Only checking token.symbol with selected network.symbol is sufficient
    // since there is no swap support for custom networks.
    return symbol == network.symbol ? BraveWallet.ethSwapAddress : contractAddress
  }
}

extension BraveWallet {
  /// The address that is expected when you are swapping ETH via SwapService APIs
  public static let ethSwapAddress: String = "0xeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee"
}
