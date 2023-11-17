// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import SwiftUI
import BraveCore
import Shared
import Strings
import BraveUI

struct NetworkInputItem: Identifiable {
  var input: String
  var isSelected: Bool = false
  var error: String?
  var id = UUID()
}

struct NetworkTextField: View {
  var placeholder: String
  @Binding var item: NetworkInputItem

  var body: some View {
    VStack(alignment: .leading) {
      TextField(placeholder, text: $item.input)
        .autocapitalization(.none)
        .disableAutocorrection(true)
        .foregroundColor(Color(.braveLabel))
      if let error = item.error {
        HStack(alignment: .firstTextBaseline, spacing: 4) {
          Image(systemName: "exclamationmark.circle.fill")
          Text(error)
            .fixedSize(horizontal: false, vertical: true)
            .animation(nil, value: error)
        }
        .accessibilityElement(children: .combine)
        .transition(
          .asymmetric(
            insertion: .opacity.animation(.default),
            removal: .identity
          )
        )
        .font(.footnote)
        .foregroundColor(Color(.braveErrorLabel))
      }
    }
  }
}

class CustomNetworkModel: ObservableObject, Identifiable {
  enum Mode {
    case add
    case edit
    case view
    
    var isEditMode: Bool { self == .edit }
    var isViewMode: Bool { self == .view }
  }
  
  var mode: Mode = .add
  var id: String {
    "\(mode.isEditMode)"
  }

  @Published var networkId = NetworkInputItem(input: "") {
    didSet {
      if networkId.input != oldValue.input {
        if let intValue = Int(networkId.input), intValue > 0 {
          networkId.error = nil
        } else {
          networkId.error = Strings.Wallet.customNetworkChainIdErrMsg
        }
      }
    }
  }
  @Published var networkName = NetworkInputItem(input: "") {
    didSet {
      if networkName.input != oldValue.input {
        if networkName.input.isEmpty {
          networkName.error = Strings.Wallet.customNetworkEmptyErrMsg
        } else {
          networkName.error = nil
        }
      }
    }
  }
  @Published var networkSymbolName = NetworkInputItem(input: "") {
    didSet {
      if networkSymbolName.input != oldValue.input {
        if networkSymbolName.input.isEmpty {
          networkSymbolName.error = Strings.Wallet.customNetworkEmptyErrMsg
        } else {
          networkSymbolName.error = nil
        }
      }
    }
  }
  @Published var networkSymbol = NetworkInputItem(input: "") {
    didSet {
      if networkSymbol.input != oldValue.input {
        if networkSymbol.input.isEmpty {
          networkSymbol.error = Strings.Wallet.customNetworkEmptyErrMsg
        } else {
          networkSymbol.error = nil
        }
      }
    }
  }
  @Published var networkDecimals = NetworkInputItem(input: "") {
    didSet {
      if networkDecimals.input != oldValue.input {
        if networkDecimals.input.isEmpty {
          networkDecimals.error = Strings.Wallet.customNetworkEmptyErrMsg
        } else if let intValue = Int(networkDecimals.input), intValue > 0 {
          networkDecimals.error = nil
        } else {
          networkDecimals.error = Strings.Wallet.customNetworkCurrencyDecimalErrMsg
        }
      }
    }
  }

  @Published var rpcUrls: [NetworkInputItem] = [NetworkInputItem(input: "", isSelected: true)] {
    didSet {
      // we only care the set on each item's `input`
      if rpcUrls.reduce("", { $0 + $1.input }) != oldValue.reduce("", { $0 + $1.input }) {
        // validate every entry except the last new entry if there is one
        var hasNewEntry = false
        for (index, item) in rpcUrls.enumerated() {
          if item.input.isEmpty && item.error == nil {  // no validation on new entry
            hasNewEntry = true
          } else {
            if URIFixup.getURL(item.input) == nil {
              rpcUrls[index].error = Strings.Wallet.customNetworkInvalidAddressErrMsg
            } else {
              rpcUrls[index].error = nil
            }
          }
        }
        // Only insert a new entry when all existed entries pass validation and we are not in view mode
        if rpcUrls.compactMap({ $0.error }).isEmpty && !hasNewEntry && !mode.isViewMode {
          rpcUrls.append(NetworkInputItem(input: ""))
        }
      }
    }
  }
  @Published var iconUrls = [NetworkInputItem(input: "")] {
    didSet {
      // we only care the set on each item's `input`
      if iconUrls.reduce("", { $0 + $1.input }) != oldValue.reduce("", { $0 + $1.input }) {
        // validate every entry except the last new entry if there is one
        var hasNewEntry = false
        for (index, item) in iconUrls.enumerated() {
          if item.input.isEmpty && item.error == nil {  // no validation on new entry
            hasNewEntry = true
          } else {
            if URIFixup.getURL(item.input) == nil {
              iconUrls[index].error = Strings.Wallet.customNetworkInvalidAddressErrMsg
            } else {
              iconUrls[index].error = nil
            }
          }
        }
        // Only insert a new entry when all existed entries pass validation and there is no new entry and we are not in view mode
        if iconUrls.compactMap({ $0.error }).isEmpty && !hasNewEntry && !mode.isViewMode {
          iconUrls.append(NetworkInputItem(input: ""))
        }
      }
    }
  }
  @Published var blockUrls = [NetworkInputItem(input: "")] {
    didSet {
      // we only care the set on each item's `input`
      if blockUrls.reduce("", { $0 + $1.input }) != oldValue.reduce("", { $0 + $1.input }) {
        // validate every entry except the last new entry if there is one
        var hasNewEntry = false
        for (index, item) in blockUrls.enumerated() {
          if item.input.isEmpty && item.error == nil {  // no validation on new entry
            hasNewEntry = true
          } else {
            if URIFixup.getURL(item.input) == nil {
              blockUrls[index].error = Strings.Wallet.customNetworkInvalidAddressErrMsg
            } else {
              blockUrls[index].error = nil
            }
          }
        }
        // Only insert a new entry when all existed entries pass validation and there is no new entry and we are not in view mode
        if blockUrls.compactMap({ $0.error }).isEmpty && !hasNewEntry && !mode.isViewMode {
          blockUrls.append(NetworkInputItem(input: ""))
        }
      }
    }
  }
  
  /// Creates model for adding a new custom network
  init() {
    self.mode = .add
  }

  /// Creates model and populates the details based on a custom network and mode
  init(from network: BraveWallet.NetworkInfo, mode: Mode = .edit) {
    self.mode = mode

    let chainIdInDecimal: String
    if let intValue = Int(network.chainId.removingHexPrefix, radix: 16) { // BraveWallet.NetworkInfo.chainId should always in hex
      chainIdInDecimal = "\(intValue)"
    } else {
      chainIdInDecimal = network.chainId
    }
    self.networkId.input = chainIdInDecimal
    self.networkName.input = network.chainName
    self.networkSymbolName.input = network.symbolName
    self.networkSymbol.input = network.symbol
    self.networkDecimals.input = String(network.decimals)
    if !network.rpcEndpoints.isEmpty {
      var result: [NetworkInputItem] = []
      for (index, endpoint) in network.rpcEndpoints.enumerated() {
        result.append(
          NetworkInputItem(
            input: endpoint.absoluteString,
            isSelected: index == network.activeRpcEndpointIndex
          )
        )
      }
      self.rpcUrls = result
    } else if mode.isViewMode {
      self.rpcUrls = []
    }
    if !network.iconUrls.isEmpty {
      self.iconUrls = network.iconUrls.compactMap({ NetworkInputItem(input: $0) })
    } else if mode.isViewMode {
      self.iconUrls = []
    }
    if !network.blockExplorerUrls.isEmpty {
      self.blockUrls = network.blockExplorerUrls.compactMap({ NetworkInputItem(input: $0) })
    } else if mode.isViewMode {
      self.blockUrls = []
    }
  }
}

enum CustomNetworkError: LocalizedError, Identifiable {
  case failed(errorMessage: String)
  case duplicateId

  var id: String {
    errorTitle + errorDescription
  }

  var errorTitle: String {
    switch self {
    case .failed:
      return Strings.Wallet.failedToAddCustomNetworkErrorTitle
    case .duplicateId:
      return ""
    }
  }

  var errorDescription: String {
    switch self {
    case .failed(let errorMessage):
      return errorMessage
    case .duplicateId:
      return Strings.Wallet.networkIdDuplicationErrMsg
    }
  }
}

struct CustomNetworkDetailsView: View {
  @ObservedObject var networkStore: NetworkStore
  @ObservedObject var model: CustomNetworkModel

  @Environment(\.presentationMode) @Binding private var presentationMode

  @State private var customNetworkError: CustomNetworkError?

  init(
    networkStore: NetworkStore,
    model: CustomNetworkModel
  ) {
    self.networkStore = networkStore
    self.model = model
  }
  
  private var navigationTitle: String {
    switch model.mode {
    case .add: return Strings.Wallet.customNetworkDetailsTitle
    case .edit: return Strings.Wallet.editfCustomNetworkTitle
    case .view: return Strings.Wallet.viewNetworkDetailsTitle
    }
  }

  var body: some View {
    Form {
      Section(
        header: WalletListHeaderView(title: Text(Strings.Wallet.customNetworkChainIdTitle))
      ) {
        networkTextField(
          placeholder: Strings.Wallet.customNetworkChainIdPlaceholder,
          item: $model.networkId
        )
        .keyboardType(.numberPad)
        .disabled(model.mode.isEditMode)
        .listRowBackground(Color(.secondaryBraveGroupedBackground))
      }
      Section(
        header: WalletListHeaderView(title: Text(Strings.Wallet.customNetworkChainNameTitle))
      ) {
        networkTextField(
          placeholder: Strings.Wallet.customNetworkChainNamePlaceholder,
          item: $model.networkName
        )
        .listRowBackground(Color(.secondaryBraveGroupedBackground))
      }
      Section(
        header: WalletListHeaderView(title: Text(Strings.Wallet.customNetworkSymbolNameTitle))
      ) {
        networkTextField(
          placeholder: Strings.Wallet.customNetworkSymbolNamePlaceholder,
          item: $model.networkSymbolName
        )
        .listRowBackground(Color(.secondaryBraveGroupedBackground))
      }
      Section(
        header: WalletListHeaderView(title: Text(Strings.Wallet.customNetworkSymbolTitle))
      ) {
        networkTextField(
          placeholder: Strings.Wallet.customNetworkSymbolPlaceholder,
          item: $model.networkSymbol
        )
        .listRowBackground(Color(.secondaryBraveGroupedBackground))
      }
      Section(
        header: WalletListHeaderView(title: Text(Strings.Wallet.customNetworkCurrencyDecimalTitle))
      ) {
        networkTextField(
          placeholder: Strings.Wallet.customNetworkCurrencyDecimalPlaceholder,
          item: $model.networkDecimals
        )
        .keyboardType(.numberPad)
        .listRowBackground(Color(.secondaryBraveGroupedBackground))
      }
      if !model.rpcUrls.isEmpty {
        Section(
          header: WalletListHeaderView(title: Text(Strings.Wallet.customNetworkRpcUrlsTitle))
        ) {
          ForEach($model.rpcUrls) { $url in
            networkTextField(
              placeholder: Strings.Wallet.customNetworkUrlsPlaceholder,
              item: $url,
              showRadioButton: true
            )
          }
          .listRowBackground(Color(.secondaryBraveGroupedBackground))
        }
      }
      if !model.iconUrls.isEmpty {
        Section(
          header: WalletListHeaderView(title: Text(Strings.Wallet.customNetworkIconUrlsTitle))
        ) {
          ForEach($model.iconUrls) { $url in
            networkTextField(
              placeholder: Strings.Wallet.customNetworkUrlsPlaceholder,
              item: $url
            )
          }
          .listRowBackground(Color(.secondaryBraveGroupedBackground))
        }
      }
      if !model.blockUrls.isEmpty {
        Section(
          header: WalletListHeaderView(title: Text(Strings.Wallet.customNetworkBlockExplorerUrlsTitle))
        ) {
          ForEach($model.blockUrls) { $url in
            networkTextField(
              placeholder: Strings.Wallet.customNetworkUrlsPlaceholder,
              item: $url
            )
          }
          .listRowBackground(Color(.secondaryBraveGroupedBackground))
        }
      }
    }
    .listBackgroundColor(Color(UIColor.braveGroupedBackground))
    .navigationBarTitle(navigationTitle)
    .navigationBarTitleDisplayMode(.inline)
    .toolbar {
      ToolbarItemGroup(placement: .confirmationAction) {
        if !model.mode.isViewMode { // don't show confirmation action in view mode
          if networkStore.isAddingNewNetwork {
            ProgressView()
          } else {
            Button(action: {
              addCustomNetwork()
            }) {
              Text(Strings.Wallet.saveButtonTitle)
                .foregroundColor(Color(.braveBlurpleTint))
            }
          }
        }
      }
      ToolbarItemGroup(placement: .cancellationAction) {
        Button(action: {
          presentationMode.dismiss()
        }) {
          Text(Strings.cancelButtonTitle)
            .foregroundColor(Color(.braveBlurpleTint))
        }
      }
    }
    .background(
      Color.clear
        .alert(
          item: $customNetworkError,
          content: { error in
            Alert(
              title: Text(error.errorTitle),
              message: Text(error.errorDescription),
              dismissButton: .default(Text(Strings.OKString))
            )
          })
    )
  }
  
  @ViewBuilder private func networkTextField(placeholder: String, item: Binding<NetworkInputItem>, showRadioButton: Bool = false) -> some View {
    HStack {
      if showRadioButton {
        NetworkRadioButton(
          checked: item.isSelected,
          isDisabled: Binding(get: { item.input.wrappedValue.isEmpty || model.mode.isViewMode }, set: { _, _ in }),
          onTapped: {
            for index in model.rpcUrls.indices where model.rpcUrls[index].id != item.id {
              model.rpcUrls[index].isSelected = !item.isSelected.wrappedValue
            }
          })
      }
      if model.mode.isViewMode {
        Text(item.wrappedValue.input)
          .contextMenu {
            Button(action: { UIPasteboard.general.string = item.wrappedValue.input }) {
              Label(Strings.Wallet.copyToPasteboard, braveSystemImage: "leo.copy.plain-text")
            }
          }
      } else {
        NetworkTextField(
          placeholder: placeholder,
          item: item
        )
      }
    }
  }

  private func validateAllFields() -> Bool {
    if model.networkId.input.isEmpty {
      model.networkId.error = Strings.Wallet.customNetworkEmptyErrMsg
    }
    model.networkName.error = model.networkName.input.isEmpty ? Strings.Wallet.customNetworkEmptyErrMsg : nil
    model.networkSymbolName.error = model.networkSymbolName.input.isEmpty ? Strings.Wallet.customNetworkEmptyErrMsg : nil
    model.networkSymbol.error = model.networkSymbol.input.isEmpty ? Strings.Wallet.customNetworkEmptyErrMsg : nil
    if model.networkDecimals.input.isEmpty {
      model.networkDecimals.error = Strings.Wallet.customNetworkEmptyErrMsg
    }
    if model.rpcUrls.first(where: { !$0.input.isEmpty && $0.error == nil }) == nil {  // has no valid url
      if let index = model.rpcUrls.firstIndex(where: { $0.input.isEmpty }) {  // find the first empty entry
        model.rpcUrls[index].error = Strings.Wallet.customNetworkEmptyErrMsg  // set the empty err msg
      }
    }

    if model.networkId.error != nil
      || model.networkName.error != nil
      || model.networkSymbolName.error != nil
      || model.networkSymbol.error != nil
      || model.networkDecimals.error != nil
      || model.rpcUrls.filter({ !$0.input.isEmpty && $0.error == nil }).isEmpty {
      return false
    }

    return true
  }

  private func addCustomNetwork() {
    guard validateAllFields() else { return }

    var chainIdInHex = ""
    if let idValue = Int(model.networkId.input) {
      chainIdInHex = "0x\(String(format: "%02x", idValue))"
    }
    // Check if input chain id already existed for non-edit mode
    if !model.mode.isEditMode,
      networkStore.allChains.contains(where: { $0.id == chainIdInHex }) {
      customNetworkError = .duplicateId
      return
    }

    let blockExplorerUrls: [String] = model.blockUrls.compactMap({
      if !$0.input.isEmpty && $0.error == nil {
        return $0.input
      } else {
        return nil
      }
    })
    let iconUrls: [String] = model.iconUrls.compactMap({
      if !$0.input.isEmpty && $0.error == nil {
        return $0.input
      } else {
        return nil
      }
    })
    let rpcEndpoints: [URL] = model.rpcUrls.compactMap({
      if !$0.input.isEmpty && $0.error == nil {
        return URL(string: $0.input)
      } else {
        return nil
      }
    })
    let activeRpcEndpointIndex = model.rpcUrls.firstIndex(where: { $0.isSelected }) ?? 0
    let network: BraveWallet.NetworkInfo = .init(
      chainId: chainIdInHex,
      chainName: model.networkName.input,
      blockExplorerUrls: blockExplorerUrls,
      iconUrls: iconUrls,
      activeRpcEndpointIndex: Int32(activeRpcEndpointIndex),
      rpcEndpoints: rpcEndpoints,
      symbol: model.networkSymbol.input,
      symbolName: model.networkSymbol.input,
      decimals: Int32(model.networkDecimals.input) ?? 18,
      coin: .eth,
      supportedKeyrings: [BraveWallet.KeyringId.default.rawValue].map(NSNumber.init(value:)),
      isEip1559: false
    )
    Task { @MainActor in
      let (accepted, errMsg) = await networkStore.addCustomNetwork(network)
      guard accepted else {
        customNetworkError = .failed(errorMessage: errMsg)
        return
      }
      presentationMode.dismiss()
    }
  }
}

struct NetworkRadioButton: View {
  @Binding var checked: Bool
  @Binding var isDisabled: Bool
  var onTapped: () -> Void
  
  var body: some View {
    Image(braveSystemName: checked ? "leo.check.circle-outline" : "leo.radio.unchecked")
      .renderingMode(.template)
      .foregroundColor(Color((checked && !isDisabled) ? .braveBlurpleTint : .braveDisabled))
      .font(.title3)
      .onTapGesture {
        if !self.isDisabled && !checked {
          self.checked = true
          self.onTapped()
        }
      }
  }
}

#if DEBUG
struct CustomNetworkDetailsView_Previews: PreviewProvider {
  static var previews: some View {
    NavigationView {
      CustomNetworkDetailsView(
        networkStore: .previewStore,
        model: .init()
      )
    }
  }
}
#endif
