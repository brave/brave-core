/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import SwiftUI
import BraveCore
import DesignSystem
import Preferences

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
  /// All accounts and if they are currently selected. Default is all accounts selected.
  var accounts: [Selectable<BraveWallet.AccountInfo>]
  /// All networks and if they are currently selected. Default is all selected except known test networks.
  var networks: [Selectable<BraveWallet.NetworkInfo>]
}

struct FiltersDisplaySettingsView: View {
  
  /// How the assets are grouped. Unavailable until Portfolio supports grouping.
  @State var groupBy: GroupBy
  /// Ascending order is smallest fiat to largest fiat. Default is descending order.
  @State var sortOrder: SortOrder
  /// If we are hiding small balances (less than $1 value). Default is false.
  @State var isHidingSmallBalances: Bool
  
  /// All accounts and if they are currently selected. Default is all accounts selected.
  @State var accounts: [Selectable<BraveWallet.AccountInfo>]
  /// All networks and if they are currently selected. Default is all selected except known test networks.
  @State var networks: [Selectable<BraveWallet.NetworkInfo>]
  
  var networkStore: NetworkStore
  let save: (Filters) -> Void
  
  /// Returns true if all accounts are selected
  var allAccountsSelected: Bool {
    accounts.allSatisfy(\.isSelected)
  }
  
  /// Returns true if all visible networks are selected
  var allNetworksSelected: Bool {
    networks
      .filter {
        if !Preferences.Wallet.showTestNetworks.value {
          return !WalletConstants.supportedTestNetworkChainIds.contains($0.model.chainId)
        }
        return true
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
    networkStore: NetworkStore,
    save: @escaping (Filters) -> Void
  ) {
    self._groupBy = State(initialValue: filters.groupBy)
    self._sortOrder = State(initialValue: filters.sortOrder)
    self._isHidingSmallBalances = State(initialValue: filters.isHidingSmallBalances)
    self._accounts = State(initialValue: filters.accounts)
    self._networks = State(initialValue: filters.networks)
    self.networkStore = networkStore
    self.save = save
  }
  
  var body: some View {
    NavigationView {
      ScrollView {
        LazyVStack(spacing: 0) {
          /*
           Unavailable until Portfolio supports grouping.
           groupByRow
            .padding(.vertical, rowPadding)
           */

          sortAssets
            .padding(.vertical, rowPadding)

          hideSmallBalances
            .padding(.vertical, rowPadding)

          DividerLine()

          accountFilters
            .padding(.vertical, rowPadding)

          networkFilters
            .padding(.vertical, rowPadding)

        }
        .padding(.horizontal)
      }
      .background(Color(uiColor: WalletV2Design.containerBackground))
      .safeAreaInset(edge: .bottom, content: {
        saveChangesContainer
      })
      .navigationTitle(Strings.Wallet.filtersAndDisplaySettings)
      .navigationBarTitleDisplayMode(.inline)
      .toolbar {
        ToolbarItem(placement: .navigationBarTrailing) {
          Button(action: restoreToDefaults) {
            Text(Strings.Wallet.settingsResetTransactionAlertButtonTitle)
              .fontWeight(.semibold)
              .foregroundColor(Color(uiColor: WalletV2Design.textInteractive))
          }
        }
      }
    }
  }
  
  private var groupByRow: some View {
    FilterPickerRowView(
      title: Strings.Wallet.groupByTitle,
      description: Strings.Wallet.groupByDescription,
      icon: .init(
        braveSystemName: "leo.list.bullet-default",
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
        braveSystemName: "leo.arrow.down",
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
  
  private var accountFilters: some View {
    NavigationLink(destination: {
      AccountFilterView(
        accounts: $accounts
      )
    }, label: {
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
          } else if accounts.contains(where: { $0.isSelected }) { // at least 1 selected
            MultipleAccountBlockiesView(
              accountAddresses: accounts.filter(\.isSelected).map(\.model.address)
            )
          }
        }
      )
    })
    .buttonStyle(FadeButtonStyle())
  }
  
  private var networkFilters: some View {
    NavigationLink(destination: {
      NetworkFilterView(
        networks: networks,
        networkStore: networkStore,
        showsCancelButton: false,
        requiresSave: false,
        saveAction: { selectedNetworks in
          networks = selectedNetworks
        }
      )
    }) {
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
          } else if networks.contains(where: { $0.isSelected }) { // at least 1 selected
            MultipleNetworkIconsView(
              networks: networks.filter(\.isSelected).map(\.model)
            )
          }
        }
      )
    }
    .buttonStyle(FadeButtonStyle())
  }
  
  private var saveChangesContainer: some View {
    VStack {
      Button(action: {
        let filters = Filters(
          groupBy: groupBy,
          sortOrder: sortOrder,
          isHidingSmallBalances: isHidingSmallBalances,
          accounts: accounts,
          networks: networks
        )
        save(filters)
        dismiss()
      }) {
        Text(Strings.Wallet.saveChangesButtonTitle)
          .fontWeight(.semibold)
          .frame(maxWidth: .infinity)
          .padding(.vertical, 4)
      }
      .buttonStyle(BraveFilledButtonStyle(size: .large))
      
      Button(action: { dismiss() }) {
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
  
  func restoreToDefaults() {
    self.groupBy = .none
    // Fiat value in descending order (largest fiat to smallest) by default
    self.sortOrder = .valueDesc
    // Small balances shown by default
    self.isHidingSmallBalances = false
    
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
        accounts: [
          .init(isSelected: true, model: .mockEthAccount),
          .init(isSelected: true, model: .mockSolAccount)
        ],
        networks: [
          .init(isSelected: true, model: .mockMainnet),
          .init(isSelected: true, model: .mockSolana),
          .init(isSelected: true, model: .mockPolygon),
          .init(isSelected: false, model: .mockSolanaTestnet),
          .init(isSelected: false, model: .mockGoerli)
        ]
      ),
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
      Menu(content: {
        ForEach(allOptions) { option in
          Button(action: { selection = option }) {
            HStack {
              Image(braveSystemName: "leo.check.normal")
                .resizable()
                .aspectRatio(contentMode: .fit)
                .hidden(isHidden: selection.id != option.id)
              content(option)
            }
          }
        }
      }, label: {
        HStack(spacing: 8) {
          content(selection)
          Image(braveSystemName: "leo.carat.down")
        }
      })
      .foregroundColor(Color(WalletV2Design.textInteractive))
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
