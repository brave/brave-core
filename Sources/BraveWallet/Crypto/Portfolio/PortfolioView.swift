/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import SwiftUI
import BraveCore
import SnapKit
import Introspect
import Strings
import DesignSystem
import BraveUI
import Shared
import Preferences

struct PortfolioView: View {
  var cryptoStore: CryptoStore
  @ObservedObject var keyringStore: KeyringStore
  @ObservedObject var networkStore: NetworkStore
  @ObservedObject var portfolioStore: PortfolioStore

  @State private var dismissedBackupBannerThisSession: Bool = false
  @State private var isPresentingBackup: Bool = false
  @State private var isPresentingEditUserAssets: Bool = false
  @State private var isPresentingFiltersDisplaySettings: Bool = false
  
  @Environment(\.sizeCategory) private var sizeCategory
  @Environment(\.buySendSwapDestination)
  private var buySendSwapDestination: Binding<BuySendSwapDestination?>
  /// Reference to the collection view used to back the `List` on iOS 16+
  @State private var collectionViewRef: WeakRef<UICollectionView>?

  private var isShowingBackupBanner: Bool {
    !keyringStore.defaultKeyring.isBackedUp && !dismissedBackupBannerThisSession
  }

  private var listHeader: some View {
    VStack(spacing: 0) {
      if isShowingBackupBanner {
        BackupNotifyView(
          action: {
            isPresentingBackup = true
          },
          onDismiss: {
            // Animating this doesn't seem to work in SwiftUI.. will keep an eye out for iOS 15
            dismissedBackupBannerThisSession = true
          }
        )
        .buttonStyle(PlainButtonStyle())
        .padding([.top, .leading, .trailing], 12)
        .sheet(isPresented: $isPresentingBackup) {
          NavigationView {
            BackupWalletView(
              password: nil,
              keyringStore: keyringStore
            )
          }
          .environment(\.modalPresentationMode, $isPresentingBackup)
          .accentColor(Color(.braveBlurpleTint))
        }
      }
      BalanceHeaderView(
        balance: portfolioStore.balance,
        historicalBalances: portfolioStore.historicalBalances,
        isLoading: portfolioStore.isLoadingBalances,
        keyringStore: keyringStore,
        networkStore: networkStore,
        selectedDateRange: $portfolioStore.timeframe
      )
    }
  }

  @State private var tableInset: CGFloat = -16.0

  @State private var selectedToken: BraveWallet.BlockchainToken?
  
  private var editUserAssetsButton: some View {
    Button(action: { isPresentingEditUserAssets = true }) {
      Image(braveSystemName: "leo.list.settings")
        .font(.footnote.weight(.medium))
        .foregroundColor(Color(.braveBlurpleTint))
        .clipShape(Rectangle())
    }
    .sheet(isPresented: $isPresentingEditUserAssets) {
      EditUserAssetsView(
        networkStore: networkStore,
        keyringStore: keyringStore,
        userAssetsStore: portfolioStore.userAssetsStore
      ) {
        cryptoStore.updateAssets()
      }
    }
  }
  
  private var filtersButton: some View {
    Button(action: {
      self.isPresentingFiltersDisplaySettings = true
    }) {
      Image(braveSystemName: "leo.tune")
        .font(.footnote.weight(.medium))
        .foregroundColor(Color(.braveBlurpleTint))
        .clipShape(Rectangle())
    }
    .sheet(isPresented: $isPresentingFiltersDisplaySettings) {
      FiltersDisplaySettingsView(
        filters: portfolioStore.filters,
        isNFTFilters: false,
        networkStore: networkStore,
        save: { filters in
          portfolioStore.saveFilters(filters)
        }
      )
      .osAvailabilityModifiers({ view in
        if #available(iOS 16, *) {
          view
            .presentationDetents([
              .fraction(0.7),
              .large
            ])
        } else {
          view
        }
      })
    }
  }

  var body: some View {
    List {
      Section(
        header:
          listHeader
          .padding(.horizontal, tableInset)  // inset grouped layout margins workaround
          .resetListHeaderStyle()
      ) { }

      assetSections
    }
    .background(
      NavigationLink(
        isActive: Binding(
          get: { selectedToken != nil },
          set: { if !$0 { selectedToken = nil } }
        ),
        destination: {
          if let token = selectedToken {
            AssetDetailView(
              assetDetailStore: cryptoStore.assetDetailStore(for: .blockchainToken(token)),
              keyringStore: keyringStore,
              networkStore: cryptoStore.networkStore
            )
            .onDisappear {
              cryptoStore.closeAssetDetailStore(for: .blockchainToken(token))
            }
          }
        },
        label: {
          EmptyView()
        })
    )
    .animation(.default, value: portfolioStore.assetGroups)
    .listStyle(InsetGroupedListStyle())
    .listBackgroundColor(Color(UIColor.braveGroupedBackground))
    .introspectTableView { tableView in
      withAnimation(nil) {
        tableInset = -tableView.layoutMargins.left
      }
    }
    .onChange(of: sizeCategory) { _ in
      // Fix broken header when text size changes on iOS 16+
      self.collectionViewRef?.value?.collectionViewLayout.invalidateLayout()
    }
    .introspect(
      selector: TargetViewSelector.ancestorOrSiblingContaining
    ) { (collectionView: UICollectionView) in
      self.collectionViewRef = .init(collectionView)
    }
  }
  
  /// Header for the assets section(s) containing the title, edit user assets and filter buttons
  private var assetSectionsHeader: some View {
    HStack {
      Text(Strings.Wallet.assetsTitle)
      if portfolioStore.isLoadingDiscoverAssets {
        ProgressView()
          .padding(.leading, 5)
      }
      Spacer()
      editUserAssetsButton
        .padding(.trailing, 10)
      filtersButton
    }
    .textCase(nil)
    .padding(.horizontal, -8)
    .frame(maxWidth: .infinity, alignment: .leading)
  }
  
  private var emptyAssetsState: some View {
    VStack(spacing: 10) {
      Image("portfolio-empty", bundle: .module)
        .aspectRatio(contentMode: .fit)
      Text(Strings.Wallet.portfolioEmptyStateTitle)
        .font(.headline)
        .foregroundColor(Color(WalletV2Design.textPrimary))
      Text(Strings.Wallet.portfolioEmptyStateDescription)
        .font(.footnote)
        .foregroundColor(Color(WalletV2Design.textSecondary))
    }
    .multilineTextAlignment(.center)
    .padding(.vertical)
  }
  
  /// Grouped / Ungrouped asset `Section`s for the List
  @ViewBuilder private var assetSections: some View {
    if portfolioStore.isShowingAssetsLoadingState || portfolioStore.isShowingAssetsEmptyState {
      Section(content: {
        Group {
          if portfolioStore.isShowingAssetsLoadingState {
            SkeletonLoadingAssetView()
          } else { // isShowingAssetsEmptyState
            emptyAssetsState
          }
        }
        .listRowBackground(Color(.secondaryBraveGroupedBackground))
      }, header: {
        assetSectionsHeader
      })
    } else {
      ForEach(portfolioStore.assetGroups) { group in
        Section(
          content: {
            if group.groupType == .none {
              ungroupedAssets(group)
                .listRowBackground(Color(.secondaryBraveGroupedBackground))
            } else {
              groupedAssetsSection(for: group)
                .listRowBackground(Color(.secondaryBraveGroupedBackground))
            }
          },
          header: {
            if group == portfolioStore.assetGroups.first {
              assetSectionsHeader
            }
          }
        )
      }
    }
  }
  
  /// Builds the list of assets without any grouping or expandable / collapse behaviour.
  @ViewBuilder private func ungroupedAssets(_ group: AssetGroupViewModel) -> some View {
    ForEach(group.assets) { asset in
      Button(action: {
        selectedToken = asset.token
      }) {
        PortfolioAssetView(
          image: AssetIconView(
            token: asset.token,
            network: asset.network,
            shouldShowNetworkIcon: true
          ),
          title: asset.token.name,
          symbol: asset.token.symbol,
          networkName: asset.network.chainName,
          amount: asset.fiatAmount(currencyFormatter: portfolioStore.currencyFormatter),
          quantity: asset.quantity
        )
      }
    }
  }
  
  @State var groupToggleState: [AssetGroupViewModel.ID: Bool] = [:]
  
  /// Builds the expandable/collapseable (expanded by default) section content for a given group.
  @ViewBuilder private func groupedAssetsSection(for group: AssetGroupViewModel) -> some View {
    WalletDisclosureGroup(
      isExpanded: Binding(
        get: { groupToggleState[group.id, default: true] },
        set: { isExpanded in
          groupToggleState[group.id] = isExpanded
        }
      ),
      content: {
        ForEach(group.assets) { asset in
          Button(action: {
            selectedToken = asset.token
          }) {
            PortfolioAssetView(
              image: AssetIconView(
                token: asset.token,
                network: asset.network,
                shouldShowNetworkIcon: true
              ),
              title: asset.token.name,
              symbol: asset.token.symbol,
              networkName: asset.network.chainName,
              amount: asset.fiatAmount(currencyFormatter: portfolioStore.currencyFormatter),
              quantity: asset.quantity
            )
          }
        }
      },
      label: {
        if case let .account(account) = group.groupType {
          AddressView(address: account.address) {
            groupHeader(for: group)
          }
        } else {
          groupHeader(for: group)
        }
      }
    )
  }
  
  /// Builds the in-section header for an AssetGroupViewModel that is shown in expanded and non-expanded state. Not used for ungrouped assets.
  private func groupHeader(for group: AssetGroupViewModel) -> some View {
    VStack(spacing: 0) {
      HStack {
        if case let .network(networkInfo) = group.groupType {
          NetworkIcon(network: networkInfo, length: 32)
        } else if case let .account(accountInfo) = group.groupType {
          Blockie(address: accountInfo.address, shape: .rectangle)
            .frame(width: 32, height: 32)
            .clipShape(RoundedRectangle(cornerRadius: 4))
        }
        VStack(alignment: .leading) {
          Text(group.title)
            .font(.callout.weight(.semibold))
            .foregroundColor(Color(WalletV2Design.textPrimary))
          if let description = group.description {
            Text(description)
              .font(.footnote)
              .foregroundColor(Color(WalletV2Design.textSecondary))
          }
        }
        .multilineTextAlignment(.leading)
        Spacer()
        Text(portfolioStore.currencyFormatter.string(from: NSNumber(value: group.totalFiatValue)) ?? "")
          .font(.callout.weight(.semibold))
          .foregroundColor(Color(WalletV2Design.textPrimary))
          .multilineTextAlignment(.trailing)
      }
      .padding(.vertical, 4)
    }
  }
}

struct BalanceHeaderView: View {
  var balance: String
  var historicalBalances: [BalanceTimePrice]
  var isLoading: Bool
  var keyringStore: KeyringStore
  @ObservedObject var networkStore: NetworkStore
  @Binding var selectedDateRange: BraveWallet.AssetPriceTimeframe

  @Environment(\.sizeCategory) private var sizeCategory
  @Environment(\.horizontalSizeClass) private var horizontalSizeClass

  @State private var selectedBalance: BalanceTimePrice?

  private var balanceOrDataPointView: some View {
    HStack {
      Text(verbatim: balance)
        .font(.largeTitle.bold())
        .frame(maxWidth: .infinity, alignment: .leading)
        .opacity(selectedBalance == nil ? 1 : 0)
        .overlay(
          Group {
            if let dataPoint = selectedBalance {
              Text(dataPoint.formattedPrice)
                .font(.largeTitle.bold())
            }
          },
          alignment: .leading
        )
      if horizontalSizeClass == .regular {
        Spacer()
        DateRangeView(selectedRange: $selectedDateRange)
          .padding(6)
          .overlay(
            RoundedRectangle(cornerRadius: 10, style: .continuous)
              .strokeBorder(Color(.secondaryButtonTint))
          )
      }
    }
    .foregroundColor(.primary)
    .padding(.top, 12)
  }

  private var emptyBalanceData: [BalanceTimePrice] {
    // About 300 points added so it doesn't animate funny
    (0..<300).map { _ in .init(date: Date(), price: 0.0, formattedPrice: "") }
  }

  var body: some View {
    let chartData = historicalBalances.isEmpty ? emptyBalanceData : historicalBalances
    VStack(alignment: .leading, spacing: 4) {
      balanceOrDataPointView
      LineChartView(data: chartData, numberOfColumns: chartData.count, selectedDataPoint: $selectedBalance) {
        LinearGradient(braveGradient: .lightGradient02)
          .shimmer(isLoading)
      }
      .chartAccessibility(
        title: Strings.Wallet.portfolioPageTitle,
        dataPoints: chartData
      )
      .disabled(historicalBalances.isEmpty)
      .frame(height: 148)
      .padding(.horizontal, -12)
      .animation(.default, value: historicalBalances)
      if horizontalSizeClass == .compact {
        DateRangeView(selectedRange: $selectedDateRange)
      }
    }
    .padding(12)
  }
}

#if DEBUG
struct PortfolioViewController_Previews: PreviewProvider {
  static var previews: some View {
    NavigationView {
      PortfolioView(
        cryptoStore: .previewStore,
        keyringStore: .previewStore,
        networkStore: .previewStore,
        portfolioStore: CryptoStore.previewStore.portfolioStore
      )
      .navigationBarTitleDisplayMode(.inline)
    }
    .previewColorSchemes()
  }
}
#endif
