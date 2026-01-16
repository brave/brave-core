// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveUI
import DesignSystem
import Preferences
import Strings
import SwiftUI

struct BuyTokenView: View {
  @ObservedObject var keyringStore: KeyringStore
  @ObservedObject var networkStore: NetworkStore
  @ObservedObject var buyTokenStore: BuyTokenStore

  @State private var isPresentingAccountPicker: Bool = false
  @State private var isPresentingMeldAPIAgreement: Bool = false
  @State private var hasReadAgreementBoxChecked: Bool = false
  @State private var sortedQuotes: [BraveWallet.MeldCryptoQuote]?
  @State private var providers: [BraveWallet.MeldServiceProvider] = []
  @ScaledMetric private var avatarSize = 24.0
  @Environment(\.presentationMode) @Binding private var presentationMode
  @Environment(\.openURL) private var openWalletURL

  var onDismiss: (() -> Void)

  @ViewBuilder private var accountPickerView: some View {
    Menu {
      if let account = buyTokenStore.selectedAccount {
        Text(buyTokenStore.selectedAccountAddress.zwspOutput)
      }
      Button {
        UIPasteboard.general.string = keyringStore.selectedAccount.address
      } label: {
        Label(Strings.Wallet.copyAddressButtonTitle, braveSystemImage: "leo.copy.plain-text")
      }
    } label: {
      if let account = buyTokenStore.selectedAccount {
        VStack(alignment: .leading) {
          HStack {
            Blockie(address: account.blockieSeed, shape: .rectangle)
              .frame(width: avatarSize, height: avatarSize)
            Text(account.name)
              .font(.title3.weight(.semibold))
              .foregroundColor(Color(.braveLabel))
            Spacer()
            Image(systemName: "chevron.down")
              .imageScale(.small)
              .foregroundColor(Color(.secondaryBraveLabel))
          }
          Text(buyTokenStore.selectedAccountAddress)
            .padding(4)
            .font(.caption)
            .foregroundColor(Color(braveSystemName: .textSecondary))
            .multilineTextAlignment(.leading)
            .background(
              RoundedRectangle(cornerRadius: 4, style: .continuous)
                .fill(Color(braveSystemName: .pageBackground))
            )
        }
      }
    } primaryAction: {
      isPresentingAccountPicker = true
    }
    .accessibilityLabel(Strings.Wallet.selectedAccountAccessibilityLabel)
    .accessibilityValue(
      "\(buyTokenStore.selectedAccount?.name ?? "")"
    )
  }

  var body: some View {
    NavigationView {
      Form {
        Section(
          header: WalletListHeaderView(title: Text(Strings.Wallet.buy))
        ) {
          NavigationLink(
            destination: BuyTokenSearchView(
              buyTokenStore: buyTokenStore,
              keyringStore: keyringStore,
              networkStore: networkStore
            )
          ) {
            HStack {
              AssetIconView(
                token: nil,
                meldCryptoCurrency: buyTokenStore.selectedBuyToken,
                shouldShowNetworkIcon: true,
                length: 26
              )
              Text(
                buyTokenStore.selectedBuyToken.name ?? buyTokenStore.selectedBuyToken.displaySymbol
              )
              .font(.title3.weight(.semibold))
              .foregroundColor(Color(.braveLabel))
            }
            .padding(.vertical, 8)
          }
          .listRowBackground(Color(.secondaryBraveGroupedBackground))
        }
        Section(
          header: WalletListHeaderView(title: Text(Strings.Wallet.meldAccountHeaderTitle))
        ) {
          accountPickerView
            .padding(.vertical, 8)
            .listRowBackground(Color(.secondaryBraveGroupedBackground))
        }
        Section(
          header: WalletListHeaderView(title: Text(Strings.Wallet.enterAmount))
        ) {
          HStack {
            Menu {
              Picker(
                selection: $buyTokenStore.selectedFiatCurrency,
                content: {
                  ForEach(buyTokenStore.supportedFiatCurrencies, id: \.currencyCode) { currency in
                    Text(currency.name ?? "")
                      .foregroundColor(Color(.secondaryBraveLabel))
                      .tag(currency)
                  }
                },
                label: { EmptyView() }  // `Menu` label is used instead
              )
              EmptyView()
            } label: {
              HStack(spacing: 4) {
                Text(buyTokenStore.selectedFiatCurrency.currencyCode)
                  .font(.title2.weight(.bold))
                  .foregroundColor(Color(.braveLabel))
                Image(systemName: "chevron.down")
                  .imageScale(.small)
                  .foregroundColor(Color(.secondaryBraveLabel))
              }
            }
            TextField("0", text: $buyTokenStore.buyAmount)
              .keyboardType(.decimalPad)
          }
          .padding(.vertical, 8)
          .listRowBackground(Color(.secondaryBraveGroupedBackground))
        }
        Section(
          header: WalletListHeaderView(title: Text(Strings.Wallet.meldCountryHeaderTitle))
        ) {
          Menu {
            Picker(
              selection: $buyTokenStore.selectedCountry,
              content: {
                ForEach(buyTokenStore.supportedCountries, id: \.countryCode) { country in
                  Text(country.name ?? "")
                    .foregroundColor(Color(.secondaryBraveLabel))
                    .tag(country)
                }
              },
              label: { EmptyView() }
            )
            EmptyView()
          } label: {
            HStack(spacing: 4) {
              Text(buyTokenStore.selectedCountry.name ?? "")
                .foregroundColor(Color(.braveLabel))
              Image(systemName: "chevron.down")
                .imageScale(.small)
                .foregroundColor(Color(.secondaryBraveLabel))
            }
          }
          .padding(.vertical, 8)
          .listRowBackground(Color(.secondaryBraveGroupedBackground))
        }
        Section(
          header: WalletListHeaderView(title: Text(Strings.Wallet.meldPaymentMethodHeaderTitle))
        ) {
          Menu {
            Picker(
              selection: $buyTokenStore.selectedPaymentType,
              content: {
                ForEach(buyTokenStore.supportedPaymentTypes, id: \.paymentMethod) { type in
                  Text(type.name ?? "")
                    .foregroundColor(Color(.secondaryBraveLabel))
                    .tag(type)
                }
              },
              label: { EmptyView() }
            )
            EmptyView()
          } label: {
            HStack(spacing: 4) {
              Text(buyTokenStore.selectedPaymentType.name ?? "")
                .foregroundColor(Color(.braveLabel))
              Image(systemName: "chevron.down")
                .imageScale(.small)
                .foregroundColor(Color(.secondaryBraveLabel))
            }
          }
          .padding(.vertical, 8)
          .listRowBackground(Color(.secondaryBraveGroupedBackground))
        }
        Section(
          header: HStack {
            WalletLoadingButton(
              isLoading: (buyTokenStore.isFetchingServiceProviders
                || buyTokenStore.isFetchingPrefilledToken)
            ) {
              Task {
                (sortedQuotes, providers) = await buyTokenStore.fetchProviders()
              }
            } label: {
              Text(Strings.Wallet.purchaseMethodButtonTitle)
            }
            .buttonStyle(BraveFilledButtonStyle(size: .normal))
            .frame(maxWidth: .infinity)
          }
          .resetListHeaderStyle()
          .listRowBackground(Color(.clear))
        ) {
        }
      }
      .environment(\.defaultMinListHeaderHeight, 0)
      .environment(\.defaultMinListRowHeight, 0)
      .listBackgroundColor(Color(UIColor.braveGroupedBackground))
      .navigationTitle(Strings.Wallet.buy)
      .navigationBarTitleDisplayMode(.inline)
      .toolbar {
        ToolbarItemGroup(placement: .cancellationAction) {
          Button {
            onDismiss()
          } label: {
            Text(Strings.cancelButtonTitle)
              .foregroundColor(Color(.braveBlurpleTint))
          }
        }
      }
      .alert(isPresented: $buyTokenStore.encounterMeldAPIError) {
        Alert(
          title: Text(Strings.genericErrorTitle),
          message: Text(Strings.Wallet.internalErrorMessage),
          dismissButton: .default(
            Text(Strings.OKString),
            action: {
              presentationMode.dismiss()
            }
          )
        )
      }
      .sheet(isPresented: $isPresentingAccountPicker) {
        NavigationView {
          AccountSelectionRootView(
            navigationTitle: Strings.Wallet.selectAccountTitle,
            allAccounts: buyTokenStore.availableAccountsForSelectedBuyToken(),
            selectedAccounts: [buyTokenStore.selectedAccount!],
            showsSelectAllButton: false,
            selectAccount: { selectedAccount in
              buyTokenStore.selectedAccount = selectedAccount
              isPresentingAccountPicker = false
            }
          )
        }
        .navigationViewStyle(.stack)
      }
      .background(
        NavigationLink(
          isActive: Binding(
            get: { sortedQuotes != nil },
            set: { if !$0 { sortedQuotes = nil } }
          ),
          destination: {
            if let sortedQuotes {
              BuyProviderSelectionView(
                buyTokenStore: buyTokenStore,
                keyringStore: keyringStore,
                sortedQuotes: sortedQuotes,
                providers: providers
              )
            }
          },
          label: {
            EmptyView()
          }
        )
      )
      .background(
        WalletPromptView(
          isPresented: $isPresentingMeldAPIAgreement,
          isPrimaryButtonEnabled: $hasReadAgreementBoxChecked,
          primaryButton: .init(
            title: Strings.Wallet.continueButtonTitle,
            action: { _ in
              if hasReadAgreementBoxChecked {
                Preferences.Wallet.meldAPIAgreementShownAndAgreed.value = true
                Task { @MainActor in
                  isPresentingMeldAPIAgreement = false
                  await buyTokenStore.updateInfo()
                }
              }
            }
          ),
          secondaryButton: .init(
            title: Strings.CancelString,
            action: { _ in
              isPresentingMeldAPIAgreement = false
              presentationMode.dismiss()
            }
          ),
          buttonsAxis: .horizontal,
          showCloseButton: true,
          dismissAction: { _ in
            presentationMode.dismiss()
          },
          content: {
            VStack(spacing: 16) {
              Text(Strings.Wallet.meldTransactionPartner)
                .font(.headline.weight(.semibold))
                .foregroundColor(Color(braveSystemName: .textPrimary))
                .multilineTextAlignment(.center)
              Image("transaction-partner", bundle: .module)
                .resizable()
                .aspectRatio(3.7, contentMode: .fit)
              Text(Strings.Wallet.meldTransactionPartnerDescription)
                .font(.callout)
                .foregroundColor(Color(braveSystemName: .textPrimary))
              HStack(spacing: 8) {
                WalletCheckbox(isChecked: $hasReadAgreementBoxChecked)
                  .font(.title2)
                Text(
                  LocalizedStringKey(
                    String.localizedStringWithFormat(
                      Strings.Wallet.meldTransactionPartnerLegal,
                      WalletConstants.meldTermOfUseLink.absoluteDisplayString
                    )
                  )
                )
                .foregroundColor(Color(braveSystemName: .textPrimary))
                .tint(Color(braveSystemName: .textInteractive))
                .font(.callout)
                .frame(maxWidth: .infinity, alignment: .leading)
                .onTapGesture {
                  hasReadAgreementBoxChecked.toggle()
                }
              }
            }
            .padding(.bottom, 16)
          }
        )
      )
      .onAppear {
        if !Preferences.Wallet.meldAPIAgreementShownAndAgreed.value {
          DispatchQueue.main.asyncAfter(deadline: .now() + 0.5) {
            isPresentingMeldAPIAgreement = true
          }
        }
      }
    }
    .navigationViewStyle(.stack)
  }
}

#if DEBUG
struct BuyTokenView_Previews: PreviewProvider {
  static var previews: some View {
    BuyTokenView(
      keyringStore: .previewStore,
      networkStore: .previewStore,
      buyTokenStore: .previewStore,
      onDismiss: {}
    )
    .previewColorSchemes()
  }
}
#endif
