// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import SwiftUI
import BraveShared
import BraveCore

struct DappsSettings: View {
  var coin: BraveWallet.CoinType
  @ObservedObject var siteConnectionStore: ManageSiteConnectionsStore
  @ObservedObject var defaultWallet: Preferences.Option<Int>
  @ObservedObject var allowProviderAccess: Preferences.Option<Bool>
  @State private var filterText: String = ""
  @State private var isShowingConfirmAlert: Bool = false
  
  init(
    coin: BraveWallet.CoinType,
    siteConnectionStore: ManageSiteConnectionsStore
  ) {
    self.coin = coin
    self.siteConnectionStore = siteConnectionStore
    switch coin {
    case .eth:
      self.defaultWallet = Preferences.Wallet.defaultEthWallet
      self.allowProviderAccess = Preferences.Wallet.allowEthProviderAccess
    case .sol:
      self.defaultWallet = Preferences.Wallet.defaultSolWallet
      self.allowProviderAccess = Preferences.Wallet.allowSolProviderAccess
    default:
      assertionFailure("Not supported coin type.")
      self.defaultWallet = Preferences.Wallet.defaultEthWallet
      self.allowProviderAccess = Preferences.Wallet.allowEthProviderAccess
    }
  }
  
  private var defaultWalletTitle: String {
    switch coin {
    case .eth:
      return Strings.Wallet.web3PreferencesDefaultEthWallet
    case .sol:
      return Strings.Wallet.web3PreferencesDefaultSolWallet
    default:
      return ""
    }
  }
  
  private var allowProviderAccessTitle: String {
    switch coin {
    case .eth:
      return Strings.Wallet.web3PreferencesAllowEthProviderAccess
    case .sol:
      return Strings.Wallet.web3PreferencesAllowSolProviderAccess
    default:
      return ""
    }
  }
  
  private var visibleSiteConnections: [SiteConnection] {
    siteConnectionStore.siteConnections.filter(by: coin, text: filterText)
  }
  
  var body: some View {
    List {
      Section(header: Text(Strings.Wallet.dappsSettingsGeneralSectionTitle)
        .foregroundColor(Color(.secondaryBraveLabel))
      ) {
        HStack {
          Text(defaultWalletTitle)
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
        Toggle(allowProviderAccessTitle, isOn: $allowProviderAccess.value)
          .foregroundColor(Color(.braveLabel))
          .toggleStyle(SwitchToggleStyle(tint: Color(.braveBlurpleTint)))
      }
      Section(
        header: Text(Strings.Wallet.dappsSettingsConnectedSitesSectionTitle)
      ) {
        if visibleSiteConnections.isEmpty {
          HStack {
            Spacer()
            Text(Strings.Wallet.dappsSettingsConnectedSitesSectionEmpty)
              .foregroundColor(Color(.secondaryBraveLabel))
              .font(.footnote)
              .multilineTextAlignment(.center)
            Spacer()
          }
          .padding(.vertical)
        } else {
          ForEach(visibleSiteConnections) { siteConnection in
            NavigationLink(
              destination: SiteConnectionDetailView(
                siteConnection: siteConnection,
                siteConnectionStore: siteConnectionStore
              )
            ) {
              SiteRow(
                siteConnection: siteConnection
              )
              .osAvailabilityModifiers { content in
                if #available(iOS 15.0, *) {
                  content
                    .swipeActions(edge: .trailing) {
                      Button(role: .destructive, action: {
                        withAnimation {
                          siteConnectionStore.removeAllPermissions(from: [siteConnection])
                        }
                      }) {
                        Label(Strings.Wallet.delete, systemImage: "trash")
                      }
                    }
                } else {
                  content
                }
              }
            }
          }
          .onDelete { indexes in
            let visibleSiteConnections = siteConnectionStore.siteConnections
            let siteConnectionsToRemove = indexes.map { visibleSiteConnections[$0] }
            withAnimation {
              siteConnectionStore.removeAllPermissions(from: siteConnectionsToRemove)
            }
          }
        }
      }
    }
    .listStyle(InsetGroupedListStyle())
    .navigationTitle(String.localizedStringWithFormat(Strings.Wallet.dappsSettingsNavTitle, coin.localizedTitle))
    .navigationBarTitleDisplayMode(.inline)
    .filterable(text: $filterText, prompt: Strings.Wallet.manageSiteConnectionsFilterPlaceholder)
    .toolbar {
      ToolbarItemGroup(placement: .bottomBar) {
        Spacer()
        Button(action: {
          isShowingConfirmAlert = true
        }) {
          Text(Strings.Wallet.manageSiteConnectionsRemoveAll)
            .foregroundColor(siteConnectionStore.siteConnections.isEmpty ? Color(.braveDisabled) : .red)
        }
        .disabled(siteConnectionStore.siteConnections.isEmpty)
      }
    }
    .onAppear(perform: siteConnectionStore.fetchSiteConnections)
    .alert(isPresented: $isShowingConfirmAlert) {
      Alert(
        title: Text(Strings.Wallet.warningAlertConfirmation),
        message: Text(String.localizedStringWithFormat(Strings.Wallet.dappsSettingsRemoveAllWarning, visibleSiteConnections.count, visibleSiteConnections.count == 1 ? Strings.Wallet.dappsSettingsWebsiteSingular : Strings.Wallet.dappsSettingsWebsitePlural)),
        primaryButton: Alert.Button.destructive(
          Text(Strings.Wallet.manageSiteConnectionsConfirmAlertRemove),
          action: removeAll
        ),
        secondaryButton: Alert.Button.cancel(Text(Strings.CancelString))
      )
    }
  }
  
  func removeAll() {
    siteConnectionStore.removeAllPermissions(from: visibleSiteConnections)
    filterText = ""
  }
}

private struct SiteRow: View {
  
  let siteConnection: SiteConnection
  
  private let maxBlockies = 3
  @ScaledMetric private var blockieSize = 16.0
  private let maxBlockieSize: CGFloat = 32
  @ScaledMetric private var blockieDotSize = 2.0
  
  private var connectedAddresses: String {
    let account = Strings.Wallet.manageSiteConnectionsAccountSingular
    let accounts = Strings.Wallet.manageSiteConnectionsAccountPlural
    return String.localizedStringWithFormat(
      Strings.Wallet.manageSiteConnectionsAccount,
      siteConnection.connectedAddresses.count,
      siteConnection.connectedAddresses.count == 1 ? account : accounts
    )
  }
  
  var body: some View {
    VStack(alignment: .leading, spacing: 4) {
      Text(verbatim: siteConnection.url)
        .foregroundColor(Color(.braveLabel))
        .font(.headline)
      HStack {
        Text(connectedAddresses)
          .font(.subheadline)
          .foregroundColor(Color(.secondaryBraveLabel))
        accountBlockies
      }
      .frame(maxWidth: .infinity, alignment: .leading)
    }
    .padding(.vertical, 6)
  }
  
  @ViewBuilder private var accountBlockies: some View {
    if siteConnection.connectedAddresses.isEmpty {
      EmptyView()
    } else {
      HStack(spacing: -(min(blockieSize, maxBlockieSize) / 2)) {
        let numberOfBlockies = min(maxBlockies, siteConnection.connectedAddresses.count)
        ForEach(0..<numberOfBlockies, id: \.self) { index in
          Blockie(address: siteConnection.connectedAddresses[index])
            .frame(width: min(blockieSize, maxBlockieSize), height: min(blockieSize, maxBlockieSize))
            .overlay(Circle().stroke(Color(.secondaryBraveGroupedBackground), lineWidth: 1))
            .zIndex(Double(numberOfBlockies - index))
        }
        if siteConnection.connectedAddresses.count > maxBlockies {
          Circle()
            .foregroundColor(Color(.braveBlurple))
            .frame(width: min(blockieSize, maxBlockieSize), height: min(blockieSize, maxBlockieSize))
            .overlay(
              HStack(spacing: 1) {
                Circle()
                  .frame(width: blockieDotSize, height: blockieDotSize)
                Circle()
                  .frame(width: blockieDotSize, height: blockieDotSize)
                Circle()
                  .frame(width: blockieDotSize, height: blockieDotSize)
              }
                .foregroundColor(.white)
            )
        }
      }
    }
  }
}

private struct SiteConnectionDetailView: View {
  
  let siteConnection: SiteConnection
  @ObservedObject var siteConnectionStore: ManageSiteConnectionsStore
  
  @Environment(\.presentationMode) @Binding private var presentationMode
  
  @State private var isShowingConfirmAlert = false
  
  var body: some View {
    List {
      Section(header: Text(String.localizedStringWithFormat(Strings.Wallet.manageSiteConnectionsDetailHeader, siteConnection.coin.localizedTitle))) {
        ForEach(siteConnection.connectedAddresses, id: \.self) { address in
          AccountView(address: address, name: siteConnectionStore.accountInfo(for: address)?.name ?? "")
            .osAvailabilityModifiers { content in
              if #available(iOS 15.0, *) {
                content
                  .swipeActions(edge: .trailing) {
                    Button(role: .destructive, action: {
                      withAnimation(.default) {
                        if let url = URL(string: siteConnection.url) {
                          siteConnectionStore.removePermissions(for: siteConnection.coin, from: [address], url: url)
                        }
                      }
                    }) {
                      Label(Strings.Wallet.delete, systemImage: "trash")
                    }
                  }
              } else {
                content
              }
            }
        }
        .onDelete { indexSet in
          let addressesToRemove = indexSet.map({ siteConnection.connectedAddresses[$0] })
          withAnimation(.default) {
            if let url = URL(string: siteConnection.url) {
              siteConnectionStore.removePermissions(for: siteConnection.coin, from: addressesToRemove, url: url)
              if #available(iOS 15, *) {
                // iOS 15 will dismiss itself (and will use `.swipeActions` instead of `.onDelete`)
              } else if siteConnection.connectedAddresses.count == addressesToRemove.count {
                presentationMode.dismiss()
              }
            }
          }
        }
        .listRowBackground(Color(.secondaryBraveGroupedBackground))
      }
    }
    .listStyle(.insetGrouped)
    .navigationTitle(siteConnection.url)
    .navigationBarTitleDisplayMode(.inline)
    .toolbar {
      ToolbarItemGroup(placement: .bottomBar) {
        Spacer()
        Button(action: {
          isShowingConfirmAlert = true
        }) {
          Text(Strings.Wallet.manageSiteConnectionsRemoveAll)
            .foregroundColor(siteConnectionStore.siteConnections.isEmpty ? Color(.braveDisabled) : .red)
        }
      }
    }
    .alert(isPresented: $isShowingConfirmAlert) {
      Alert(
        title: Text(Strings.Wallet.manageSiteConnectionsConfirmAlertTitle),
        message: Text(Strings.Wallet.manageSiteConnectionsDetailConfirmAlertMessage),
        primaryButton: Alert.Button.destructive(
          Text(Strings.Wallet.manageSiteConnectionsConfirmAlertRemove),
          action: {
            siteConnectionStore.removeAllPermissions(from: [siteConnection])
            if #available(iOS 15, *) { // iOS 15 will dismiss itself
            } else {
              presentationMode.dismiss()
            }
          }
        ),
        secondaryButton: Alert.Button.cancel(Text(Strings.CancelString))
      )
    }
  }
}
