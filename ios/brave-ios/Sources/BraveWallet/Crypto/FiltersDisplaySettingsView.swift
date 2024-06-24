// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import DesignSystem
import Preferences
import SwiftUI

public enum GroupBy: Int, CaseIterable, Identifiable, UserDefaultsEncodable {
  case none
  case accounts
  case networks

  var title: String {
    switch self {
    case .none: return Strings.Wallet.groupByNoneOptionTitle
    case .accounts: return Strings.Wallet.groupByAccountsOptionTitle
    case .networks: return Strings.Wallet.groupByNetworksOptionTitle
    }
  }
  public var id: String { title }
}

public enum SortOrder: Int, CaseIterable, Identifiable, UserDefaultsEncodable {
  /// Fiat value lowest to highest
  case valueAsc
  /// Fiat value highest to lowest
  case valueDesc
  /// A-Z
  case alphaAsc
  /// Z-A
  case alphaDesc

  var title: String {
    switch self {
    case .valueAsc: return Strings.Wallet.lowToHighSortOption
    case .valueDesc: return Strings.Wallet.highToLowSortOption
    case .alphaAsc: return Strings.Wallet.aToZSortOption
    case .alphaDesc: return Strings.Wallet.zToASortOption
    }
  }
  public var id: String { title }
}

struct Filters {
  /// How the assets should be grouped. Default is none / no grouping.
  let groupBy: GroupBy
  /// Ascending order is smallest fiat to largest fiat. Default is descending order.
  let sortOrder: SortOrder
  /// If we are hiding small balances (less than $1 value). Default is true.
  let isHidingSmallBalances: Bool
  /// If we are hiding unowned NFTs. Default is false.
  let isHidingUnownedNFTs: Bool
  /// If we are showing the network logo on NFTs. Default is true.
  let isShowingNFTNetworkLogo: Bool
  /// All accounts and if they are currently selected. Default is all accounts selected.
  var accounts: [Selectable<BraveWallet.AccountInfo>]
  /// All networks and if they are currently selected. Default is all selected except known test networks.
  var networks: [Selectable<BraveWallet.NetworkInfo>]

  init(
    groupBy: GroupBy = GroupBy(rawValue: Preferences.Wallet.groupByFilter.value) ?? .none,
    sortOrder: SortOrder = SortOrder(rawValue: Preferences.Wallet.sortOrderFilter.value)
      ?? .valueDesc,
    isHidingSmallBalances: Bool = Preferences.Wallet.isHidingSmallBalancesFilter.value,
    isHidingUnownedNFTs: Bool = Preferences.Wallet.isHidingUnownedNFTsFilter.value,
    isShowingNFTNetworkLogo: Bool = Preferences.Wallet.isShowingNFTNetworkLogoFilter.value,
    accounts: [Selectable<BraveWallet.AccountInfo>],
    networks: [Selectable<BraveWallet.NetworkInfo>]
  ) {
    self.groupBy = groupBy
    self.sortOrder = sortOrder
    self.isHidingSmallBalances = isHidingSmallBalances
    self.isHidingUnownedNFTs = isHidingUnownedNFTs
    self.isShowingNFTNetworkLogo = isShowingNFTNetworkLogo
    self.accounts = accounts
    self.networks = networks
  }

  func save() {
    Preferences.Wallet.groupByFilter.value = groupBy.rawValue
    Preferences.Wallet.sortOrderFilter.value = sortOrder.rawValue
    Preferences.Wallet.isHidingSmallBalancesFilter.value = isHidingSmallBalances
    Preferences.Wallet.isShowingNFTNetworkLogoFilter.value = isShowingNFTNetworkLogo
    Preferences.Wallet.isHidingUnownedNFTsFilter.value = isHidingUnownedNFTs
    Preferences.Wallet.nonSelectedAccountsFilter.value =
      accounts
      .filter({ !$0.isSelected })
      .map(\.model.id)
    Preferences.Wallet.nonSelectedNetworksFilter.value =
      networks
      .filter({ !$0.isSelected })
      .map(\.model.chainId)
  }
}

struct FiltersDisplaySettingsView: View {

  /// How the assets are grouped. Unavailable until Portfolio supports grouping.
  @State var groupBy: GroupBy
  /// Ascending order is smallest fiat to largest fiat. Default is descending order.
  @State var sortOrder: SortOrder
  /// If we are hiding small balances (less than $1 value). Default is false.
  @State var isHidingSmallBalances: Bool
  /// If we are hiding unowned NFTs. Default is false.
  @State var isHidingUnownedNFTs: Bool
  /// If we are showing the network logo on NFTs. Default is true.
  @State var isShowingNFTNetworkLogo: Bool
  /// If we should disable `Hide Unowned`
  @State var isHidingUnownedNFTsDisabled: Bool

  /// All accounts and if they are currently selected. Default is all accounts selected.
  @State var accounts: [Selectable<BraveWallet.AccountInfo>]
  /// All networks and if they are currently selected. Default is all selected except known test networks.
  @State var networks: [Selectable<BraveWallet.NetworkInfo>]

  let isNFTFilters: Bool
  let networkStore: NetworkStore
  let save: (Filters) -> Void

  private let originalFilters: Filters

  /// Returns true if all accounts are selected
  var allAccountsSelected: Bool {
    accounts.allSatisfy(\.isSelected)
  }

  /// Returns true if all visible networks are selected
  var allNetworksSelected: Bool {
    networks
      .filter { network in
        !networkStore.hiddenChains.contains { hiddenChain in
          hiddenChain.chainId == network.model.chainId
        }
      }
      .allSatisfy(\.isSelected)
  }

  @State private var isShowingNetworksDetail: Bool = false
  @Environment(\.dismiss) private var dismiss

  /// Size of the circle containing the icon for each filter.
  /// The `relativeTo: .headline` should match icon's `TextStyle` in `FilterLabelView`.
  @ScaledMetric(relativeTo: .headline) private var iconContainerSize: CGFloat = 40
  private var maxIconContainerSize: CGFloat = 80
  private let rowPadding: CGFloat = 16

  init(
    filters: Filters,
    isNFTFilters: Bool,
    networkStore: NetworkStore,
    save: @escaping (Filters) -> Void
  ) {
    self._groupBy = State(initialValue: filters.groupBy)
    self._sortOrder = State(initialValue: filters.sortOrder)
    self._isHidingSmallBalances = State(initialValue: filters.isHidingSmallBalances)
    self._isHidingUnownedNFTs = State(
      initialValue: filters.groupBy == .accounts ? true : filters.isHidingUnownedNFTs
    )
    self._isShowingNFTNetworkLogo = State(initialValue: filters.isShowingNFTNetworkLogo)
    self._isHidingUnownedNFTsDisabled = State(initialValue: filters.groupBy == .accounts)
    self._accounts = State(initialValue: filters.accounts)
    self._networks = State(initialValue: filters.networks)
    self.isNFTFilters = isNFTFilters
    self.networkStore = networkStore
    self.save = save
    self.originalFilters = filters
  }

  var body: some View {
    NavigationView {
      ScrollView {
        LazyVStack(spacing: 0) {
          if isNFTFilters {
            groupByRow
              .padding(.vertical, rowPadding)

            showNFTNetworkLogo
              .padding(.vertical, rowPadding)

            hideUnownedNFTs
              .padding(.vertical, rowPadding)
          } else {  // Portfolio filters
            groupByRow
              .padding(.vertical, rowPadding)

            sortAssets
              .padding(.vertical, rowPadding)

            hideSmallBalances
              .padding(.vertical, rowPadding)
          }

          DividerLine()

          accountFilters
            .padding(.vertical, rowPadding)

          networkFilters
            .padding(.vertical, rowPadding)

        }
        .padding(.horizontal)
      }
      .onChange(
        of: groupBy,
        perform: { newValue in
          if isNFTFilters {
            if newValue == .accounts {
              isHidingUnownedNFTs = true
              isHidingUnownedNFTsDisabled = true
            } else {
              isHidingUnownedNFTsDisabled = false
            }
          }
        }
      )
      .background(Color(uiColor: WalletV2Design.containerBackground))
      .safeAreaInset(
        edge: .bottom,
        content: {
          saveChangesContainer
        }
      )
      .navigationTitle(Strings.Wallet.filtersAndDisplaySettings)
      .navigationBarTitleDisplayMode(.inline)
      .toolbar {
        ToolbarItem(placement: .navigationBarTrailing) {
          Button(action: resetToDefaults) {
            Text(Strings.Wallet.settingsResetTransactionAlertButtonTitle)
              .fontWeight(.semibold)
          }
          .disabled(isResetDisabled)
        }
      }
    }
  }

  private var groupByRow: some View {
    FilterPickerRowView(
      title: Strings.Wallet.groupByTitle,
      description: Strings.Wallet.groupByDescription,
      icon: .init(
        braveSystemName: "leo.stack",
        iconContainerSize: min(iconContainerSize, maxIconContainerSize)
      ),
      allOptions: GroupBy.allCases,
      selection: $groupBy
    ) { groupBy in
      Text(groupBy.title)
    }
  }

  private var sortAssets: some View {
    FilterPickerRowView(
      title: Strings.Wallet.sortAssetsTitle,
      description: Strings.Wallet.sortAssetsDescription,
      icon: .init(
        braveSystemName: "leo.sort.desc",
        iconContainerSize: min(iconContainerSize, maxIconContainerSize)
      ),
      allOptions: SortOrder.allCases,
      selection: $sortOrder
    ) { sortOrder in
      Text(sortOrder.title)
    }
  }

  private var hideSmallBalances: some View {
    Toggle(isOn: $isHidingSmallBalances) {
      FilterLabelView(
        title: Strings.Wallet.hideSmallBalancesTitle,
        description: Strings.Wallet.hideSmallBalancesDescription,
        icon: .init(
          braveSystemName: "leo.eye.on",
          iconContainerSize: min(iconContainerSize, maxIconContainerSize)
        )
      )
    }
    .tint(Color(.braveBlurpleTint))
  }

  private var hideUnownedNFTs: some View {
    Toggle(isOn: $isHidingUnownedNFTs) {
      FilterLabelView(
        title: Strings.Wallet.hideUnownedNFTsTitle,
        description: Strings.Wallet.hideUnownedNFTsDescription,
        icon: .init(
          braveSystemName: "leo.eye.on",
          iconContainerSize: min(iconContainerSize, maxIconContainerSize)
        )
      )
    }
    .tint(Color(.braveBlurpleTint))
    .disabled(isHidingUnownedNFTsDisabled)
  }

  private var showNFTNetworkLogo: some View {
    Toggle(isOn: $isShowingNFTNetworkLogo) {
      FilterLabelView(
        title: Strings.Wallet.showNFTNetworkLogoTitle,
        description: Strings.Wallet.showNFTNetworkLogoDescription,
        icon: .init(
          braveSystemName: "leo.web3",
          iconContainerSize: min(iconContainerSize, maxIconContainerSize)
        )
      )
    }
    .tint(Color(.braveBlurpleTint))
  }

  private var accountFilters: some View {
    NavigationLink(
      destination: {
        AccountFilterView(
          accounts: $accounts
        )
      },
      label: {
        FilterDetailRowView(
          title: Strings.Wallet.selectAccountsTitle,
          description: Strings.Wallet.selectAccountsDescription,
          icon: .init(
            braveSystemName: "leo.user.accounts",
            iconContainerSize: iconContainerSize
          ),
          selectionView: {
            if allAccountsSelected {
              AllSelectedView(title: Strings.Wallet.allAccountsLabel)
            } else if accounts.contains(where: { $0.isSelected }) {  // at least 1 selected
              MultipleAccountBlockiesView(
                accountAddresses: accounts.filter(\.isSelected).map(\.model.address)
              )
            }
          }
        )
      }
    )
    .buttonStyle(FadeButtonStyle())
  }

  private var networkFilters: some View {
    NavigationLink {
      NetworkFilterView(
        networks: networks,
        networkStore: networkStore,
        showsCancelButton: false,
        requiresSave: false,
        saveAction: { selectedNetworks in
          networks = selectedNetworks
        }
      )
    } label: {
      FilterDetailRowView(
        title: Strings.Wallet.selectNetworksTitle,
        description: Strings.Wallet.selectNetworksDescription,
        icon: .init(
          braveSystemName: "leo.internet",
          iconContainerSize: iconContainerSize
        ),
        selectionView: {
          if allNetworksSelected {
            AllSelectedView(title: Strings.Wallet.allNetworksLabel)
          } else if networks.contains(where: { $0.isSelected }) {  // at least 1 selected
            MultipleNetworkIconsView(
              networks: networks.filter(\.isSelected).map(\.model)
            )
          }
        }
      )
    }
    .buttonStyle(FadeButtonStyle())
  }

  private var isSaveChangesDisabled: Bool {
    originalFilters.groupBy == groupBy && originalFilters.sortOrder == sortOrder
      && originalFilters.isHidingSmallBalances == isHidingSmallBalances
      && originalFilters.isHidingUnownedNFTs == isHidingUnownedNFTs
      && originalFilters.isShowingNFTNetworkLogo == isShowingNFTNetworkLogo
      && originalFilters.accounts == accounts && originalFilters.networks == networks
  }

  private var saveChangesContainer: some View {
    VStack {
      Button {
        let filters = Filters(
          groupBy: groupBy,
          sortOrder: sortOrder,
          isHidingSmallBalances: isHidingSmallBalances,
          isHidingUnownedNFTs: isHidingUnownedNFTs,
          isShowingNFTNetworkLogo: isShowingNFTNetworkLogo,
          accounts: accounts,
          networks: networks
        )
        save(filters)
        dismiss()
      } label: {
        Text(Strings.Wallet.saveChangesButtonTitle)
          .fontWeight(.semibold)
          .frame(maxWidth: .infinity)
          .padding(.vertical, 4)
      }
      .buttonStyle(BraveFilledButtonStyle(size: .large))
      .disabled(isSaveChangesDisabled)

      Button {
        dismiss()
      } label: {
        Text(Strings.CancelString)
          .fontWeight(.semibold)
          .foregroundColor(Color(uiColor: WalletV2Design.textInteractive))
          .frame(maxWidth: .infinity)
          .padding(.vertical, 4)
      }
    }
    .padding(.horizontal)
    .padding(.vertical, 14)
    .background(
      Color(uiColor: WalletV2Design.containerBackground)
        .ignoresSafeArea()
    )
    .shadow(color: Color.black.opacity(0.04), radius: 16, x: 0, y: -8)
  }

  private var isResetDisabled: Bool {
    groupBy == GroupBy(rawValue: Preferences.Wallet.groupByFilter.defaultValue) ?? .none
      && sortOrder == SortOrder(rawValue: Preferences.Wallet.sortOrderFilter.defaultValue)
        ?? .valueDesc
      && isHidingSmallBalances == Preferences.Wallet.isHidingSmallBalancesFilter.defaultValue
      && isHidingUnownedNFTs == Preferences.Wallet.isHidingUnownedNFTsFilter.defaultValue
      && isShowingNFTNetworkLogo == Preferences.Wallet.isShowingNFTNetworkLogoFilter.defaultValue
      && accounts.allSatisfy(\.isSelected)
      && networks.allSatisfy { selectableNetwork in
        let isTestnet = WalletConstants.supportedTestNetworkChainIds.contains(
          selectableNetwork.model.chainId
        )
        return selectableNetwork.isSelected == !isTestnet
      }
  }

  func resetToDefaults() {
    /// Assets are not grouped by default
    self.groupBy = GroupBy(rawValue: Preferences.Wallet.groupByFilter.defaultValue) ?? .none
    // Fiat value in descending order (largest fiat to smallest) by default
    self.sortOrder =
      SortOrder(rawValue: Preferences.Wallet.sortOrderFilter.defaultValue) ?? .valueDesc
    self.isHidingSmallBalances = Preferences.Wallet.isHidingSmallBalancesFilter.defaultValue
    self.isHidingUnownedNFTs = Preferences.Wallet.isHidingUnownedNFTsFilter.defaultValue
    self.isShowingNFTNetworkLogo = Preferences.Wallet.isShowingNFTNetworkLogoFilter.defaultValue

    // All accounts selected by default
    self.accounts = self.accounts.map {
      .init(isSelected: true, model: $0.model)
    }
    // All non-test networks selected by default
    self.networks = self.networks.map {
      let isTestnet = WalletConstants.supportedTestNetworkChainIds.contains($0.model.chainId)
      return .init(isSelected: !isTestnet, model: $0.model)
    }
  }

  func selectAllAccounts() {
    self.accounts = self.accounts.map {
      .init(isSelected: true, model: $0.model)
    }
  }

  func selectAllNetworks() {
    self.networks = self.networks.map {
      .init(isSelected: true, model: $0.model)
    }
  }
}

#if DEBUG
struct FiltersDisplaySettingsView_Previews: PreviewProvider {
  static var previews: some View {
    FiltersDisplaySettingsView(
      filters: Filters(
        groupBy: .none,
        sortOrder: .valueDesc,
        isHidingSmallBalances: false,
        isHidingUnownedNFTs: false,
        isShowingNFTNetworkLogo: false,
        accounts: [
          .init(isSelected: true, model: .mockEthAccount),
          .init(isSelected: true, model: .mockSolAccount),
        ],
        networks: [
          .init(isSelected: true, model: .mockMainnet),
          .init(isSelected: true, model: .mockSolana),
          .init(isSelected: true, model: .mockPolygon),
          .init(isSelected: false, model: .mockSolanaTestnet),
          .init(isSelected: false, model: .mockSepolia),
        ]
      ),
      isNFTFilters: false,
      networkStore: .previewStore,
      save: { _ in }
    )
  }
}
#endif

private struct AllSelectedView: View {
  let title: String

  var body: some View {
    Text(title)
      .foregroundColor(Color(uiColor: WalletV2Design.extendedGray50))
      .font(.footnote.weight(.semibold))
      .padding(.horizontal, 2)
      .padding(4)
      .background(Color(uiColor: WalletV2Design.extendedGray20))
      .clipShape(RoundedRectangle(cornerRadius: 6))
  }
}

struct FilterIconInfo {
  let braveSystemName: String
  let iconContainerSize: CGFloat
}

// View with icon, title and description.
private struct FilterLabelView: View {

  let title: String
  let description: String
  let icon: FilterIconInfo?

  var body: some View {
    HStack {
      if let icon {
        Color(uiColor: WalletV2Design.containerHighlight)
          .clipShape(Circle())
          .frame(width: icon.iconContainerSize, height: icon.iconContainerSize)
          .overlay {
            Image(braveSystemName: icon.braveSystemName)
              .imageScale(.medium)
              .font(.headline)
              .foregroundColor(Color(uiColor: WalletV2Design.iconDefault))
          }
      }
      VStack(alignment: .leading, spacing: 2) {
        Text(title)
          .font(.body.weight(.semibold))
          .foregroundColor(Color(uiColor: WalletV2Design.text01))
        Text(description)
          .font(.footnote)
          .foregroundColor(Color(uiColor: WalletV2Design.textSecondary))
      }
      .multilineTextAlignment(.leading)
    }
  }
}

// `FilterLabelView` with a detail disclosure.
private struct FilterDetailRowView<SelectionView: View>: View {

  let title: String
  let description: String
  let icon: FilterIconInfo?
  @ViewBuilder let selectionView: () -> SelectionView

  var body: some View {
    HStack {
      FilterLabelView(
        title: title,
        description: description,
        icon: icon
      )
      Spacer()
      selectionView()
      Image(systemName: "chevron.right")
        .font(.body.weight(.semibold))
        .foregroundColor(Color(.separator))
    }
    .contentShape(Rectangle())
  }
}

/// Displays provided options in a context menu allowing a single selection.
struct FilterPickerRowView<T: Equatable & Identifiable & Hashable, Content: View>: View {

  let title: String
  let description: String
  let icon: FilterIconInfo?

  let allOptions: [T]
  @Binding var selection: T
  let content: (T) -> Content

  var body: some View {
    HStack {
      FilterLabelView(
        title: title,
        description: description,
        icon: icon
      )
      Spacer()
      Menu(
        content: {
          Picker(
            selection: $selection,
            content: {
              ForEach(allOptions) { option in
                content(option)
                  .tag(option)
              }
            },
            label: {
              // Menu label is used
              EmptyView()
            }
          )
        },
        label: {
          HStack(spacing: 8) {
            content(selection)
            Image(braveSystemName: "leo.carat.down")
          }
        }
      )
      .osAvailabilityModifiers({
        if #unavailable(iOS 17) {
          // Prior to iOS 17, if selection changes from outside
          // the Menu (ex. Reset button) the view might not
          // resize to fit a larger label
          $0.id(selection)
        } else {
          $0
        }
      })
      .foregroundColor(Color(WalletV2Design.textInteractive))
      .transaction { transaction in
        transaction.animation = nil
        transaction.disablesAnimations = true
      }
    }
  }
}

struct DividerLine: View {
  var body: some View {
    Color(uiColor: WalletV2Design.dividerSubtle)
      .frame(height: 1)
  }
}

struct FadeButtonStyle: ButtonStyle {
  @Environment(\.isEnabled) private var isEnabled

  func makeBody(configuration: Configuration) -> some View {
    configuration.label
      .opacity(configuration.isPressed ? 0.7 : 1.0)
      .clipShape(Rectangle())
  }
}
