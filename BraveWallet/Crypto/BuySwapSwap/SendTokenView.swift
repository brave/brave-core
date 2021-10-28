// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import SwiftUI
import struct Shared.Strings
import BraveUI

struct SendTokenView: View {
  @ObservedObject var keyringStore: KeyringStore
  @ObservedObject var networkStore: NetworkStore
  @ObservedObject var sendTokenStore: SendTokenStore
  
  @Environment(\.presentationMode) @Binding private var presentationMode
  
  @State private var amountInput = ""
  @State private var sendAddress = ""
  @State private var isShowingScanner = false
  
  @ScaledMetric private var length: CGFloat = 16.0
  
  var body: some View {
    NavigationView {
      Form {
        Section(
          header: AccountPicker(
            keyringStore: keyringStore,
            networkStore: networkStore
          )
            .listRowBackground(Color.clear)
            .resetListHeaderStyle()
            .padding(.top)
            .padding(.bottom, -16) // Get it a bit closer
        ) {
        }
        Section(
          header: WalletListHeaderView(title: Text(Strings.Wallet.sendCryptoFromTitle))
        ) {
          NavigationLink(destination: SendTokenSearchView(sendTokenStore: sendTokenStore)) {
            HStack {
              if let token = sendTokenStore.selectedSendToken {
                AssetIconView(token: token, length: 26)
              }
              Text(sendTokenStore.selectedSendToken?.symbol ?? "")
                .font(.title3.weight(.semibold))
                .foregroundColor(Color(.braveLabel))
              Spacer()
              Text(sendTokenStore.selectedSendTokenBalance ?? "")
                .font(.title3.weight(.semibold))
                .foregroundColor(Color(.braveLabel))
            }
            .padding(.vertical, 8)
          }
        }
        .listRowBackground(Color(.secondaryBraveGroupedBackground))
        Section(
          header:
            WalletListHeaderView(
              title: Text(String.localizedStringWithFormat(Strings.Wallet.sendCryptoAmountTitle,
                                                          sendTokenStore.selectedSendToken?.symbol ?? "")
                        )
            ),
          footer: ShortcutAmountGrid(action: { amount in
            // TODO: compute using `sendTokenStore.selectedSendTokenBalance` and `amount` if there is one and update `amountInput`
          })
          .listRowInsets(.zero)
          .padding(.bottom, 8)
        ) {
          TextField(String.localizedStringWithFormat(Strings.Wallet.sendCryptoAmountPlaceholder,
                                                     sendTokenStore.selectedSendToken?.symbol ?? ""),
                    text: $amountInput
          )
            .keyboardType(.decimalPad)
        }
        .listRowBackground(Color(.secondaryBraveGroupedBackground))
        Section(
          header: WalletListHeaderView(title: Text(Strings.Wallet.sendCryptoToTitle))
        ) {
          HStack(spacing: 14.0) {
            TextField(Strings.Wallet.sendCryptoAddressPlaceholder, text: $sendAddress)
            Button(action: {
              if let string = UIPasteboard.general.string {
                sendAddress = string
              }
            }) {
              Image("brave.clipboard")
                .renderingMode(.template)
                .foregroundColor(Color(.primaryButtonTint))
                .font(.body)
            }
            .buttonStyle(PlainButtonStyle())
            Button(action: {
              isShowingScanner = true
            }) {
              Image("brave.qr-code")
                .renderingMode(.template)
                .foregroundColor(Color(.primaryButtonTint))
                .font(.body)
            }
            .buttonStyle(PlainButtonStyle())
          }
        }
        .listRowBackground(Color(.secondaryBraveGroupedBackground))
        Section(
          header:
            Button(action: {}) {
              Text(Strings.Wallet.sendCryptoPreviewButtonTitle)
            }
            .buttonStyle(BraveFilledButtonStyle(size: .normal))
            .frame(maxWidth: .infinity)
            .resetListHeaderStyle()
            .listRowBackground(Color(.clear))
        ) {
        }
        .listRowBackground(Color(.secondaryBraveGroupedBackground))
      }
      .sheet(isPresented: $isShowingScanner) {
        AddressQRCodeScannerView(address: $sendAddress)
      }
      .navigationTitle(Strings.Wallet.send)
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
    }
    .onAppear {
      sendTokenStore.fetchAssets()
    }
  }
}

struct SendTokenView_Previews: PreviewProvider {
    static var previews: some View {
      SendTokenView(
        keyringStore: .previewStoreWithWalletCreated,
        networkStore: .previewStore,
        sendTokenStore: .previewStore
      )
        .previewColorSchemes()
    }
}
