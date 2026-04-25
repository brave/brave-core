// Copyright 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore

extension BraveWalletMeldIntegrationService {
  /// - Parameters:
  ///     - token: A `BraveWallet.BlockchainToken` from token registry that we want to convert from
  ///
  /// - Returns: A  tuple of `BraveWallet.MeldCryptoCurrency` and a list of BraveWallet.MeldCryptoCurrency.
  /// The first value of the tuple would be the meld supported crypto currency if we found one, otherwise it would be nil.
  /// The second value of the tuple would be all meld supported crypto currencies with a default filter and the given
  /// network info if it fetches without an error, otherwise it would be nil
  func convertToMeldCryptoCurrency(
    for token: BraveWallet.BlockchainToken
  ) async -> (BraveWallet.MeldCryptoCurrency?, [BraveWallet.MeldCryptoCurrency]?) {
    let (allMeldSupportedTokens, _) = await cryptoCurrencies(
      filter: .init(
        countries: nil,
        fiatCurrencies: nil,
        cryptoCurrencies: nil,
        cryptoChains: WalletConstants.supportedChainsForMeld.joined(separator: ","),
        serviceProviders: nil,
        paymentMethodTypes: nil,
        statuses: nil
      )
    )
    guard let allMeldSupportedTokens else {
      return (nil, nil)
    }

    if token.contractAddress.isEmpty,
      let foundNativeToken = allMeldSupportedTokens.first(where: {
        if let cryptoChainId = $0.chainId {
          return cryptoChainId.caseInsensitiveCompare(token.chainId) == .orderedSame
            && $0.displaySymbol.caseInsensitiveCompare(token.symbol) == .orderedSame
        }
        return false
      })
    {
      return (foundNativeToken, allMeldSupportedTokens)
    } else if let foundTokenByContractAddress = allMeldSupportedTokens.first(where: {
      if let cryptoContractAddress = $0.contractAddress,
        let cryptoChainId = $0.chainId
      {
        return cryptoContractAddress.caseInsensitiveCompare(
          token.contractAddress
        ) == .orderedSame
          && cryptoChainId.caseInsensitiveCompare(token.chainId) == .orderedSame
      }
      return false
    }) {
      return (foundTokenByContractAddress, allMeldSupportedTokens)
    } else if let foundTokenBySymbol = allMeldSupportedTokens.first(where: {
      $0.displaySymbol.caseInsensitiveCompare(token.symbol) == .orderedSame
    }) {
      return (foundTokenBySymbol, allMeldSupportedTokens)
    }
    return (nil, allMeldSupportedTokens)
  }
}
