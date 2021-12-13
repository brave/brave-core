/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import SwiftUI
import BraveCore
import BraveUI
import struct Shared.Strings
import BigNumber

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
        let isSelected = isPredefinedOptionSelected(option.id)
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
            .background(BuySendSwapGridBackgroundView(backgroundColor: Color(isSelected ? .braveBlurple : .secondaryBraveGroupedBackground)))
            .padding(.top, 8)
        }
        .accessibilityAddTraits(isSelected ? .isSelected : [])
      }
      TextField("%", text: $input)
        .onChange(of: input) { value in
          guard let intValue = Int(value) else {
            customSlippage = nil
            return
          }
          if intValue >= 0, intValue <= 100, customSlippage != intValue {
            customSlippage = intValue
          } else {
            let acceptedValue = min((max(intValue, 0)), 100)
            customSlippage = acceptedValue
            input = String(acceptedValue)
          }
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
        .accessibilityAddTraits(customSlippage != nil ? .isSelected : [])
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
        Text(swapTokenStore.selectedFromTokenPrice)
          .font(.title3.weight(.semibold))
      }
      .accessibilityElement(children: .combine)
      Spacer()
      Button(action: {
        swapTokenStore.fetchPriceQuote(base: .perSellAsset)
      }) {
        Label(Strings.Wallet.refreshMarketPriceLabel, image: "wallet-refresh")
          .labelStyle(.iconOnly)
          .foregroundColor(Color(.braveBlurpleTint))
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
  
  @State private var orderType: OrderType = .market
  @State var hideSlippage = true
  @State private var isSwapDisclaimerVisible: Bool = false
  
  @Environment(\.presentationMode) @Binding private var presentationMode
  @Environment(\.openWalletURLAction) private var openWalletURL
  
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
    if let overrideSlippage = swapTokensStore.overrideSlippage {
      let nf = NumberFormatter()
      nf.numberStyle = .percent
      nf.maximumFractionDigits = 2
      return nf.string(from: NSNumber(value: Double(overrideSlippage) / 100)) ?? ""
    } else {
      return swapTokensStore.slippageOption.localizedString
    }
  }
  
  private var isSwapButtonDisabled: Bool {
    guard !swapTokensStore.isMakingTx else {
      return true
    }
    switch swapTokensStore.state {
    case .error, .idle:
      return true
    case .lowAllowance, .swap:
      return false
    }
  }
  
  private var swapButtonTitle: String {
    switch swapTokensStore.state {
    case .error(let error):
      return error
    case .lowAllowance:
      return String.localizedStringWithFormat(Strings.Wallet.activateToken, swapTokensStore.selectedFromToken?.symbol ?? "")
    case .swap, .idle:
      return Strings.Wallet.swapCryptoSwapButtonTitle
    }
  }
  
  private var isSwapEnabled: Bool {
    let selectedChain = ethNetworkStore.selectedChainId
    return selectedChain == BraveWallet.MainnetChainId || selectedChain == BraveWallet.RopstenChainId
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
          Text(swapTokensStore.selectedFromTokenBalance?.decimalDescription ?? "0.0000")
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
        swapTokensStore.sellAmount = ((swapTokensStore.selectedFromTokenBalance ?? 0) * amount.rawValue)
          .decimalExpansion(precisionAfterDecimalPoint: Int(swapTokensStore.selectedFromToken?.decimals ?? 18))
      })
      .listRowInsets(.zero)
      .padding(.bottom, 8)
    ) {
      TextField(
        String.localizedStringWithFormat(
          Strings.Wallet.amountInCurrency,
          swapTokensStore.selectedFromToken?.symbol ?? ""),
        text: $swapTokensStore.sellAmount
      )
        .keyboardType(.decimalPad)
    }
    .listRowBackground(Color(.secondaryBraveGroupedBackground))
    Section(
      header: WalletListHeaderView(title: Text(Strings.Wallet.swapCryptoToTitle))
    ) {
      NavigationLink(
        destination: SwapTokenSearchView(swapTokenStore: swapTokensStore, searchType: .toToken)
      ) {
        HStack {
          if let token = swapTokensStore.selectedToToken {
            AssetIconView(token: token, length: 26)
          }
          Text(swapTokensStore.selectedToToken?.symbol ?? "")
            .font(.title3.weight(.semibold))
            .foregroundColor(Color(.braveLabel))
          Spacer()
          Text(swapTokensStore.selectedToTokenBalance?.decimalDescription ?? "0.0000")
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
          Strings.Wallet.amountInCurrency,
          swapTokensStore.selectedToToken?.symbol ?? ""),
        text: $swapTokensStore.buyAmount
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
        .padding(.horizontal)
        .padding(.bottom, 15),
      footer: Group {
        if !hideSlippage {
          SlippageGrid(
            selectedSlippage: $swapTokensStore.slippageOption,
            customSlippage: $swapTokensStore.overrideSlippage
          )
            .listRowInsets(.zero)
            .transition(.opacity)
        }
      }
    ) {
      Button(action: {
        withAnimation(.easeInOut(duration: 0.25)) {
          hideSlippage.toggle()
        }
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
      .accessibilityLabel(Strings.Wallet.swapCryptoSlippageTitle)
      .accessibilityValue(formatSlippage)
      .accessibility(addTraits: .isButton)
    }
    .listRowBackground(Color(.secondaryBraveGroupedBackground))
    Section(
      header:
        VStack(spacing: 16) {
          Text(
            String.localizedStringWithFormat(Strings.Wallet.braveSwapFeeDisclaimer, {
              let formatter = NumberFormatter()
              formatter.numberStyle = .percent
              formatter.minimumFractionDigits = 3
              formatter.maximumFractionDigits = 3
              return formatter.string(from: NSNumber(
                value: WalletConstants.braveSwapFee
              )) ?? ""
            }())
          )
            .foregroundColor(Color(.braveLabel))
            .font(.footnote)
          WalletLoadingButton(
            isLoading: swapTokensStore.isMakingTx,
            action: {
              swapTokensStore.prepareSwap()
            },
            label: {
              Text(swapButtonTitle)
            }
          )
            .disabled(isSwapButtonDisabled)
            .buttonStyle(BraveFilledButtonStyle(size: .normal))
        }
        .frame(maxWidth: .infinity)
        .padding(.top, 16)
        .resetListHeaderStyle()
    ) {
      Button(action: {
        isSwapDisclaimerVisible = true
      }) {
        HStack {
          Text(Strings.Wallet.swapDexAggrigatorNote)
            .multilineTextAlignment(.center)
            .foregroundColor(Color(.braveLabel))
          Image(systemName: "info.circle")
            .foregroundColor(Color(.braveBlurpleTint))
            .accessibilityHidden(true)
        }
      }
      .buttonStyle(.plain)
      .padding(.vertical, 12)
      .alert(isPresented: $isSwapDisclaimerVisible) {
        Alert(
          title: Text(Strings.Wallet.swapDexAggrigatorNote),
          message: Text(Strings.Wallet.swapDexAggrigatorDisclaimer),
          primaryButton: Alert.Button.default(Text(Strings.learnMore), action: {
            guard let url = URL(string: "https://0x.org/") else { return }
            openWalletURL?(url)
          }),
          secondaryButton: Alert.Button.cancel(Text(Strings.OKString))
        )
      }
      .frame(maxWidth: .infinity)
      .font(.footnote)
      .listRowBackground(Color(.braveGroupedBackground))
    }
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
            Text(Strings.cancelButtonTitle)
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
      keyringStore: .previewStore,
      ethNetworkStore: .previewStore,
      swapTokensStore: .previewStore
    )
//      .previewSizeCategories([.large, .accessibilityLarge])
  }
}
#endif
