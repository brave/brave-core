// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveUI
import Shared
import Strings
import SwiftUI

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

class NetworkModel: ObservableObject, Identifiable {
  enum Mode {
    case add
    case edit(_ network: BraveWallet.NetworkInfo)
    case view(_ network: BraveWallet.NetworkInfo)

    var isEditMode: Bool {
      switch self {
      case .add, .view(_):
        return false
      case .edit(_):
        return true
      }
    }
    var isViewMode: Bool {
      switch self {
      case .add, .edit(_):
        return false
      case .view(_):
        return true
      }
    }
  }

  let mode: Mode
  var id: String {
    "\(mode.isEditMode)\(mode.isViewMode)"
  }

  @Published var networkId = NetworkInputItem(input: "") {
    didSet {
      if networkId.input != oldValue.input {
        if networkId.input.isEmpty {
          networkId.error = Strings.Wallet.customNetworkChainIdErrMsg
        } else {
          networkId.error = nil
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
            if let url = URIFixup.getURL(item.input) {
              if url.isSecureWebPage() {
                rpcUrls[index].error = nil
              } else {
                rpcUrls[index].error = Strings.Wallet.customNetworkNotSecureErrMsg
              }
            } else {
              rpcUrls[index].error = Strings.Wallet.customNetworkInvalidAddressErrMsg
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
            if let url = URIFixup.getURL(item.input) {
              if url.isSecureWebPage() {
                iconUrls[index].error = nil
              } else {
                iconUrls[index].error = Strings.Wallet.customNetworkNotSecureErrMsg
              }
            } else {
              iconUrls[index].error = Strings.Wallet.customNetworkInvalidAddressErrMsg
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
            if let url = URIFixup.getURL(item.input) {
              if url.isSecureWebPage() {
                blockUrls[index].error = nil
              } else {
                blockUrls[index].error = Strings.Wallet.customNetworkNotSecureErrMsg
              }
            } else {
              blockUrls[index].error = Strings.Wallet.customNetworkInvalidAddressErrMsg
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

  /// Creates model and populates the details based on a custom network and mode
  init(mode: Mode = .add) {
    self.mode = mode

    var network: BraveWallet.NetworkInfo?
    switch mode {
    case .add:
      break
    case .edit(let editNetwork):
      network = editNetwork
    case .view(let viewNetwork):
      network = viewNetwork
    }

    if let network {
      let chainIdInDecimal: String
      if network.coin == .eth,
        let intValue = Int(network.chainId.removingHexPrefix, radix: 16)
      {
        // Eth and EVM BraveWallet.NetworkInfo.chainId should always in hex
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

struct NetworkDetailsView: View {
  @ObservedObject var networkStore: NetworkStore
  @ObservedObject var model: NetworkModel

  @Environment(\.presentationMode) @Binding private var presentationMode

  @State private var customNetworkError: CustomNetworkError?
  @State private var isPresentingEditConfirmation: Bool = false

  init(
    networkStore: NetworkStore,
    model: NetworkModel
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

  private var isChainIdInputDisabled: Bool {
    switch model.mode {
    case .add:
      return false
    case .edit(let editNetwork):
      return editNetwork.coin != .eth
    case .view(_):
      return true
    }
  }

  var body: some View {
    Form {
      Section(
        header: WalletListHeaderView(title: Text(Strings.Wallet.customNetworkChainIdTitle))
      ) {
        networkTextField(
          placeholder: Strings.Wallet.customNetworkChainIdPlaceholder,
          item: $model.networkId,
          isDisabled: isChainIdInputDisabled
        )
        .keyboardType(.numberPad)
        .listRowBackground(Color(.secondaryBraveGroupedBackground))
      }
      Section(
        header: WalletListHeaderView(title: Text(Strings.Wallet.customNetworkChainNameTitle))
      ) {
        networkTextField(
          placeholder: Strings.Wallet.customNetworkChainNamePlaceholder,
          item: $model.networkName,
          isDisabled: model.mode.isViewMode
        )
        .listRowBackground(Color(.secondaryBraveGroupedBackground))
      }
      Section(
        header: WalletListHeaderView(title: Text(Strings.Wallet.customNetworkSymbolNameTitle))
      ) {
        networkTextField(
          placeholder: Strings.Wallet.customNetworkSymbolNamePlaceholder,
          item: $model.networkSymbolName,
          isDisabled: model.mode.isViewMode
        )
        .listRowBackground(Color(.secondaryBraveGroupedBackground))
      }
      Section(
        header: WalletListHeaderView(title: Text(Strings.Wallet.customNetworkSymbolTitle))
      ) {
        networkTextField(
          placeholder: Strings.Wallet.customNetworkSymbolPlaceholder,
          item: $model.networkSymbol,
          isDisabled: model.mode.isViewMode
        )
        .listRowBackground(Color(.secondaryBraveGroupedBackground))
      }
      Section(
        header: WalletListHeaderView(title: Text(Strings.Wallet.customNetworkCurrencyDecimalTitle))
      ) {
        networkTextField(
          placeholder: Strings.Wallet.customNetworkCurrencyDecimalPlaceholder,
          item: $model.networkDecimals,
          isDisabled: model.mode.isViewMode
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
              showRadioButton: true,
              isDisabled: model.mode.isViewMode
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
              item: $url,
              isDisabled: model.mode.isViewMode
            )
          }
          .listRowBackground(Color(.secondaryBraveGroupedBackground))
        }
      }
      if !model.blockUrls.isEmpty {
        Section(
          header: WalletListHeaderView(
            title: Text(Strings.Wallet.customNetworkBlockExplorerUrlsTitle)
          )
        ) {
          ForEach($model.blockUrls) { $url in
            networkTextField(
              placeholder: Strings.Wallet.customNetworkUrlsPlaceholder,
              item: $url,
              isDisabled: model.mode.isViewMode
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
        if !model.mode.isViewMode {  // don't show confirmation action in view mode
          if networkStore.isAddingNewNetwork {
            ProgressView()
          } else {
            Button {
              if model.mode.isEditMode {
                if validateAllFields() {
                  isPresentingEditConfirmation = true
                }
              } else {
                if validateAllFields() {
                  addCustomNetwork()
                }
              }
            } label: {
              Text(Strings.Wallet.saveButtonTitle)
                .foregroundColor(Color(.braveBlurpleTint))
            }
          }
        }
      }
      ToolbarItemGroup(placement: .cancellationAction) {
        Button {
          presentationMode.dismiss()
        } label: {
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
          }
        )
    )
    .background(
      Color.clear
        .alert(
          isPresented: $isPresentingEditConfirmation,
          content: {
            Alert(
              title: Text(Strings.Wallet.editExistingNetworkAlertMsg),
              primaryButton: .default(
                Text(Strings.OKString),
                action: {
                  addCustomNetwork()
                }
              ),
              secondaryButton: .cancel()
            )
          }
        )
    )
  }

  @ViewBuilder private func networkTextField(
    placeholder: String,
    item: Binding<NetworkInputItem>,
    showRadioButton: Bool = false,
    isDisabled: Bool
  ) -> some View {
    HStack {
      if showRadioButton {
        NetworkRadioButton(
          checked: item.isSelected,
          isDisabled: Binding(
            get: { item.input.wrappedValue.isEmpty || model.mode.isViewMode },
            set: { _, _ in }
          ),
          onTapped: {
            for index in model.rpcUrls.indices where model.rpcUrls[index].id != item.id {
              model.rpcUrls[index].isSelected = !item.isSelected.wrappedValue
            }
          }
        )
      }
      if isDisabled {
        Text(item.wrappedValue.input)
          .foregroundColor(Color(.braveLabel))
          .contextMenu {
            Button {
              UIPasteboard.general.string = item.wrappedValue.input
            } label: {
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
    model.networkName.error =
      model.networkName.input.isEmpty ? Strings.Wallet.customNetworkEmptyErrMsg : nil
    model.networkSymbolName.error =
      model.networkSymbolName.input.isEmpty ? Strings.Wallet.customNetworkEmptyErrMsg : nil
    model.networkSymbol.error =
      model.networkSymbol.input.isEmpty ? Strings.Wallet.customNetworkEmptyErrMsg : nil
    if model.networkDecimals.input.isEmpty {
      model.networkDecimals.error = Strings.Wallet.customNetworkEmptyErrMsg
    }
    // has no valid url
    if model.rpcUrls.first(where: { !$0.input.isEmpty && $0.error == nil }) == nil {
      // find the first empty entry
      if let index = model.rpcUrls.firstIndex(where: { $0.input.isEmpty }) {
        // set the empty err msg
        model.rpcUrls[index].error = Strings.Wallet.customNetworkEmptyErrMsg
      }
    }

    let rpcUrlsAllGood = model.rpcUrls.allSatisfy({
      if $0.input.isEmpty && $0.error != Strings.Wallet.customNetworkEmptyErrMsg {
        return true
      } else {
        return !$0.input.isEmpty && $0.error == nil
      }
    })
    let iconUrlsAllGood = model.iconUrls.allSatisfy({
      if $0.input.isEmpty && $0.error != Strings.Wallet.customNetworkEmptyErrMsg {
        return true
      } else {
        return !$0.input.isEmpty && $0.error == nil
      }
    })
    let blockUrlsAllGood = model.blockUrls.allSatisfy({
      if $0.input.isEmpty && $0.error != Strings.Wallet.customNetworkEmptyErrMsg {
        return true
      } else {
        return !$0.input.isEmpty && $0.error == nil
      }
    })

    if model.networkId.error != nil
      || model.networkName.error != nil
      || model.networkSymbolName.error != nil
      || model.networkSymbol.error != nil
      || model.networkDecimals.error != nil
      || !rpcUrlsAllGood
      || !iconUrlsAllGood
      || !blockUrlsAllGood
    {
      return false
    }

    return true
  }

  private func addCustomNetwork() {
    var chainIdInHex = model.networkId.input
    if model.networkId.input.hasPrefix("0x") || model.networkId.input.hasPrefix("0X") {
      let hexDecimalString = model.networkId.input.removingHexPrefix
      if let decimalString = UInt8(hexDecimalString, radix: 16) {
        chainIdInHex = "0x\(String(format: "%x", decimalString))"
      }
    } else {
      if let idValue = Int(model.networkId.input) {
        chainIdInHex = "0x\(String(format: "%x", idValue))"
      }
    }
    // Check if input chain id already existed for non-edit mode
    if !model.mode.isEditMode,
      networkStore.allChains.contains(where: { $0.id == chainIdInHex })
    {
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
    var network: BraveWallet.NetworkInfo?
    switch model.mode {
    case .add:
      network = .init(
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
        supportedKeyrings: [BraveWallet.KeyringId.default.rawValue].map(NSNumber.init(value:))
      )
    case .edit(let editNetwork):
      network = .init(
        chainId: editNetwork.coin == .eth ? chainIdInHex : editNetwork.chainId,
        chainName: model.networkName.input,
        blockExplorerUrls: blockExplorerUrls,
        iconUrls: iconUrls,
        activeRpcEndpointIndex: Int32(activeRpcEndpointIndex),
        rpcEndpoints: rpcEndpoints,
        symbol: model.networkSymbol.input,
        symbolName: model.networkSymbol.input,
        decimals: Int32(model.networkDecimals.input) ?? editNetwork.decimals,
        coin: editNetwork.coin,
        supportedKeyrings: editNetwork.supportedKeyrings
      )
    case .view(_):
      break
    }
    Task { @MainActor in
      guard let network else { return }
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
struct NetworkDetailsView_Previews: PreviewProvider {
  static var previews: some View {
    NavigationView {
      NetworkDetailsView(
        networkStore: .previewStore,
        model: .init()
      )
    }
  }
}
#endif
