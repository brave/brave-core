// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BigNumber
import BraveCore
import DesignSystem
import Strings
import SwiftUI

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
        Button {
          action(amount)
        } label: {
          Text(amount.label)
            .lineLimit(1)
            .minimumScaleFactor(0.75)
            .padding(.vertical, 12)
            .frame(maxWidth: .infinity)
            .foregroundColor(Color(.secondaryBraveLabel))
            .background(WalletActionsGridBackgroundView())
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
      .halfPercent, .onePercent, .twoPercent,
    ]
  }

  @Environment(\.sizeCategory) private var sizeCategory

  var body: some View {
    HStack(spacing: 8) {
      ForEach(Option.predefinedOptions, id: \.id) { option in
        let isSelected = isPredefinedOptionSelected(option.id)
        Button {
          customSlippage = nil
          selectedSlippage = option
          resignFirstResponder()
        } label: {
          Text(option.localizedString)
            .lineLimit(1)
            .minimumScaleFactor(0.75)
            .padding(.vertical, 12)
            .frame(maxWidth: .infinity)
            .foregroundColor(
              Color(isPredefinedOptionSelected(option.id) ? .white : .secondaryBraveLabel)
            )
            .background(
              WalletActionsGridBackgroundView(
                backgroundColor: Color(
                  isSelected ? .braveBlurpleTint : .secondaryBraveGroupedBackground
                )
              )
            )
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
        .background(
          WalletActionsGridBackgroundView(
            backgroundColor: Color(
              customSlippage != nil ? .braveBlurpleTint : .secondaryBraveGroupedBackground
            )
          )
        )
        .padding(.top, 8)
        .accessibilityAddTraits(customSlippage != nil ? .isSelected : [])
    }
  }

  func resignFirstResponder() {
    UIApplication.shared.sendAction(
      #selector(UIResponder.resignFirstResponder),
      to: nil,
      from: nil,
      for: nil
    )
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
        Text(
          String.localizedStringWithFormat(
            Strings.Wallet.swapCryptoMarketPriceTitle,
            swapTokenStore.selectedFromToken?.symbol ?? ""
          )
        )
        .foregroundColor(Color(.secondaryBraveLabel))
        .font(.subheadline)
        Text(swapTokenStore.selectedFromTokenPrice)
          .font(.title3.weight(.semibold))
      }
      .accessibilityElement(children: .combine)
      Spacer()
      Button {
        swapTokenStore.fetchPriceQuote(base: .perSellAsset)
      } label: {
        Label(Strings.Wallet.refreshMarketPriceLabel, braveSystemImage: "leo.refresh")
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
  @ObservedObject var networkStore: NetworkStore
  @ObservedObject var swapTokensStore: SwapTokenStore

  @State private var orderType: OrderType = .market
  @State var hideSlippage = true
  @State private var isSwapDisclaimerVisible: Bool = false

  @Environment(\.openURL) private var openWalletURL
  @Environment(\.appRatingRequestAction) private var appRatingRequest

  var completion: ((_ success: Bool) -> Void)?
  var onDismiss: () -> Void

  enum DEXAggregator {
    case zeroX
    case jupiter

    var displayName: String {
      switch self {
      case .zeroX: return "0x"
      case .jupiter: return "Jupiter"
      }
    }

    var url: URL {
      switch self {
      case .zeroX: return WalletConstants.zeroXPrivacyPolicy
      case .jupiter: return WalletConstants.jupiterPrivacyPolicy
      }
    }

    var swapDexAggrigatorNote: String {
      String.localizedStringWithFormat(Strings.Wallet.swapDexAggrigatorNote, displayName)
    }

    var swapDexAggrigatorDisclaimer: String {
      let network: String
      switch self {
      case .zeroX: network = Strings.Wallet.coinTypeEthereum
      case .jupiter: network = Strings.Wallet.coinTypeSolana
      }
      return String.localizedStringWithFormat(
        Strings.Wallet.swapDexAggrigatorDisclaimer,
        displayName,
        network,
        displayName
      )
    }
  }

  /// The DEX Aggregator for the current network.
  var dexAggregator: DEXAggregator {
    // TODO(stephenheaps): We may need to remove and/or update
    // this disclaimer to include LiFi description & privacy
    // policy https://github.com/brave/brave-browser/issues/36436
    networkStore.defaultSelectedChain.coin == .sol ? .jupiter : .zeroX
  }

  @ViewBuilder var unsupportedSwapChainSection: some View {
    Section {
      VStack(alignment: .leading, spacing: 4.0) {
        Text(Strings.Wallet.swapCryptoUnsupportNetworkTitle)
          .font(.headline)
        Text(
          String.localizedStringWithFormat(
            Strings.Wallet.swapCryptoUnsupportNetworkDescription,
            networkStore.defaultSelectedChain.chainName
          )
        )
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
    guard !swapTokensStore.isMakingTx && !swapTokensStore.isUpdatingPriceQuote else {
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
      return String.localizedStringWithFormat(
        Strings.Wallet.activateToken,
        swapTokensStore.selectedFromToken?.symbol ?? ""
      )
    case .swap, .idle:
      return Strings.Wallet.swapCryptoSwapButtonTitle
    }
  }

  @ViewBuilder var swapFormSections: some View {
    Section {
      NavigationLink(
        destination: SwapTokenSearchView(
          swapTokenStore: swapTokensStore,
          searchType: .fromToken,
          network: networkStore.defaultSelectedChain
        )
      ) {
        HStack {
          if let token = swapTokensStore.selectedFromToken {
            AssetIconView(
              token: token,
              network: networkStore.defaultSelectedChain,
              length: 26
            )
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
      .listRowBackground(Color(.secondaryBraveGroupedBackground))
    }
    Section(
      header: WalletListHeaderView(
        title: Text(
          String.localizedStringWithFormat(
            Strings.Wallet.swapCryptoAmountTitle,
            swapTokensStore.selectedFromToken?.symbol ?? ""
          )
        )
      ),
      footer: VStack(spacing: 20) {
        ShortcutAmountGrid(action: { amount in
          swapTokensStore.suggestedAmountTapped(amount)
        })
        Button {
          swapTokensStore.swapSelectedTokens()
        } label: {
          Label(Strings.Wallet.swapSelectedTokens, systemImage: "chevron.up.chevron.down")
            .labelStyle(.iconOnly)
            .font(.body)
            .foregroundColor(Color(.bravePrimary))
            .padding(.horizontal, 20)
            .padding(.vertical, 5)
            .background(
              Color(.secondaryButtonTint)
                .clipShape(Capsule().inset(by: 0.5).stroke())
            )
            .clipShape(Capsule())
        }
      }
      .listRowInsets(.zero)
    ) {
      TextField(
        String.localizedStringWithFormat(
          Strings.Wallet.amountInCurrency,
          swapTokensStore.selectedFromToken?.symbol ?? ""
        ),
        text: $swapTokensStore.sellAmount
      )
      .keyboardType(.decimalPad)
      .listRowBackground(Color(.secondaryBraveGroupedBackground))
    }
    Section(
      header: WalletListHeaderView(title: Text(Strings.Wallet.swapCryptoToTitle))
    ) {
      NavigationLink(
        destination: SwapTokenSearchView(
          swapTokenStore: swapTokensStore,
          searchType: .toToken,
          network: networkStore.defaultSelectedChain
        )
      ) {
        HStack {
          if let token = swapTokensStore.selectedToToken {
            AssetIconView(
              token: token,
              network: networkStore.defaultSelectedChain,
              length: 26
            )
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
      .listRowBackground(Color(.secondaryBraveGroupedBackground))
    }
    Section(
      header: WalletListHeaderView(
        title: Text(
          String.localizedStringWithFormat(
            Strings.Wallet.swapCryptoAmountReceivingTitle,
            swapTokensStore.selectedToToken?.symbol ?? ""
          )
        )
      )
    ) {
      TextField(
        String.localizedStringWithFormat(
          Strings.Wallet.amountInCurrency,
          swapTokensStore.selectedToToken?.symbol ?? ""
        ),
        text: $swapTokensStore.buyAmount
      )
      .keyboardType(.decimalPad)
      .disabled(networkStore.defaultSelectedChain.coin == .sol)
      .listRowBackground(Color(.secondaryBraveGroupedBackground))
    }
    Section(
      //      MVP only supports market price swap. Ref: https://github.com/brave/brave-browser/issues/18307
      //
      //      header: Picker(Strings.Wallet.swapOrderTypeLabel, selection: $orderType) {
      //        Text(Strings.Wallet.swapMarketOrderType).tag(OrderType.market)
      //        Text(Strings.Wallet.swapLimitOrderType).tag(OrderType.limit)
      //      }
      //        .pickerStyle(SegmentedPickerStyle())
      //        .resetListHeaderStyle()
      //        .padding(.bottom, 15)
      //        .listRowBackground(Color(.clear))
      header: MarketPriceView(swapTokenStore: swapTokensStore)
        .listRowBackground(Color(UIColor.braveGroupedBackground))
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
      Button {
        withAnimation(.easeInOut(duration: 0.25)) {
          hideSlippage.toggle()
        }
      } label: {
        HStack {
          Text(Strings.Wallet.swapCryptoSlippageTitle)
            .font(.subheadline)
          Spacer()
          Text(formatSlippage)
            .foregroundColor(Color(.secondaryBraveLabel))
            .font(.subheadline.weight(.semibold))
          Image("wallet-dismiss", bundle: .module)
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
      .listRowBackground(Color(.secondaryBraveGroupedBackground))
    }
    Section(
      header:
        VStack(spacing: 16) {
          feesFooter

          WalletLoadingButton(
            isLoading: swapTokensStore.isMakingTx || swapTokensStore.isUpdatingPriceQuote,
            action: {
              Task { @MainActor in
                let success = await swapTokensStore.createSwapTransaction()
                if success {
                  appRatingRequest?()
                }
                completion?(success)
              }
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
      Button {
        isSwapDisclaimerVisible = true
      } label: {
        HStack {
          Text(dexAggregator.swapDexAggrigatorNote)
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
          title: Text(dexAggregator.swapDexAggrigatorNote),
          message: Text(dexAggregator.swapDexAggrigatorDisclaimer),
          primaryButton: Alert.Button.default(
            Text(Strings.learnMore),
            action: {
              openWalletURL(dexAggregator.url)
            }
          ),
          secondaryButton: Alert.Button.cancel(Text(Strings.OKString))
        )
      }
      .frame(maxWidth: .infinity)
      .font(.footnote)
      .listRowBackground(Color(.braveGroupedBackground))
    }
  }

  @ViewBuilder private var feesFooter: some View {
    if swapTokensStore.braveFeeForDisplay != nil {
      VStack(spacing: 4) {
        if let braveFeeForDisplay = swapTokensStore.braveFeeForDisplay {
          if swapTokensStore.isBraveFeeVoided {
            Text(
              String.localizedStringWithFormat(
                Strings.Wallet.braveFeeLabel,
                Strings.Wallet.braveSwapFree
              ) + " "
            ) + Text(braveFeeForDisplay).strikethrough()
          } else {
            Text(String.localizedStringWithFormat(Strings.Wallet.braveFeeLabel, braveFeeForDisplay))
          }
        }
      }
      .font(.footnote)
      .foregroundColor(Color(.braveLabel))
    }
  }

  enum OrderType {
    case market
    case limit
  }

  var body: some View {
    NavigationView {
      Form {
        Section {
          AccountPicker(
            keyringStore: keyringStore,
            networkStore: networkStore
          )
          .listRowBackground(Color(UIColor.braveGroupedBackground))
          .resetListHeaderStyle()
        }
        if networkStore.isSwapSupported {
          swapFormSections
        } else {
          unsupportedSwapChainSection
        }
      }
      .listBackgroundColor(Color(.braveGroupedBackground))
      .environment(\.defaultMinListHeaderHeight, 0)
      .environment(\.defaultMinListRowHeight, 0)
      .navigationTitle(Strings.Wallet.swap)
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
      .onAppear {
        swapTokensStore.prepare(with: keyringStore.selectedAccount)
      }
    }
    .navigationViewStyle(.stack)
  }
}

#if DEBUG
struct SwapCryptoView_Previews: PreviewProvider {
  static var previews: some View {
    SwapCryptoView(
      keyringStore: .previewStore,
      networkStore: .previewStore,
      swapTokensStore: .previewStore,
      onDismiss: {}
    )
    //      .previewSizeCategories([.large, .accessibilityLarge])
  }
}
#endif
