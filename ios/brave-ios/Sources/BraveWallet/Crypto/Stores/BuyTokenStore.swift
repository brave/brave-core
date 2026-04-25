// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Combine
import Foundation
import OrderedCollections
import Preferences
import os.log

/// A store contains data for buying tokens
public class BuyTokenStore: ObservableObject {
  /// The current selected token to buy. Default with nil value.
  @Published var selectedBuyToken: BraveWallet.MeldCryptoCurrency = WalletConstants.defaultBuyToken
  {
    didSet {
      if selectedBuyToken.coin != selectedAccount?.coin {
        if let matchedAccount = allAccounts.first(where: { $0.coin == selectedBuyToken.coin }) {
          selectedAccount = matchedAccount
        }
      }
    }
  }
  @Published var selectedAccount: BraveWallet.AccountInfo? {
    didSet {
      if oldValue != selectedAccount {
        Task { @MainActor in
          await updateSelectedAccountAddress()
        }
      }
    }
  }
  @Published private(set) var selectedAccountAddress: String = ""
  /// The country input to get available service provider from Meld
  @Published var selectedCountry: BraveWallet.MeldCountry = .init(
    countryCode: "US",
    name: "United States",
    flagImageUrl: nil,
    regions: nil
  )
  /// The currency user wishes to purchase with
  @Published var selectedFiatCurrency: BraveWallet.MeldFiatCurrency = WalletConstants
    .defaultFiatCurrency
  /// The amount user wishes to purchase
  @Published var buyAmount: String = "100"
  /// The payment type user wishes to use
  @Published var selectedPaymentType: BraveWallet.MeldPaymentMethod = WalletConstants
    .defaultPaymentMethod
  /// Supported service provider for user selected account, crypto currency, fiat currency, country
  @Published var supportedServiceProviders: [BraveWallet.MeldServiceProvider] = []
  /// The supported crypto currencies for purchasing
  @Published var supportedCryptoCurrencies: [BraveWallet.MeldCryptoCurrency] = []
  /// The supported fiat currencies for purchasing
  @Published var supportedFiatCurrencies: [BraveWallet.MeldFiatCurrency] = []
  /// The supported countries for purchasing
  @Published var supportedCountries: [BraveWallet.MeldCountry] = []
  /// The supported payment types  for purchasing
  @Published var supportedPaymentTypes: [BraveWallet.MeldPaymentMethod] = []
  /// Meld API error
  @Published var encounterMeldAPIError: Bool = false
  /// Indicating fetching service providers
  @Published var isFetchingServiceProviders: Bool = false
  /// Indicating fetching prefilled token
  @Published var isFetchingPrefilledToken: Bool = false

  var allAccounts: [BraveWallet.AccountInfo] = []

  private var savedBuyTokenAndAccount:
    (token: BraveWallet.MeldCryptoCurrency, account: BraveWallet.AccountInfo)?

  private let keyringService: BraveWalletKeyringService
  private let walletService: BraveWalletBraveWalletService
  private let bitcoinWalletService: BraveWalletBitcoinWalletService
  private let zcashWalletService: BraveWalletZCashWalletService
  private let meldIntegrationService: BraveWalletMeldIntegrationService
  private var prefilledToken: BraveWallet.BlockchainToken?
  private var keyringServiceObserver: KeyringServiceObserver?

  var isObserving: Bool {
    keyringServiceObserver != nil
  }

  public init(
    keyringService: BraveWalletKeyringService,
    walletService: BraveWalletBraveWalletService,
    bitcoinWalletService: BraveWalletBitcoinWalletService,
    zcashWalletService: BraveWalletZCashWalletService,
    meldIntegrationService: BraveWalletMeldIntegrationService,
    prefilledToken: BraveWallet.BlockchainToken?
  ) {
    self.keyringService = keyringService
    self.walletService = walletService
    self.bitcoinWalletService = bitcoinWalletService
    self.zcashWalletService = zcashWalletService
    self.meldIntegrationService = meldIntegrationService
    self.prefilledToken = prefilledToken

    Task {
      await updateSelectedAccount()
      if Preferences.Wallet.meldAPIAgreementShownAndAgreed.value {
        await updateInfo()
      }
    }
  }

  @MainActor
  func fetchBuyUrl(
    provider: BraveWallet.MeldServiceProvider
  ) async -> URL? {
    guard !selectedAccountAddress.isEmpty else { return nil }

    let (widget, error) = await meldIntegrationService.cryptoBuyWidgetCreate(
      sessionData: .init(
        countryCode: selectedCountry.countryCode,
        destinationCurrencyCode: selectedBuyToken.currencyCode,
        lockFields: nil,
        paymentMethodType: selectedPaymentType.paymentType,
        redirectUrl: nil,
        serviceProvider: provider.serviceProvider,
        sourceAmount: buyAmount,
        sourceCurrencyCode: selectedFiatCurrency.currencyCode,
        walletAddress: selectedAccountAddress,
        walletTag: nil
      ),
      customerData: nil
    )

    guard let widget, error == nil else { return nil }
    return URL(string: widget.widgetUrl)
  }

  @MainActor
  func updateSelectedAccountAddress() async {
    guard let selectedAccount else { return }
    selectedAccountAddress = selectedAccount.address
    if selectedAccount.coin == .btc,
      let bitcoinAccountInfo = await bitcoinWalletService.bitcoinAccountInfo(
        accountId: selectedAccount.accountId
      )
    {
      selectedAccountAddress = bitcoinAccountInfo.nextChangeAddress.addressString
    } else if selectedAccount.coin == .zec,
      let zcashAccountInfo = await zcashWalletService.zCashAccountInfo(
        accountId: selectedAccount.accountId
      )
    {
      selectedAccountAddress = zcashAccountInfo.nextTransparentChangeAddress.addressString
    }
  }

  @MainActor
  func updateSelectedAccount() async {
    allAccounts = await keyringService.allAccounts().accounts
    selectedAccount = allAccounts.first(where: { accountInfo in
      accountInfo.coin == selectedBuyToken.coin
    })
    savedBuyTokenAndAccount = nil
  }

  @MainActor
  func updateInfo() async {
    isFetchingPrefilledToken = prefilledToken != nil

    if let prefilledToken {
      let (matchedCryptoCurrency, allMeldSupportedCryptoCurrencies) =
        await meldIntegrationService.convertToMeldCryptoCurrency(for: prefilledToken)
      guard let allMeldSupportedCryptoCurrencies else {
        encounterMeldAPIError = true
        Logger.module.debug("Meld - Error getting crypto currencies")
        return
      }
      supportedCryptoCurrencies = allMeldSupportedCryptoCurrencies
      if let matchedCryptoCurrency {
        selectedBuyToken = matchedCryptoCurrency
      }
      isFetchingPrefilledToken = false
    } else {
      // get all crypto currencies
      let (cryptoCurrencies, _) = await meldIntegrationService.cryptoCurrencies(
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
      guard let cryptoCurrencies else {
        encounterMeldAPIError = true
        Logger.module.debug("Meld - Error getting crypto currencies")
        return
      }
      supportedCryptoCurrencies = cryptoCurrencies
    }

    // get all fiat currencies
    let (fiatCurrencies, _) = await meldIntegrationService.fiatCurrencies(filter: .init())
    guard let fiatCurrencies else {
      encounterMeldAPIError = true
      Logger.module.debug("Meld - Error getting fiat currencies")
      return
    }
    supportedFiatCurrencies = fiatCurrencies

    // get all countries
    let (countries, _) = await meldIntegrationService.countries(filter: .init())
    guard let countries else {
      encounterMeldAPIError = true
      Logger.module.debug("Meld - Error getting countries")
      return
    }
    supportedCountries = countries

    // get all payment types
    let (paymentTypes, _) = await meldIntegrationService.paymentMethods(
      filter: .init(
        countries: selectedCountry.countryCode,
        fiatCurrencies: selectedFiatCurrency.currencyCode,
        cryptoCurrencies: nil,
        cryptoChains: nil,
        serviceProviders: nil,
        paymentMethodTypes: nil,
        statuses: nil
      )
    )
    guard let paymentTypes else {
      encounterMeldAPIError = true
      Logger.module.debug("Meld - Error getting payment types")
      return
    }
    supportedPaymentTypes = paymentTypes
  }

  @MainActor
  func fetchProviders() async -> ([BraveWallet.MeldCryptoQuote], [BraveWallet.MeldServiceProvider])
  {
    guard let selectedAccount else { return ([], []) }
    isFetchingServiceProviders = true
    let (quotes, _) = await meldIntegrationService.cryptoQuotes(
      country: selectedCountry.countryCode,
      sourceCurrencyCode: selectedFiatCurrency.currencyCode,
      destinationCurrencyCode: selectedBuyToken.currencyCode,
      sourceAmount: Double(buyAmount) ?? 0,
      account: selectedAccount.address,
      paymentMethod: selectedPaymentType.paymentType
    )
    let sortedQuotes = quotes?.sorted { left, right in
      if let leftDestinationAmount = left.destinationAmount,
        let rightDestinationAmount = right.destinationAmount,
        let leftAmount = Double(leftDestinationAmount),
        let rightAmount = Double(rightDestinationAmount)
      {
        return leftAmount > rightAmount
      }
      return false
    }
    let providerNames =
      sortedQuotes?.compactMap { quote in
        quote.serviceProvider
      } ?? []
    let (providers, _) = await meldIntegrationService.serviceProviders(
      filter: .init(
        countries: nil,
        fiatCurrencies: nil,
        cryptoCurrencies: nil,
        cryptoChains: nil,
        serviceProviders: providerNames.joined(separator: ","),
        paymentMethodTypes: nil,
        statuses: "LIVE"
      )
    )
    isFetchingServiceProviders = false
    return (sortedQuotes ?? [], providers ?? [])
  }

  func availableAccountsForSelectedBuyToken() -> [BraveWallet.AccountInfo] {
    return allAccounts.filter {
      let selectedTokenCoin = selectedBuyToken.coin
      return $0.coin == selectedTokenCoin
    }
  }

  @MainActor func handleDismissAddAccount(
    selectingToken: BraveWallet.MeldCryptoCurrency
  ) async -> Bool {
    if await keyringService.isAccountAvailable(
      for: selectingToken.coin
    ) {
      self.selectedBuyToken = selectingToken
      await self.updateSelectedAccount()
      return true
    } else {
      return false
    }
  }

  func handleCancelAddAccount() {
    if let savedBuyTokenAndAccount {
      selectedBuyToken = savedBuyTokenAndAccount.token
      selectedAccount = savedBuyTokenAndAccount.account
      self.savedBuyTokenAndAccount = nil
    }
  }

  func hasMatchedCoinTypeAccount(for asset: BraveWallet.MeldCryptoCurrency) -> Bool {
    return allAccounts.contains { account in
      account.coin == asset.coin
    }
  }
}
