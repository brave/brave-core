// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveUI
import DesignSystem
import SDWebImageSwiftUI
import SwiftUI

struct NFTDetailView: View {
  @ObservedObject var keyringStore: KeyringStore
  @ObservedObject var nftDetailStore: NFTDetailStore
  @Binding var walletActionDestination: WalletActionDestination?

  var onNFTMetadataRefreshed: ((BraveWallet.NftMetadata) -> Void)?

  @Environment(\.openURL) private var openWalletURL
  @Environment(\.presentationMode) @Binding private var presentationMode
  @Environment(\.horizontalSizeClass) private var horizontalSizeClass

  @State private var isPresentingRemoveAlert: Bool = false

  @ViewBuilder private var noImageView: some View {
    Text(Strings.Wallet.nftDetailImageNotAvailable)
      .foregroundColor(Color(.secondaryBraveLabel))
      .frame(maxWidth: .infinity, minHeight: 300)
  }

  @ViewBuilder private var nftLogo: some View {
    if let image = nftDetailStore.networkInfo?.nativeTokenLogoImage, !nftDetailStore.isLoading {
      Image(uiImage: image)
        .resizable()
        .frame(width: 32, height: 32)
        .overlay {
          Circle()
            .stroke(lineWidth: 2)
            .foregroundColor(Color(braveSystemName: .containerBackground))
        }
    }
  }

  @ViewBuilder private var nftImage: some View {
    NFTImageView(
      urlString: nftDetailStore.nftMetadata?.image ?? "",
      isLoading: nftDetailStore.isLoading
    ) {
      noImageView
    }
    .cornerRadius(10)
    .frame(maxWidth: .infinity, minHeight: 300)
  }

  private var isSVGImage: Bool {
    guard let nftMetadata = nftDetailStore.nftMetadata
    else { return false }
    return nftMetadata.image.hasPrefix("data:image/svg") || nftMetadata.image.hasSuffix(".svg")
  }

  var body: some View {
    Form {
      Section {
        VStack(alignment: .leading, spacing: 16) {
          nftImage
            .overlay(alignment: .topLeading) {
              if nftDetailStore.nft.isSpam {
                HStack(spacing: 4) {
                  Text(Strings.Wallet.nftSpam)
                    .padding(.vertical, 4)
                    .padding(.leading, 6)
                    .foregroundColor(Color(.braveErrorLabel))
                  Image(braveSystemName: "leo.warning.triangle-outline")
                    .padding(.vertical, 4)
                    .padding(.trailing, 6)
                    .foregroundColor(Color(.braveErrorBorder))
                }
                .font(.system(size: 13).weight(.semibold))
                .background(
                  Color(uiColor: WalletV2Design.spamNFTLabelBackground)
                    .cornerRadius(4)
                )
                .padding(12)
              }
            }
            .overlay(alignment: .bottomTrailing) {
              ZStack {
                if let owner = nftDetailStore.owner {
                  Blockie(address: owner.address)
                    .overlay(
                      RoundedRectangle(cornerRadius: 4)
                        .stroke(lineWidth: 2)
                        .foregroundColor(Color(braveSystemName: .containerBackground))
                    )
                    .frame(width: 32, height: 32)
                    .zIndex(1)
                    .offset(x: -28)
                }
                nftLogo
              }
              .offset(y: 16)
            }
          VStack(alignment: .leading, spacing: 8) {
            Text(nftDetailStore.nft.nftDetailTitle)
              .font(.title3.weight(.semibold))
              .foregroundColor(Color(.braveLabel))
            Text(nftDetailStore.nft.name)
              .foregroundColor(Color(.secondaryBraveLabel))
          }
        }
        .transaction { transaction in
          transaction.animation = nil
          transaction.disablesAnimations = true
        }
        .listRowInsets(.zero)
        .listRowBackground(Color.clear)
      }
      Section {
        List {
          if let owner = nftDetailStore.owner {
            NFTDetailRow(title: Strings.Wallet.nftDetailOwnedBy) {
              AddressView(address: owner.address) {
                HStack {
                  Text(owner.name)
                    .foregroundColor(Color(.braveBlurpleTint))
                  Text(owner.address.truncatedAddress)
                    .foregroundColor(Color(.braveLabel))
                }
                .font(.subheadline)
              }
            }
          }
          if nftDetailStore.nft.isErc721,
            let tokenId = Int(nftDetailStore.nft.tokenId.removingHexPrefix, radix: 16)
          {
            NFTDetailRow(title: Strings.Wallet.nftDetailTokenID) {
              Text("\(tokenId)")
                .font(.subheadline)
                .foregroundColor(Color(.braveLabel))
            }
          }
          NFTDetailRow(
            title: nftDetailStore.nft.isErc721
              ? Strings.Wallet.contractAddressAccessibilityLabel : Strings.Wallet.tokenMintAddress
          ) {
            Button {
              if let url = nftDetailStore.networkInfo?.nftBlockExplorerURL(nftDetailStore.nft) {
                openWalletURL(url)
              }
            } label: {
              HStack {
                Text(nftDetailStore.nft.contractAddress.truncatedAddress)
                Image(systemName: "arrow.up.forward.square")
              }
              .font(.subheadline)
              .foregroundColor(Color(.braveBlurpleTint))
            }
          }
          if let networkInfo = nftDetailStore.networkInfo {
            NFTDetailRow(title: Strings.Wallet.nftDetailBlockchain) {
              Text(networkInfo.chainName)
                .font(.subheadline)
                .foregroundColor(Color(.braveLabel))
            }
          }
          NFTDetailRow(title: Strings.Wallet.nftDetailTokenStandard) {
            Text(
              nftDetailStore.nft.isErc721
                ? Strings.Wallet.nftDetailERC721 : Strings.Wallet.nftDetailSPL
            )
            .font(.subheadline)
            .foregroundColor(Color(.braveLabel))
          }
        }
        .listRowBackground(Color(.secondaryBraveGroupedBackground))
      } header: {
        Text(Strings.Wallet.nftDetailOverview)
      }
      if let nftMetadata = nftDetailStore.nftMetadata,
        !nftMetadata.desc.isEmpty
      {
        Section {
          Text(nftMetadata.desc)
            .font(.subheadline)
            .foregroundColor(Color(.braveLabel))
            .listRowBackground(Color(.secondaryBraveGroupedBackground))
            .listRowInsets(
              horizontalSizeClass == .regular
                ? EdgeInsets(top: 16, leading: 16, bottom: 16, trailing: 16) : nil
            )
        } header: {
          Text(Strings.Wallet.nftDetailDescription)
        }
      }
      if let attributes = nftDetailStore.nftMetadata?.attributes {
        Section {
          List {
            ForEach(attributes) { attribute in
              NFTDetailRow(title: attribute.traitType) {
                Text(attribute.value)
                  .font(.subheadline)
                  .foregroundColor(Color(.braveLabel))
              }
            }
          }
          .listRowBackground(Color(.secondaryBraveGroupedBackground))
        } header: {
          Text(Strings.Wallet.nftDetailProperties)
        }
      }
    }
    .listBackgroundColor(Color(.braveGroupedBackground))
    .onChange(
      of: nftDetailStore.nftMetadata,
      perform: { newValue in
        if let newMetadata = newValue {
          onNFTMetadataRefreshed?(newMetadata)
        }
      }
    )
    .onChange(
      of: keyringStore.isWalletLocked,
      perform: { isLocked in
        guard isLocked else { return }
        if isPresentingRemoveAlert {
          isPresentingRemoveAlert = false
        }
      }
    )
    .background(Color(UIColor.braveGroupedBackground).ignoresSafeArea())
    .navigationBarTitle(nftDetailStore.nft.nftDetailTitle)
    .toolbar {
      ToolbarItemGroup(placement: .navigationBarTrailing) {
        Menu {
          if nftDetailStore.nft.visible {
            Button {
              walletActionDestination = WalletActionDestination(
                kind: .send,
                initialToken: nftDetailStore.nft
              )
            } label: {
              Label(Strings.Wallet.nftDetailSendNFTButtonTitle, braveSystemImage: "leo.send")
            }
            .buttonStyle(BraveFilledButtonStyle(size: .large))
          }
          Button {
            if nftDetailStore.nft.visible {  // a collected visible NFT, mark as hidden
              Task { @MainActor in
                await nftDetailStore.updateNFTStatus(
                  visible: false,
                  isSpam: false,
                  isDeletedByUser: false
                )
              }
            } else {  // either a hidden NFT or a junk NFT, mark as visible
              Task { @MainActor in
                await nftDetailStore.updateNFTStatus(
                  visible: true,
                  isSpam: false,
                  isDeletedByUser: false
                )
              }
            }
          } label: {
            if nftDetailStore.nft.visible {  // a collected visible NFT
              Label(Strings.recentSearchHide, braveSystemImage: "leo.eye.off")
            } else if nftDetailStore.nft.isSpam {  // a spam NFT
              Label(Strings.Wallet.nftUnspam, braveSystemImage: "leo.disable.outline")
            } else {  // a hidden but not spam NFT
              Label(Strings.Wallet.nftUnhide, braveSystemImage: "leo.eye.on")
            }
          }
          Button {
            isPresentingRemoveAlert = true
          } label: {
            Label(Strings.Wallet.nftRemoveFromWallet, braveSystemImage: "leo.trash")
          }
        } label: {
          Label(
            Strings.Wallet.otherWalletActionsAccessibilityTitle,
            braveSystemImage: "leo.more.horizontal"
          )
          .labelStyle(.iconOnly)
          .foregroundColor(Color(.braveBlurpleTint))
        }
      }
    }
    .background(
      WalletPromptView(
        isPresented: $isPresentingRemoveAlert,
        primaryButton: .init(
          title: Strings.Wallet.manageSiteConnectionsConfirmAlertRemove,
          action: { _ in
            Task { @MainActor in
              isPresentingRemoveAlert = false
              await nftDetailStore.updateNFTStatus(
                visible: false,
                isSpam: nftDetailStore.nft.isSpam,
                isDeletedByUser: true
              )
              presentationMode.dismiss()
            }
          }
        ),
        secondaryButton: .init(
          title: Strings.CancelString,
          action: { _ in
            isPresentingRemoveAlert = false
          }
        ),
        showCloseButton: false,
        content: {
          VStack(spacing: 16) {
            Text(Strings.Wallet.nftRemoveFromWalletAlertTitle)
              .font(.headline)
              .foregroundColor(Color(.bravePrimary))
            Text(Strings.Wallet.nftRemoveFromWalletAlertDescription)
              .font(.footnote)
              .foregroundStyle(Color(.secondaryBraveLabel))
          }
          .padding(.bottom, 28)
        }
      )
    )
  }
}

struct NFTDetailRow<ValueContent: View>: View {
  var title: String
  var valueContent: () -> ValueContent

  init(
    title: String,
    @ViewBuilder valueContent: @escaping () -> ValueContent
  ) {
    self.title = title
    self.valueContent = valueContent
  }
  var body: some View {
    HStack {
      Text(title)
        .font(.subheadline)
        .foregroundColor(Color(.secondaryLabel))
      Spacer()
      valueContent()
        .multilineTextAlignment(.trailing)
    }
  }
}
