// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Combine
import Foundation
import XCTest

@testable import BraveWallet

class BuyTokenStoreTests: XCTestCase {
  let mockCryptoCurrencies: [BraveWallet.MeldCryptoCurrency] = [
    .init(
      currencyCode: "ETH",
      name: "Ethereum",
      chainCode: "ETH",
      chainName: "Ethereum",
      chainId: "0x1",
      contractAddress: "0x0000000000000000000000000000000000000000",
      symbolImageUrl: "https://images-currency.meld.io/crypto/ETH/symbol.png"
    ),
    .init(
      currencyCode: "SOL",
      name: "Solana",
      chainCode: "SOLANA",
      chainName: "Solana",
      chainId: "0x65",
      contractAddress: nil,
      symbolImageUrl: "https://images-currency.meld.io/crypto/SOL/symbol.png"
    ),
  ]
  let mockFiatCurrencies: [BraveWallet.MeldFiatCurrency] = [
    .init(
      currencyCode: "USD",
      name: "US Dollar",
      symbolImageUrl: "https://images-currency.meld.io/fiat/USD/symbol.png"
    )
  ]
  let mockCountries: [BraveWallet.MeldCountry] = [
    .init(
      countryCode: "US",
      name: "United States",
      flagImageUrl: nil,
      regions: nil
    )
  ]
  let mockPayments: [BraveWallet.MeldPaymentMethod] = [
    .init(
      paymentMethod: "CREDIT_DEBIT_CARD",
      name: "Credit & Debit Card",
      paymentType: "CARD",
      logoImages: .init(
        darkUrl: "https://images-paymentMethod.meld.io/CREDIT_DEBIT_CARD/logo_dark.png",
        darkShortUrl: nil,
        lightUrl: "https://images-paymentMethod.meld.io/CREDIT_DEBIT_CARD/logo_light.png",
        lightShortUrl: nil
      )
    )
  ]

  private func setupServices() -> (
    BraveWallet.TestKeyringService,
    BraveWallet.TestBraveWalletService,
    BraveWallet.TestBitcoinWalletService,
    BraveWallet.TestZCashWalletService,
    BraveWallet.TestMeldIntegrationService
  ) {
    let keyringService = BraveWallet.TestKeyringService()
    keyringService._addObserver = { _ in }
    keyringService._allAccounts = { completion in
      completion(
        .init(
          accounts: [.previewAccount, .mockSolAccount],
          selectedAccount: .previewAccount,
          ethDappSelectedAccount: .previewAccount,
          solDappSelectedAccount: nil,
          adaDappSelectedAccount: nil
        )
      )
    }

    let walletService = BraveWallet.TestBraveWalletService()

    let bitcoinWalletService = BraveWallet.TestBitcoinWalletService()

    let zcashWalletService = BraveWallet.TestZCashWalletService()

    let meldIntegrationService = BraveWallet.TestMeldIntegrationService()
    meldIntegrationService._cryptoCurrencies = { _, completion in
      completion(self.mockCryptoCurrencies, nil)
    }
    meldIntegrationService._fiatCurrencies = { _, completion in
      completion(self.mockFiatCurrencies, nil)
    }
    meldIntegrationService._countries = { _, completion in
      completion(self.mockCountries, nil)
    }
    meldIntegrationService._paymentMethods = { _, completion in
      completion(self.mockPayments, nil)
    }

    return (
      keyringService,
      walletService,
      bitcoinWalletService,
      zcashWalletService,
      meldIntegrationService
    )
  }

  @MainActor func testPrefilledToken() async {
    let defaultEth: BraveWallet.MeldCryptoCurrency = .init(
      currencyCode: "ETH",
      name: "Ethereum",
      chainCode: "ETH",
      chainName: "Ethereum",
      chainId: "0x1",
      contractAddress: "0x0000000000000000000000000000000000000000",
      symbolImageUrl: "https://images-currency.meld.io/crypto/ETH/symbol.png"
    )
    let (
      keyringService,
      walletService,
      bitcoinWalletService,
      zcashWalletService,
      meldIntegrationService
    ) = setupServices()
    var store = BuyTokenStore(
      keyringService: keyringService,
      walletService: walletService,
      bitcoinWalletService: bitcoinWalletService,
      zcashWalletService: zcashWalletService,
      meldIntegrationService: meldIntegrationService,
      prefilledToken: nil
    )
    XCTAssertNotNil(store.selectedBuyToken)

    store = BuyTokenStore(
      keyringService: keyringService,
      walletService: walletService,
      bitcoinWalletService: bitcoinWalletService,
      zcashWalletService: zcashWalletService,
      meldIntegrationService: meldIntegrationService,
      prefilledToken: .previewToken
    )

    await store.updateInfo()
    XCTAssertEqual(
      store.selectedBuyToken.coin,
      BraveWallet.BlockchainToken.previewToken.coin
    )
    XCTAssertEqual(
      store.selectedBuyToken.contractAddress,
      defaultEth.contractAddress ?? ""
    )
    XCTAssertEqual(
      store.selectedBuyToken.name,
      defaultEth.name
    )
    XCTAssertEqual(
      store.selectedBuyToken.chainId,
      defaultEth.chainId
    )
  }

  @MainActor
  func testSelectedAccount() async {
    let (
      keyringService,
      walletService,
      bitcoinWalletService,
      zcashWalletService,
      meldIntegrationService
    ) = setupServices()

    let store = BuyTokenStore(
      keyringService: keyringService,
      walletService: walletService,
      bitcoinWalletService: bitcoinWalletService,
      zcashWalletService: zcashWalletService,
      meldIntegrationService: meldIntegrationService,
      prefilledToken: .mockSolToken
    )
    // initial selected account
    XCTAssertNil(store.selectedAccount)

    await store.updateSelectedAccount()
    await store.updateInfo()
    // updated since `selectedBuyToken changed`
    guard let account = store.selectedAccount else {
      XCTFail("No selected account")
      return
    }
    XCTAssertEqual(account, .mockSolAccount)
  }

  @MainActor
  func testUpdateInfo() async {
    let (
      keyringService,
      walletService,
      bitcoinWalletService,
      zcashWalletService,
      meldIntegrationService
    ) = setupServices()
    let store = BuyTokenStore(
      keyringService: keyringService,
      walletService: walletService,
      bitcoinWalletService: bitcoinWalletService,
      zcashWalletService: zcashWalletService,
      meldIntegrationService: meldIntegrationService,
      prefilledToken: nil
    )

    await store.updateInfo()

    XCTAssertEqual(store.supportedCountries.count, 1)
    XCTAssertEqual(store.supportedCountries[safe: 0], mockCountries[safe: 0])
    XCTAssertEqual(store.supportedPaymentTypes.count, 1)
    XCTAssertEqual(store.supportedPaymentTypes[safe: 0], mockPayments[safe: 0])
    XCTAssertEqual(store.supportedFiatCurrencies.count, 1)
    XCTAssertEqual(store.supportedFiatCurrencies[safe: 0], mockFiatCurrencies[safe: 0])
    XCTAssertEqual(store.supportedCryptoCurrencies.count, 2)
    XCTAssertEqual(store.supportedCryptoCurrencies[safe: 0], mockCryptoCurrencies[safe: 0])
    XCTAssertEqual(store.supportedCryptoCurrencies[safe: 1], mockCryptoCurrencies[safe: 1])
  }
}
