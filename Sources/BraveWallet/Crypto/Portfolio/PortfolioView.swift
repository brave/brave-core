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

struct PortfolioView: View {
  var cryptoStore: CryptoStore
  @ObservedObject var keyringStore: KeyringStore
  @ObservedObject var networkStore: NetworkStore
  @ObservedObject var portfolioStore: PortfolioStore

  @State private var dismissedBackupBannerThisSession: Bool = false
  @State private var isPresentingBackup: Bool = false
  @State private var isPresentingEditUserAssets: Bool = false
  @State private var isPresentingNetworkFilter: Bool = false
  
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
  @State private var selectedNFTViewModel: NFTAssetViewModel?
  
  private var editUserAssetsButton: some View {
    Button(action: { isPresentingEditUserAssets = true }) {
      Text(Strings.Wallet.editVisibleAssetsButtonTitle)
        .multilineTextAlignment(.center)
        .font(.footnote.weight(.semibold))
        .foregroundColor(Color(.bravePrimary))
        .frame(maxWidth: .infinity)
    }
    .sheet(isPresented: $isPresentingEditUserAssets) {
      EditUserAssetsView(
        networkStore: networkStore,
        keyringStore: keyringStore,
        userAssetsStore: portfolioStore.userAssetsStore
      ) {
        portfolioStore.update()
      }
    }
  }
  
  private var networkFilterButton: some View {
    Button(action: {
      self.isPresentingNetworkFilter = true
    }) {
      HStack {
        Text(portfolioStore.networkFilter.title)
        Image(braveSystemName: "leo.list")
      }
      .font(.footnote.weight(.medium))
      .foregroundColor(Color(.braveBlurpleTint))
    }
    .sheet(isPresented: $isPresentingNetworkFilter) {
      NavigationView {
        NetworkFilterView(
          networkFilter: $portfolioStore.networkFilter,
          networkStore: networkStore
        )
      }
      .onDisappear {
        networkStore.closeNetworkSelectionStore()
      }
    }
  }

  var body: some View {
    List {
      Section(
        header:
          listHeader
          .padding(.horizontal, tableInset)  // inset grouped layout margins workaround
          .resetListHeaderStyle()
      ) {
      }
      Section(content: {
        Group {
          ForEach(portfolioStore.userVisibleAssets) { asset in
            Button(action: {
              selectedToken = asset.token
            }) {
              PortfolioAssetView(
                image: AssetIconView(
                  token: asset.token,
                  network: asset.network,
                  shouldShowNativeTokenIcon: true
                ),
                title: asset.token.name,
                symbol: asset.token.symbol,
                networkName: asset.network.chainName,
                amount: portfolioStore.currencyFormatter.string(from: NSNumber(value: (Double(asset.price) ?? 0) * asset.decimalBalance)) ?? "",
                quantity: String(format: "%.04f", asset.decimalBalance)
              )
            }
          }
          editUserAssetsButton
        }
        .listRowBackground(Color(.secondaryBraveGroupedBackground))
      }, header: {
        HStack {
          Text(Strings.Wallet.assetsTitle)
          if portfolioStore.isLoadingDiscoverAssets {
            ProgressView()
              .padding(.leading, 5)
          }
          Spacer()
          networkFilterButton
        }
        .textCase(nil)
        .padding(.horizontal, -8)
        .frame(maxWidth: .infinity, alignment: .leading)
      })
      
      if !portfolioStore.userVisibleNFTs.isEmpty {
        Section(content: {
          Group {
            ForEach(portfolioStore.userVisibleNFTs) { nftAsset in
              Button(action: {
                selectedNFTViewModel = nftAsset
              }) {
                PortfolioNFTAssetView(
                  image: NFTIconView(
                    token: nftAsset.token,
                    network: nftAsset.network,
                    url: nftAsset.nftMetadata?.imageURL,
                    shouldShowNativeTokenIcon: true
                  ),
                  title: nftAsset.token.nftTokenTitle,
                  symbol: nftAsset.token.symbol,
                  networkName: nftAsset.network.chainName,
                  quantity: "\(nftAsset.balance)"
                )
              }
            }
            editUserAssetsButton
          }
          .listRowBackground(Color(.secondaryBraveGroupedBackground))
        }, header: {
          WalletListHeaderView(title: Text(Strings.Wallet.nftsTitle))
        })
      }
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
              assetDetailStore: cryptoStore.assetDetailStore(for: token),
              keyringStore: keyringStore,
              networkStore: cryptoStore.networkStore
            )
            .onDisappear {
              cryptoStore.closeAssetDetailStore(for: token)
            }
          }
        },
        label: {
          EmptyView()
        })
    )
    .background(
      NavigationLink(
        isActive: Binding(
          get: { selectedNFTViewModel != nil },
          set: { if !$0 { selectedNFTViewModel = nil } }
        ),
        destination: {
          if let nftViewModel = selectedNFTViewModel {
            if nftViewModel.token.isErc721 || nftViewModel.token.isNft {
              NFTDetailView(
                nftDetailStore: cryptoStore.nftDetailStore(for: nftViewModel.token, nftMetadata: nftViewModel.nftMetadata),
                buySendSwapDestination: buySendSwapDestination
              ) { erc721Metadata in
                portfolioStore.updateERC721MetadataCache(for: nftViewModel.token, metadata: erc721Metadata)
              }
              .onDisappear {
                cryptoStore.closeNFTDetailStore(for: nftViewModel.token)
              }
            } else {
              AssetDetailView(
                assetDetailStore: cryptoStore.assetDetailStore(for: nftViewModel.token),
                keyringStore: keyringStore,
                networkStore: cryptoStore.networkStore
              )
              .onDisappear {
                cryptoStore.closeAssetDetailStore(for: nftViewModel.token)
              }
            }
          }
        },
        label: {
          EmptyView()
        })
    )
    .animation(.default, value: portfolioStore.userVisibleAssets)
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
