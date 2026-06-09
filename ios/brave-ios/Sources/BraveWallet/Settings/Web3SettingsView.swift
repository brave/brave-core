// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveUI
import Preferences
import Strings
import SwiftUI

public struct Web3SettingsView: View {
  var settingsStore: SettingsStore?
  var networkStore: NetworkStore?
  var keyringStore: KeyringStore?

  @State private var isShowingResetWalletAlert = false
  @State private var isShowingResetTransactionAlert = false
  /// If we are showing the modal so the user can enter their password to enable unlock via biometrics.
  @State private var isShowingBiometricsPasswordEntry = false

  public init(
    settingsStore: SettingsStore? = nil,
    networkStore: NetworkStore? = nil,
    keyringStore: KeyringStore? = nil
  ) {
    self.settingsStore = settingsStore
    self.networkStore = networkStore
    self.keyringStore = keyringStore
  }

  public var body: some View {
    Form {
      if let settingsStore = settingsStore {
        if let networkStore = networkStore, let keyringStore = keyringStore {
          WalletSettingsView(
            settingsStore: settingsStore,
            networkStore: networkStore,
            keyringStore: keyringStore,
            isShowingResetWalletAlert: $isShowingResetWalletAlert,
            isShowingResetTransactionAlert: $isShowingResetTransactionAlert,
            isShowingBiometricsPasswordEntry: $isShowingBiometricsPasswordEntry
          )
        }
      }
      if let settingsStore {
        Web3DomainSettingsView(settingsStore: settingsStore)
      }
    }
    .listStyle(InsetGroupedListStyle())
    .navigationTitle(Strings.Wallet.web3)
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
                settingsStore?.resetTransaction()
              }
            ),
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
                settingsStore?.reset()
              }
            ),
            secondaryButton: .cancel(Text(Strings.no))
          )
        }
    )
    .background(
      Color.clear
        .sheet(isPresented: $isShowingBiometricsPasswordEntry) {
          if let keyringStore, let settingsStore {
            BiometricsPasscodeEntryView(
              keyringStore: keyringStore,
              settingsStore: settingsStore
            )
          }
        }
    )
    .onAppear {
      settingsStore?.setup()
    }
  }
}

private struct WalletSettingsView: View {
  @ObservedObject var settingsStore: SettingsStore
  @ObservedObject var networkStore: NetworkStore
  @ObservedObject var keyringStore: KeyringStore
  @ObservedObject var displayDappsNotifications = Preferences.Wallet.displayWeb3Notifications

  @Binding var isShowingResetWalletAlert: Bool
  @Binding var isShowingResetTransactionAlert: Bool
  @Binding var isShowingBiometricsPasswordEntry: Bool

  private var autoLockIntervals: [AutoLockInterval] {
    var all = AutoLockInterval.allOptions
    if !all.contains(settingsStore.autoLockInterval) {
      // Ensure that the users selected interval always appears as an option even if
      // we remove it from `allOptions`
      all.append(settingsStore.autoLockInterval)
    }
    return all.sorted(by: { $0.value < $1.value })
  }

  var body: some View {
    if keyringStore.isWalletCreated {
      sections
    } else {
      // `KeyringStore` is optional in `Web3SettingsView`, but observed here.
      // When wallet is reset, we need SwiftUI to be notified `isWalletCreated`
      // changed so we can hide Wallet specific sections
      EmptyView()
    }
  }

  @ViewBuilder private var sections: some View {
    Section(
      footer: Text(Strings.Wallet.autoLockFooter)
        .foregroundColor(Color(braveSystemName: .textSecondary))
    ) {
      FormPicker(selection: $settingsStore.autoLockInterval) {
        ForEach(autoLockIntervals) { interval in
          Text(interval.label)
            .foregroundColor(Color(braveSystemName: .textSecondary))
            .tag(interval)
        }
      } label: {
        Text(Strings.Wallet.autoLockTitle)
          .foregroundColor(Color(braveSystemName: .textPrimary))
          .padding(.vertical, 4)
      }
    }
    Section {
      FormPicker(selection: $settingsStore.currencyCode) {
        ForEach(CurrencyCode.allCodes) { currencyCode in
          Text(currencyCode.code)
            .foregroundColor(Color(braveSystemName: .textSecondary))
            .tag(currencyCode)
        }
      } label: {
        Text(Strings.Wallet.settingsDefaultBaseCurrencyTitle)
          .foregroundColor(Color(braveSystemName: .textPrimary))
          .padding(.vertical, 4)
      }
    }
    if settingsStore.isBiometricsAvailable, keyringStore.isWalletCreated {
      Section(
        footer: Text(Strings.Wallet.settingsEnableBiometricsFooter)
          .foregroundColor(Color(braveSystemName: .textSecondary))
      ) {
        Toggle(
          Strings.Wallet.settingsEnableBiometricsTitle,
          isOn: Binding(
            get: { settingsStore.isBiometricsUnlockEnabled },
            set: { toggledBiometricsUnlock($0) }
          )
        )
        .foregroundColor(Color(braveSystemName: .textPrimary))
        .tint(Color(braveSystemName: .primitivePrimary40))
      }
    }
    Section(
      footer: Text(
        LocalizedStringKey(
          String.localizedStringWithFormat(
            Strings.Wallet.web3SettingsEnableNFTDiscoveryFooter,
            WalletConstants.nftDiscoveryURL.absoluteDisplayString
          )
        )
      )
      .foregroundColor(Color(braveSystemName: .textSecondary))
      .tint(Color(braveSystemName: .textInteractive))
    ) {
      Toggle(
        Strings.Wallet.web3SettingsEnableNFTDiscovery,
        isOn: $settingsStore.isNFTDiscoveryEnabled
      )
      .foregroundColor(Color(braveSystemName: .textPrimary))
      .tint(Color(braveSystemName: .primitivePrimary40))
    }
    Section(
      footer: Text(Strings.Wallet.networkFooter)
        .foregroundColor(Color(braveSystemName: .textSecondary))
    ) {
      NavigationLink(destination: NetworkListView(networkStore: networkStore)) {
        Text(Strings.Wallet.settingsNetworkButtonTitle)
          .foregroundColor(Color(braveSystemName: .textPrimary))
      }
    }
    Section(
      header: Text(Strings.Wallet.web3PreferencesSectionTitle)
        .foregroundColor(Color(braveSystemName: .textSecondary))
    ) {
      Group {
        ForEach(WalletConstants.supportedCoinTypes(.dapps)) { coin in
          NavigationLink(
            destination:
              DappsSettings(
                coin: coin,
                siteConnectionStore: settingsStore.manageSiteConnectionsStore(
                  keyringStore: keyringStore
                )
              )
              .onDisappear {
                settingsStore.closeManageSiteConnectionStore()
              }
          ) {
            Text(coin.localizedTitle)
              .foregroundColor(Color(braveSystemName: .textPrimary))
          }
        }
        Toggle(
          Strings.Wallet.web3PreferencesDisplayWeb3Notifications,
          isOn: $displayDappsNotifications.value
        )
        .foregroundColor(Color(braveSystemName: .textPrimary))
        .tint(Color(braveSystemName: .primitivePrimary40))
      }
    }
    Section(
      footer: Text(Strings.Wallet.settingsResetTransactionFooter)
        .foregroundColor(Color(braveSystemName: .textSecondary))
    ) {
      Button {
        isShowingResetTransactionAlert = true
      } label: {
        Text(Strings.Wallet.settingsResetTransactionTitle)
          .foregroundColor(Color(braveSystemName: .textInteractive))
      }
    }
    Section {
      Button {
        isShowingResetWalletAlert = true
      } label: {
        Text(Strings.Wallet.settingsResetButtonTitle)
          .foregroundColor(.red)
      }  // iOS 15: .role(.destructive)
    }
  }

  private func toggledBiometricsUnlock(_ enabled: Bool) {
    if enabled {
      self.isShowingBiometricsPasswordEntry = true
    } else {
      keyringStore.resetKeychainStoredPassword()
      settingsStore.updateBiometricsToggle()
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
        keyringStore: .previewStore,
        isShowingResetWalletAlert: .constant(false),
        isShowingResetTransactionAlert: .constant(false),
        isShowingBiometricsPasswordEntry: .constant(false)
      )
    }
  }
}
#endif

// Section containing the follow preferences:
// - Allow SNS Resolve (Ask/Enabled/Disabled)
// - Allow ENS Resolve (Ask/Enabled/Disabled)
// - Allow ENS Offchain Resolve (Ask/Enabled/Disabled)
private struct Web3DomainSettingsView: View {

  @ObservedObject var settingsStore: SettingsStore

  @Environment(\.openURL) private var openWalletURL

  var body: some View {
    Section(header: Text(Strings.Wallet.web3DomainOptionsHeader)) {
      Group {
        snsResolveMethodPreference
        ensResolveMethodPreference
        ensOffchainResolveMethodPreference
        udResolveMethodPreference
      }
    }
  }

  @ViewBuilder private var ensResolveMethodPreference: some View {
    FormPicker(selection: $settingsStore.ensResolveMethod) {
      ForEach(BraveWallet.ResolveMethod.allCases) { option in
        Text(option.name)
          .foregroundColor(Color(braveSystemName: .textSecondary))
          .tag(option)
      }
    } label: {
      Text(Strings.Wallet.ensResolveMethodTitle)
        .foregroundColor(Color(braveSystemName: .textPrimary))
        .padding(.vertical, 4)
    }
  }

  @ViewBuilder private var ensOffchainResolveMethodPreference: some View {
    FormPicker(selection: $settingsStore.ensOffchainResolveMethod) {
      ForEach(BraveWallet.ResolveMethod.allCases) { option in
        Text(option.name)
          .foregroundColor(Color(braveSystemName: .textSecondary))
          .tag(option)
      }
    } label: {
      VStack(alignment: .leading, spacing: 6) {
        Text(Strings.Wallet.ensOffchainResolveMethodTitle)
          .foregroundColor(Color(braveSystemName: .textPrimary))
        Text(
          LocalizedStringKey(
            String.localizedStringWithFormat(
              Strings.Wallet.ensOffchainResolveMethodDescription,
              WalletConstants.braveWalletENSOffchainURL.absoluteDisplayString
            )
          )
        )
        .foregroundColor(Color(braveSystemName: .textSecondary))
        .tint(Color(braveSystemName: .textInteractive))
        .font(.footnote)
      }
      .padding(.vertical, 4)
    }
  }

  @ViewBuilder private var snsResolveMethodPreference: some View {
    FormPicker(selection: $settingsStore.snsResolveMethod) {
      ForEach(BraveWallet.ResolveMethod.allCases) { option in
        Text(option.name)
          .foregroundColor(Color(braveSystemName: .textSecondary))
          .tag(option)
      }
    } label: {
      Text(Strings.Wallet.snsResolveMethodTitle)
        .foregroundColor(Color(braveSystemName: .textPrimary))
        .padding(.vertical, 4)
    }
  }

  @ViewBuilder private var udResolveMethodPreference: some View {
    FormPicker(selection: $settingsStore.udResolveMethod) {
      ForEach(BraveWallet.ResolveMethod.allCases) { option in
        Text(option.name)
          .foregroundColor(Color(braveSystemName: .textSecondary))
          .tag(option)
      }
    } label: {
      VStack(alignment: .leading, spacing: 6) {
        Text(Strings.Wallet.udResolveMethodTitle)
          .foregroundColor(Color(braveSystemName: .textPrimary))
        Text(
          LocalizedStringKey(
            String.localizedStringWithFormat(
              Strings.Wallet.udResolveMethodDescription,
              WalletConstants.braveWalletUnstoppableDomainsURL.absoluteDisplayString
            )
          )
        )
        .foregroundColor(Color(braveSystemName: .textSecondary))
        .tint(Color(braveSystemName: .textInteractive))
        .font(.footnote)
      }
      .padding(.vertical, 4)
    }
  }
}
