// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import SwiftUI
import DesignSystem
import Preferences
import BraveCore

struct NFTView: View {
  var cryptoStore: CryptoStore
  @ObservedObject var keyringStore: KeyringStore
  @ObservedObject var networkStore: NetworkStore
  @ObservedObject var nftStore: NFTStore
  
  @State private var isPresentingFiltersDisplaySettings: Bool = false
  @State private var isPresentingEditUserAssets: Bool = false
  @State private var selectedNFTViewModel: NFTAssetViewModel?
  @State private var isShowingNFTDiscoveryAlert: Bool = false
  @State private var isShowingAddCustomNFT: Bool = false
  @State private var isNFTDiscoveryEnabled: Bool = false
  
  @Environment(\.buySendSwapDestination)
  private var buySendSwapDestination: Binding<BuySendSwapDestination?>
  @Environment(\.openURL) private var openWalletURL
  
  private var emptyView: some View {
    VStack(alignment: .center, spacing: 10) {
      Text(nftStore.displayType.emptyTitle)
        .font(.headline.weight(.semibold))
        .foregroundColor(Color(.braveLabel))
      if let description = nftStore.displayType.emptyDescription {
        Text(description)
          .font(.subheadline.weight(.semibold))
          .foregroundColor(Color(.secondaryLabel))
      }
      Button(Strings.Wallet.nftEmptyImportNFT) {
        isShowingAddCustomNFT = true
      }
      .buttonStyle(BraveFilledButtonStyle(size: .normal))
      .hidden(isHidden: nftStore.displayType != .visible)
      .padding(.top, 8)
    }
    .multilineTextAlignment(.center)
    .frame(maxWidth: .infinity)
    .padding(.vertical, 60)
    .padding(.horizontal, 32)
  }
  
  private var editUserAssetsButton: some View {
    Button(action: { isPresentingEditUserAssets = true }) {
      Text(Strings.Wallet.editVisibleAssetsButtonTitle)
        .multilineTextAlignment(.center)
        .font(.footnote.weight(.semibold))
        .foregroundColor(Color(.braveBlurpleTint))
        .frame(maxWidth: .infinity)
    }
    .sheet(isPresented: $isPresentingEditUserAssets) {
      EditUserAssetsView(
        networkStore: networkStore,
        keyringStore: keyringStore,
        userAssetsStore: nftStore.userAssetsStore
      ) {
        cryptoStore.updateAssets()
      }
    }
  }
  
  private let nftGrids = [GridItem(.adaptive(minimum: 120), spacing: 16, alignment: .top)]
  
  @ViewBuilder private func nftLogo(_ nftViewModel: NFTAssetViewModel) -> some View {
    if let image = nftViewModel.network.nativeTokenLogoImage, nftStore.filters.isShowingNFTNetworkLogo {
      Image(uiImage: image)
        .resizable()
        .frame(width: 20, height: 20)
        .padding(4)
    }
  }
  
  @ViewBuilder private func nftImage(_ nftViewModel: NFTAssetViewModel) -> some View {
    Group {
      if let urlString = nftViewModel.nftMetadata?.imageURLString {
        NFTImageView(urlString: urlString) {
          noImageView(nftViewModel)
        }
      } else {
        noImageView(nftViewModel)
      }
    }
    .overlay(nftLogo(nftViewModel), alignment: .bottomTrailing)
    .cornerRadius(4)
  }
  
  @ViewBuilder private func noImageView(_ nftViewModel: NFTAssetViewModel) -> some View {
    Blockie(address: nftViewModel.token.contractAddress, shape: .rectangle)
      .overlay(
        Text(nftViewModel.token.symbol.first?.uppercased() ?? "")
          .font(.system(size: 80, weight: .bold, design: .rounded))
          .foregroundColor(.white)
          .shadow(color: .black.opacity(0.3), radius: 2, x: 0, y: 1)
      )
      .aspectRatio(1.0, contentMode: .fit)
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
        filters: nftStore.filters,
        isNFTFilters: true,
        networkStore: networkStore,
        save: { filters in
          nftStore.saveFilters(filters)
        }
      )
      .osAvailabilityModifiers({ view in
        if #available(iOS 16, *) {
          view
            .presentationDetents([
              .fraction(0.6),
              .large
            ])
        } else {
          view
        }
      })
    }
  }
  
  private var nftHeaderView: some View {
    HStack {
      Text(Strings.Wallet.assetsTitle)
        .font(.footnote)
        .foregroundColor(Color(.secondaryBraveLabel))
        .padding(.leading, 16)
      if nftStore.isLoadingDiscoverAssets && isNFTDiscoveryEnabled {
        ProgressView()
          .padding(.leading, 5)
      }
      Spacer()
      Picker(selection: $nftStore.displayType) {
        ForEach(NFTStore.NFTDisplayType.allCases) { type in
          Text(type.dropdownTitle)
            .foregroundColor(Color(.secondaryBraveLabel))
            .tag(type)
        }
      } label: {
        Text(nftStore.displayType.dropdownTitle)
          .font(.footnote)
          .foregroundColor(Color(.braveLabel))
      }
      filtersButton
        .padding(.trailing, 10)
      addCustomAssetButton
    }
    .textCase(nil)
    .padding(.horizontal, 10)
    .frame(maxWidth: .infinity, alignment: .leading)
  }
  
  private var addCustomAssetButton: some View {
    Button(action: {
      isShowingAddCustomNFT = true
    }) {
      Image(systemName: "plus")
    }
  }
  
  private var nftDiscoveryDescriptionText: NSAttributedString? {
    let attributedString = NSMutableAttributedString(
      string: Strings.Wallet.nftDiscoveryCalloutDescription,
      attributes: [.foregroundColor: UIColor.braveLabel, .font: UIFont.preferredFont(for: .subheadline, weight: .regular)]
    )
    
    attributedString.addAttributes([.underlineStyle: NSUnderlineStyle.single.rawValue], range: (attributedString.string as NSString).range(of: "SimpleHash")) // `SimpleHash` won't get translated
    attributedString.addAttribute(
      .link,
      value: WalletConstants.nftDiscoveryURL.absoluteString,
      range: (attributedString.string as NSString).range(of: Strings.Wallet.nftDiscoveryCalloutDescriptionLearnMore)
    )
    
    return attributedString
  }
  
  @ViewBuilder var nftGridsView: some View {
    if nftStore.displayNFTs.isEmpty {
      emptyView
        .listRowBackground(Color(.clear))
    } else {
      LazyVGrid(columns: nftGrids) {
        ForEach(nftStore.displayNFTs) { nft in
          Button(action: {
            selectedNFTViewModel = nft
          }) {
            VStack(alignment: .leading, spacing: 4) {
              nftImage(nft)
                .padding(.bottom, 8)
              Text(nft.token.nftTokenTitle)
                .font(.callout.weight(.medium))
                .foregroundColor(Color(.braveLabel))
                .multilineTextAlignment(.leading)
              if !nft.token.symbol.isEmpty {
                Text(nft.token.symbol)
                  .font(.caption)
                  .foregroundColor(Color(.secondaryBraveLabel))
                  .multilineTextAlignment(.leading)
              }
            }
          }
          .contextMenu {
            Button(action: {
              nftStore.updateNFTStatus(nft.token, visible: isHiddenNFT(nft.token), isSpam: false)
            }) {
              Label(isHiddenNFT(nft.token) ? Strings.Wallet.nftUnhide : Strings.recentSearchHide, braveSystemImage: isHiddenNFT(nft.token) ? "leo.eye.on" : "leo.eye.off")
            }
            Button(action: {
              nftStore.updateNFTStatus(nft.token, visible: isSpamNFT(nft.token), isSpam: !isSpamNFT(nft.token))
            }) {
              Label(isSpamNFT(nft.token) ? Strings.Wallet.nftUnspam : Strings.Wallet.nftMoveToSpam, braveSystemImage: "leo.disable.outline")
            }
          }
        }
      }
      VStack(spacing: 16) {
        Divider()
        editUserAssetsButton
      }
      .padding(.top, 20)
    }
  }
  
  var body: some View {
    ScrollView {
      VStack {
        nftHeaderView
          .padding(.horizontal, 8)
        nftGridsView
          .padding(.horizontal, 24)
      }
      .padding(.vertical, 24)
    }
    .background(Color(UIColor.braveGroupedBackground))
    .background(
      NavigationLink(
        isActive: Binding(
          get: { selectedNFTViewModel != nil },
          set: { if !$0 { selectedNFTViewModel = nil } }
        ),
        destination: {
          if let nftViewModel = selectedNFTViewModel {
            NFTDetailView(
              nftDetailStore: cryptoStore.nftDetailStore(for: nftViewModel.token, nftMetadata: nftViewModel.nftMetadata),
              buySendSwapDestination: buySendSwapDestination
            ) { nftMetadata in
              nftStore.updateNFTMetadataCache(for: nftViewModel.token, metadata: nftMetadata)
            }
            .onDisappear {
              cryptoStore.closeNFTDetailStore(for: nftViewModel.token)
            }
          }
        },
        label: {
          EmptyView()
        })
    )
    .background(
      WalletPromptView(
        isPresented: $isShowingNFTDiscoveryAlert,
        primaryButton: .init(
          title: Strings.Wallet.nftDiscoveryCalloutEnable,
          action: { _ in
            isNFTDiscoveryEnabled = true
            nftStore.enableNFTDiscovery()
            Preferences.Wallet.shouldShowNFTDiscoveryPermissionCallout.value = false
            isShowingNFTDiscoveryAlert = false
          }
        ),
        secondaryButton: .init(
          title: Strings.Wallet.nftDiscoveryCalloutDisable,
          action: { _ in
            isNFTDiscoveryEnabled = false
            Preferences.Wallet.shouldShowNFTDiscoveryPermissionCallout.value = false
            // don't need to setDiscovery(false) since the default value is false
            // and when nftDiscoveryEnabled() is true, this WalletPromptView won't
            // get prompt
            isShowingNFTDiscoveryAlert = false
          }
        ),
        showCloseButton: false,
        content: {
          VStack(spacing: 10) {
            Text(Strings.Wallet.nftDiscoveryCalloutTitle)
              .font(.headline.weight(.bold))
              .multilineTextAlignment(.center)
            if let attrString = nftDiscoveryDescriptionText {
              AdjustableHeightAttributedTextView(
                attributedString: attrString,
                openLink: { url in
                  if let url {
                    openWalletURL(url)
                  }
                }
              )
            }
          }
        }
      )
    )
    .sheet(isPresented: $isShowingAddCustomNFT) {
      AddCustomAssetView(
        networkStore: networkStore,
        networkSelectionStore: networkStore.openNetworkSelectionStore(mode: .formSelection),
        keyringStore: keyringStore,
        userAssetStore: nftStore.userAssetsStore,
        supportedTokenTypes: [.nft]
      ) {
        cryptoStore.updateAssets()
      }
    }
    .onAppear {
      Task {
        isNFTDiscoveryEnabled = await nftStore.isNFTDiscoveryEnabled()
        if !isNFTDiscoveryEnabled && Preferences.Wallet.shouldShowNFTDiscoveryPermissionCallout.value {
          self.isShowingNFTDiscoveryAlert = true
        }
      }
    }
  }
  
  private func isSpamNFT(_ nft: BraveWallet.BlockchainToken) -> Bool {
    if nftStore.displayType == .spam {
      return true
    } else {
      return nft.isSpam
    }
  }
  
  private func isHiddenNFT(_ nft: BraveWallet.BlockchainToken) -> Bool {
    if nftStore.displayType == .spam {
      return false
    } else {
      return !nft.visible
    }
  }
}

#if DEBUG
struct NFTView_Previews: PreviewProvider {
  static var previews: some View {
    NFTView(
      cryptoStore: .previewStore,
      keyringStore: .previewStore,
      networkStore: .previewStore,
      nftStore: CryptoStore.previewStore.nftStore
    )
  }
}
#endif
