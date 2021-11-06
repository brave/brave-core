/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import SwiftUI
import BraveCore
import BraveUI
import struct Shared.Strings

struct ShortcutAmountGrid: View {
  enum Amount: Double, CaseIterable {
    case quarter = 0.25
    case half = 0.5
    case threeQuarters = 0.75
    case all = 1.0
    
    var label: String {
      let nf = NumberFormatter()
      nf.numberStyle = .percent
      return nf.string(from: NSNumber(value: rawValue)) ?? ""
    }
  }
  
  var action: (Amount) -> Void
  
  @Environment(\.sizeCategory) private var sizeCategory
  
  var body: some View {
    HStack(spacing: 8) {
      ForEach(Amount.allCases, id: \.rawValue) { amount in
        Button(action: { action(amount) }) {
          Text(amount.label)
            .lineLimit(1)
            .minimumScaleFactor(0.75)
            .padding(.vertical, 12)
            .frame(maxWidth: .infinity)
            .foregroundColor(Color(.secondaryBraveLabel))
            .background(BuySendSwapGridBackgroundView())
            .padding(.top, 8)
        }
      }
    }
  }
}

struct SlippageGrid: View {
  @Binding var selectedSlippage: Option
  @Binding var customSlippage: Int?
  
  @State private var input = ""
  
  struct Option: Equatable {
    var value: Double
    
    var id: Double {
      return value
    }
    
    var localizedString: String {
      let nf = NumberFormatter()
      nf.numberStyle = .percent
      nf.maximumFractionDigits = 2
      return nf.string(from: NSNumber(value: value)) ?? ""
    }
    
    static let halfPercent: Option = .init(value: 0.005)
    static let onePercent: Option = .init(value: 0.01)
    static let twoPercent: Option = .init(value: 0.02)
    static let predefinedOptions: [Option] = [
      .halfPercent, .onePercent, .twoPercent
    ]
  }
  
  @Environment(\.sizeCategory) private var sizeCategory
  
  var body: some View {
    HStack(spacing: 8) {
      ForEach(Option.predefinedOptions, id: \.id) { option in
        Button(action: {
          customSlippage = nil
          selectedSlippage = option
          resignFirstResponder()
        }) {
          Text(option.localizedString)
            .lineLimit(1)
            .minimumScaleFactor(0.75)
            .padding(.vertical, 12)
            .frame(maxWidth: .infinity)
            .foregroundColor(Color(isPredefinedOptionSelected(option.id) ? .white : .secondaryBraveLabel))
            .background(BuySendSwapGridBackgroundView(backgroundColor: Color(isPredefinedOptionSelected(option.id) ? .braveBlurple : .secondaryBraveGroupedBackground)))
            .padding(.top, 8)
        }
      }
      TextField("%", text: $input)
        .onChange(of: input) { value in
          guard let intValue = Int(value) else {
            customSlippage = nil
            return
          }
          customSlippage = min((max(intValue, 0)), 100)
        }
        .keyboardType(.numberPad)
        .multilineTextAlignment(.center)
        .lineLimit(1)
        .minimumScaleFactor(0.75)
        .padding(.vertical, 12)
        .frame(maxWidth: .infinity)
        .accentColor(customSlippage != nil ? .white : nil)
        .foregroundColor(Color(customSlippage != nil ? .white : .secondaryBraveLabel))
        .background(BuySendSwapGridBackgroundView(backgroundColor: Color(customSlippage != nil ? .braveBlurple : .secondaryBraveGroupedBackground)))
        .padding(.top, 8)
    }
  }
  
  func resignFirstResponder() {
      UIApplication.shared.sendAction(#selector(UIResponder.resignFirstResponder), to: nil, from: nil, for: nil)
  }
  
  private func isPredefinedOptionSelected(_ id: Double) -> Bool {
    guard customSlippage == nil else { return false }
    
    return id == selectedSlippage.id
  }
}

struct MarketPriceView: View {
  @ObservedObject var swapTokenStore: SwapTokenStore
  
  var body: some View {
    HStack {
      VStack(alignment: .leading) {
        Text(String.localizedStringWithFormat(Strings.Wallet.swapCryptoMarketPriceTitle, swapTokenStore.selectedFromToken?.symbol ?? ""))
          .foregroundColor(Color(.secondaryBraveLabel))
          .font(.subheadline)
        Text("$\(swapTokenStore.selectedFromTokenPrice ?? 0)")
          .font(.title3.weight(.semibold))
      }
      Spacer()
      Button(action: { }) {
        Image("wallet-refresh")
          .foregroundColor(Color(.braveLighterBlurple))
          .font(.title3)
      }
      .buttonStyle(.plain)
    }
  }
}

struct SwapCryptoView: View {
  @ObservedObject var keyringStore: KeyringStore
  @ObservedObject var ethNetworkStore: NetworkStore
  @ObservedObject var swapTokensStore: SwapTokenStore
  
  @State private var fromQuantity: String = ""
  @State private var toQuantity: String = ""
  @State private var orderType: OrderType = .market
  @State var slippage = SlippageGrid.Option.halfPercent
  @State var overrideSlippage: Int?
  @State var hideSlippage = true
  
  @Environment(\.presentationMode) @Binding private var presentationMode
  
  @ViewBuilder var unsupportedSwapChainSection: some View {
    Section {
      VStack(alignment: .leading, spacing: 4.0) {
        Text(Strings.Wallet.swapCryptoUnsupportNetworkTitle)
          .font(.headline)
        Text(String.localizedStringWithFormat(Strings.Wallet.swapCryptoUnsupportNetworkDescription, ethNetworkStore.selectedChain.chainName))
          .font(.subheadline)
          .foregroundColor(Color(.secondaryBraveLabel))
      }
      .padding(.vertical, 6.0)
      .listRowBackground(Color(.secondaryBraveGroupedBackground))
    }
  }
  
  private var formatSlippage: String {
    if let overrideSlippage = overrideSlippage {
      let nf = NumberFormatter()
      nf.numberStyle = .percent
      nf.maximumFractionDigits = 2
      return nf.string(from: NSNumber(value: Double(overrideSlippage) / 100)) ?? ""
    } else {
      return slippage.localizedString
    }
  }
  
  @ViewBuilder var swapFormSections: some View {
    Section(
      header: WalletListHeaderView(title: Text(Strings.Wallet.swapCryptoFromTitle))
    ) {
      NavigationLink(destination: SwapTokenSearchView(swapTokenStore: swapTokensStore, searchType: .fromToken)) {
        HStack {
          if let token = swapTokensStore.selectedFromToken {
            AssetIconView(token: token, length: 26)
          }
          Text(swapTokensStore.selectedFromToken?.symbol ?? "")
            .font(.title3.weight(.semibold))
            .foregroundColor(Color(.braveLabel))
          Spacer()
          Text(String(format: "%.04f", swapTokensStore.selectedFromTokenBalance ?? 0))
            .font(.footnote)
            .foregroundColor(Color(.secondaryBraveLabel))
        }
        .padding(.vertical, 8)
      }
    }
    .listRowBackground(Color(.secondaryBraveGroupedBackground))
    Section(
      header: WalletListHeaderView(
        title: Text(String.localizedStringWithFormat(
          Strings.Wallet.swapCryptoAmountTitle,
          swapTokensStore.selectedFromToken?.symbol ?? ""))
      ),
      footer: ShortcutAmountGrid(action: { amount in
        
      })
      .listRowInsets(.zero)
      .padding(.bottom, 8)
    ) {
      TextField(
        String.localizedStringWithFormat(
          Strings.Wallet.swapCryptoAmountPlaceholder,
          swapTokensStore.selectedFromToken?.symbol ?? ""),
        text: $fromQuantity
      )
        .keyboardType(.decimalPad)
    }
    .listRowBackground(Color(.secondaryBraveGroupedBackground))
    Section(
      header: WalletListHeaderView(title: Text(Strings.Wallet.swapCryptoToTitle))
    ) {
      NavigationLink(destination: SwapTokenSearchView(swapTokenStore: swapTokensStore, searchType: .toToken)) {
        HStack {
          if let token = swapTokensStore.selectedToToken {
            AssetIconView(token: token, length: 26)
          }
          Text(swapTokensStore.selectedToToken?.symbol ?? "")
            .font(.title3.weight(.semibold))
            .foregroundColor(Color(.braveLabel))
          Spacer()
          Text(String(format: "%.04f", swapTokensStore.selectedToTokenBalance ?? 0))
            .font(.footnote)
            .foregroundColor(Color(.secondaryBraveLabel))
        }
        .padding(.vertical, 8)
      }
    }
    .listRowBackground(Color(.secondaryBraveGroupedBackground))
    Section(
      header: WalletListHeaderView(
        title: Text(String.localizedStringWithFormat(
          Strings.Wallet.swapCryptoAmountReceivingTitle,
          swapTokensStore.selectedToToken?.symbol ?? ""))
      )
    ) {
      TextField(
        String.localizedStringWithFormat(
          Strings.Wallet.swapCryptoAmountPlaceholder,
          swapTokensStore.selectedToToken?.symbol ?? ""),
        text: $toQuantity
      )
        .keyboardType(.decimalPad)
    }
    .listRowBackground(Color(.secondaryBraveGroupedBackground))
    Section(
      /*
       MVP only supports market price swap. Ref: https://github.com/brave/brave-browser/issues/18307
       */
      /*header: Picker(Strings.Wallet.swapOrderTypeLabel, selection: $orderType) {
        Text(Strings.Wallet.swapMarketOrderType).tag(OrderType.market)
        Text(Strings.Wallet.swapLimitOrderType).tag(OrderType.limit)
      }
        .pickerStyle(SegmentedPickerStyle())
        .resetListHeaderStyle()
        .padding(.bottom, 15)
        .listRowBackground(Color(.clear))*/
      header: MarketPriceView(swapTokenStore: swapTokensStore)
        .listRowBackground(Color.clear)
        .resetListHeaderStyle()
        .padding(.trailing, 10)
        .padding(.bottom, 15),
      footer: Group {
        if !hideSlippage {
          SlippageGrid(selectedSlippage: $slippage, customSlippage: $overrideSlippage)
            .listRowInsets(.zero)
            .transition(.opacity.animation(.default))
        }
      }
    ) {
      Button(action: {
        hideSlippage.toggle()
      }) {
        HStack {
          Text(Strings.Wallet.swapCryptoSlippageTitle)
            .font(.subheadline)
          Spacer()
          Text(formatSlippage)
            .foregroundColor(Color(.secondaryBraveLabel))
            .font(.subheadline.weight(.semibold))
          Image("wallet-dismiss")
            .renderingMode(.template)
            .resizable()
            .foregroundColor(Color(.secondaryBraveLabel))
            .frame(width: 12, height: 6)
            .rotationEffect(.degrees(hideSlippage ? 0 : 180))
            .animation(.default, value: hideSlippage)
        }
        .contentShape(Rectangle())
      }
      .buttonStyle(.plain)
    }
    .listRowBackground(Color(.secondaryBraveGroupedBackground))
    Section(
      header:
        Button(action: {}) {
          Text(Strings.Wallet.swapCryptoSwapButtonTitle)
        }
        .buttonStyle(BraveFilledButtonStyle(size: .normal))
        .frame(maxWidth: .infinity)
        .resetListHeaderStyle()
        .padding(.top, 20)
    ) {
    }
  }
  
  private var isSwapEnabled: Bool {
    let selectedChain = ethNetworkStore.selectedChainId
    return selectedChain == BraveWallet.MainnetChainId || selectedChain == BraveWallet.RopstenChainId
  }
  
  enum OrderType {
    case market
    case limit
  }
  
  var body: some View {
    NavigationView {
      Form {
        Section(
          header: AccountPicker(
            keyringStore: keyringStore,
            networkStore: ethNetworkStore
          )
            .listRowBackground(Color.clear)
            .resetListHeaderStyle()
            .padding(.top)
            .padding(.bottom, -16) // Get it a bit closer
        ) {
        }
        if isSwapEnabled {
          swapFormSections
        } else {
          unsupportedSwapChainSection
        }
      }
      .navigationTitle(Strings.Wallet.swap)
      .navigationBarTitleDisplayMode(.inline)
      .toolbar {
        ToolbarItemGroup(placement: .cancellationAction) {
          Button(action: {
            presentationMode.dismiss()
          }) {
            Text(Strings.CancelString)
              .foregroundColor(Color(.braveOrange))
          }
        }
      }
      .onAppear {
        swapTokensStore.prepare(with: keyringStore.selectedAccount)
      }
    }
  }
}

#if DEBUG
struct SwapCryptoView_Previews: PreviewProvider {
  static var previews: some View {
    SwapCryptoView(
      keyringStore: .previewStoreWithWalletCreated,
      ethNetworkStore: .previewStore,
      swapTokensStore: .previewStore
    )
      .previewSizeCategories([.large, .accessibilityLarge])
  }
}
#endif
