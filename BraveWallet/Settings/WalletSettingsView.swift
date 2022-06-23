// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import SwiftUI
import Strings
import BraveUI
import BraveShared

public struct WalletSettingsView: View {
  @ObservedObject var settingsStore: SettingsStore
  @ObservedObject var networkStore: NetworkStore
  @ObservedObject var keyringStore: KeyringStore
  @ObservedObject var defaultWallet = Preferences.Wallet.defaultWallet
  @ObservedObject var allowDappsRequestAccounts = Preferences.Wallet.allowEthereumProviderAccountRequests
  @ObservedObject var displayDappsNotifications = Preferences.Wallet.displayWeb3Notifications

  @State private var isShowingResetWalletAlert = false
  @State private var isShowingResetTransactionAlert = false
  /// If we are showing the modal so the user can enter their password to enable unlock via biometrics.
  @State private var isShowingBiometricsPasswordEntry = false

  public init(
    settingsStore: SettingsStore,
    networkStore: NetworkStore,
    keyringStore: KeyringStore
  ) {
    self.settingsStore = settingsStore
    self.networkStore = networkStore
    self.keyringStore = keyringStore
  }

  private var autoLockIntervals: [AutoLockInterval] {
    var all = AutoLockInterval.allOptions
    if !all.contains(settingsStore.autoLockInterval) {
      // Ensure that the users selected interval always appears as an option even if
      // we remove it from `allOptions`
      all.append(settingsStore.autoLockInterval)
    }
    return all.sorted(by: { $0.value < $1.value })
  }

  public var body: some View {
    List {
      Section(
        footer: Text(Strings.Wallet.autoLockFooter)
          .foregroundColor(Color(.secondaryBraveLabel))
      ) {
        Picker(selection: $settingsStore.autoLockInterval) {
          ForEach(autoLockIntervals) { interval in
            Text(interval.label)
              .foregroundColor(Color(.secondaryBraveLabel))
              .tag(interval)
          }
        } label: {
          Text(Strings.Wallet.autoLockTitle)
            .foregroundColor(Color(.braveLabel))
            .padding(.vertical, 4)
        }
      }
      .listRowBackground(Color(.secondaryBraveGroupedBackground))
      Section {
        Picker(selection: $settingsStore.currencyCode) {
          ForEach(CurrencyCode.allCurrencyCodes) { currencyCode in
            Text(currencyCode.code)
              .foregroundColor(Color(.secondaryBraveLabel))
              .tag(currencyCode)
          }
        } label: {
          Text(Strings.Wallet.settingsDefaultBaseCurrencyTitle)
            .foregroundColor(Color(.braveLabel))
            .padding(.vertical, 4)
        }
      }
      .listRowBackground(Color(.secondaryBraveGroupedBackground))
      if settingsStore.isBiometricsAvailable, keyringStore.keyring.isKeyringCreated {
        Section(
          footer: Text(Strings.Wallet.settingsEnableBiometricsFooter)
            .foregroundColor(Color(.secondaryBraveLabel))
        ) {
          Toggle(
            Strings.Wallet.settingsEnableBiometricsTitle,
            isOn: Binding(get: { settingsStore.isBiometricsUnlockEnabled },
                          set: { toggledBiometricsUnlock($0) })
          )
            .foregroundColor(Color(.braveLabel))
            .toggleStyle(SwitchToggleStyle(tint: Color(.braveBlurpleTint)))
        }
        .listRowBackground(Color(.secondaryBraveGroupedBackground))
      }
      Section(
        footer: Text(Strings.Wallet.networkFooter)
          .foregroundColor(Color(.secondaryBraveLabel))
      ) {
        NavigationLink(destination: CustomNetworkListView(networkStore: networkStore)) {
          Text(Strings.Wallet.settingsNetworkButtonTitle)
            .foregroundColor(Color(.braveLabel))
        }
      }
      .listRowBackground(Color(.secondaryBraveGroupedBackground))
      Section(
        header: Text(Strings.Wallet.web3PreferencesSectionTitle)
          .foregroundColor(Color(.secondaryBraveLabel))
      ) {
        HStack {
          Text(Strings.Wallet.web3PreferencesDefaultWallet)
            .foregroundColor(Color(.braveLabel))
          Spacer()
          Menu {
            Picker("", selection: $defaultWallet.value) {
              ForEach(Preferences.Wallet.WalletType.allCases) { walletType in
                Text(walletType.name)
                  .tag(walletType)
              }
            }
            .pickerStyle(.inline)
          } label: {
            let wallet = Preferences.Wallet.WalletType(rawValue: defaultWallet.value) ?? .none
            Text(wallet.name)
              .foregroundColor(Color(.braveBlurpleTint))
          }
        }
        Toggle(Strings.Wallet.web3PreferencesAllowSiteToRequestAccounts, isOn: $allowDappsRequestAccounts.value)
          .foregroundColor(Color(.braveLabel))
          .toggleStyle(SwitchToggleStyle(tint: Color(.braveBlurpleTint)))
        Toggle(Strings.Wallet.web3PreferencesDisplayWeb3Notifications, isOn: $displayDappsNotifications.value)
          .foregroundColor(Color(.braveLabel))
          .toggleStyle(SwitchToggleStyle(tint: Color(.braveBlurpleTint)))
        NavigationLink(
          destination: ManageSiteConnectionsView(
            siteConnectionStore: settingsStore.manageSiteConnectionsStore(keyringStore: keyringStore)
          )
          .onDisappear {
            settingsStore.closeManageSiteConnectionStore()
          }
        ) {
          Text(Strings.Wallet.web3PreferencesManageSiteConnections)
            .foregroundColor(Color(.braveLabel))
        }
      }
      .listRowBackground(Color(.secondaryBraveGroupedBackground))
      Section(
        footer: Text(Strings.Wallet.settingsResetTransactionFooter)
          .foregroundColor(Color(.secondaryBraveLabel))
      ) {
        Button(action: { isShowingResetTransactionAlert = true }) {
          Text(Strings.Wallet.settingsResetTransactionTitle)
            .foregroundColor(Color(.braveBlurpleTint))
        }
      }
      .listRowBackground(Color(.secondaryBraveGroupedBackground))
      Section {
        Button(action: { isShowingResetWalletAlert = true }) {
          Text(Strings.Wallet.settingsResetButtonTitle)
            .foregroundColor(.red)
        }
        // iOS 15: .role(.destructive)
      }
      .listRowBackground(Color(.secondaryBraveGroupedBackground))
    }
    .listStyle(InsetGroupedListStyle())
    .navigationTitle(Strings.Wallet.braveWallet)
    .navigationBarTitleDisplayMode(.inline)
    .background(
      Color.clear
        .alert(isPresented: $isShowingResetTransactionAlert) {
          Alert(
            title: Text(Strings.Wallet.settingsResetTransactionAlertTitle),
            message: Text(Strings.Wallet.settingsResetTransactionAlertMessage),
            primaryButton: .destructive(
              Text(Strings.Wallet.settingsResetTransactionAlertButtonTitle),
              action: {
                settingsStore.resetTransaction()
              }),
            secondaryButton: .cancel(Text(Strings.cancelButtonTitle))
          )
        }
    )
    .background(
      Color.clear
        .alert(isPresented: $isShowingResetWalletAlert) {
          Alert(
            title: Text(Strings.Wallet.settingsResetWalletAlertTitle),
            message: Text(Strings.Wallet.settingsResetWalletAlertMessage),
            primaryButton: .destructive(
              Text(Strings.Wallet.settingsResetWalletAlertButtonTitle),
              action: {
                settingsStore.reset()
              }),
            secondaryButton: .cancel(Text(Strings.no))
          )
        }
    )
    .background(
      Color.clear
        .sheet(isPresented: $isShowingBiometricsPasswordEntry) {
          BiometricsPasscodeEntryView(keyringStore: keyringStore)
        }
    )
  }
  
  private func toggledBiometricsUnlock(_ enabled: Bool) {
    if enabled {
      self.isShowingBiometricsPasswordEntry = true
    } else {
      keyringStore.resetKeychainStoredPassword()
    }
  }
}

#if DEBUG
struct WalletSettingsView_Previews: PreviewProvider {
  static var previews: some View {
    NavigationView {
      WalletSettingsView(
        settingsStore: .previewStore,
        networkStore: .previewStore,
        keyringStore: .previewStore
      )
    }
  }
}
#endif
