// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import SwiftUI
import Shared
import BraveUI
import BigNumber

struct SendTokenView: View {
  @ObservedObject var keyringStore: KeyringStore
  @ObservedObject var networkStore: NetworkStore
  @ObservedObject var sendTokenStore: SendTokenStore
  
  @Environment(\.presentationMode) @Binding private var presentationMode
  
  @State private var amountInput = ""
  @State private var isShowingScanner = false
  @State private var isShowingError = false
  
  @ScaledMetric private var length: CGFloat = 16.0
  
  private var isSendDisabled: Bool {
    guard let sendAmount = BDouble(amountInput),
          let balance = sendTokenStore.selectedSendTokenBalance,
          let token = sendTokenStore.selectedSendToken,
          !sendTokenStore.isMakingTx else {
      return true
    }
    
    let weiFormatter = WeiFormatter(decimalFormatStyle: .decimals(precision: Int(token.decimals)))
    if weiFormatter.weiString(from: amountInput, radix: .decimal, decimals: Int(token.decimals)) == nil {
      return true
    }
    
    return sendAmount == 0
    || sendAmount > balance
    || amountInput.isEmpty
    || sendTokenStore.addressError != nil
  }
  
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
          NavigationLink(destination:
                          SendTokenSearchView(sendTokenStore: sendTokenStore,
                                              network: networkStore.selectedChain
                                             )
          ) {
            HStack {
              if let token = sendTokenStore.selectedSendToken {
                AssetIconView(token: token, length: 26)
              }
              Text(sendTokenStore.selectedSendToken?.symbol ?? "")
                .font(.title3.weight(.semibold))
                .foregroundColor(Color(.braveLabel))
              Spacer()
              Text(String(format: "%.04f", sendTokenStore.selectedSendTokenBalance ?? 0))
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
            amountInput = "\((sendTokenStore.selectedSendTokenBalance ?? 0) * amount.rawValue)"
          })
          .listRowInsets(.zero)
          .padding(.bottom, 8)
        ) {
          TextField(String.localizedStringWithFormat(Strings.Wallet.amountInCurrency,
                                                     sendTokenStore.selectedSendToken?.symbol ?? ""),
                    text: $amountInput
          )
            .keyboardType(.decimalPad)
        }
        .listRowBackground(Color(.secondaryBraveGroupedBackground))
        Section(
          header: WalletListHeaderView(title: Text(Strings.Wallet.sendCryptoToTitle)),
          footer: Group {
            if let error = sendTokenStore.addressError {
              HStack(alignment: .firstTextBaseline, spacing: 4) {
                Image(systemName: "exclamationmark.circle.fill")
                Text(error.localizedDescription)
                  .fixedSize(horizontal: false, vertical: true)
                  .animation(nil, value: error.localizedDescription)
              }
              .transition(
                .asymmetric(
                  insertion: .opacity.animation(.default),
                  removal: .identity
                )
              )
              .font(.footnote)
              .foregroundColor(Color(.braveErrorLabel))
              .padding(.bottom)
            }
          }
        ) {
          HStack(spacing: 14.0) {
            TextField(Strings.Wallet.sendCryptoAddressPlaceholder, text: $sendTokenStore.sendAddress)
            Button(action: {
              if let string = UIPasteboard.general.string {
                sendTokenStore.sendAddress = string
              }
            }) {
              Label(Strings.Wallet.pasteFromPasteboard, image: "brave.clipboard")
                .labelStyle(.iconOnly)
                .foregroundColor(Color(.primaryButtonTint))
                .font(.body)
            }
            .buttonStyle(PlainButtonStyle())
            Button(action: {
              isShowingScanner = true
            }) {
              Label(Strings.Wallet.scanQRCodeAccessibilityLabel, image: "brave.qr-code")
                .labelStyle(.iconOnly)
                .foregroundColor(Color(.primaryButtonTint))
                .font(.body)
            }
            .buttonStyle(PlainButtonStyle())
          }
        }
        .listRowBackground(Color(.secondaryBraveGroupedBackground))
        Section(
          header:
            WalletLoadingButton(
              isLoading: sendTokenStore.isMakingTx,
              action: {
                sendTokenStore.sendToken(amount: amountInput) { success in
                  isShowingError = !success
                }
              },
              label: {
                Text(Strings.Wallet.sendCryptoSendButtonTitle)
              }
            )
            .buttonStyle(BraveFilledButtonStyle(size: .normal))
            .disabled(isSendDisabled)
            .frame(maxWidth: .infinity)
            .resetListHeaderStyle()
            .listRowBackground(Color(.clear))
        ) {
        }
        .listRowBackground(Color(.secondaryBraveGroupedBackground))
      }
      .alert(isPresented: $isShowingError) {
        Alert(
          title: Text(""),
          message: Text(Strings.Wallet.sendCryptoSendError),
          dismissButton: .cancel(Text(Strings.OKString))
        )
      }
      .sheet(isPresented: $isShowingScanner) {
        AddressQRCodeScannerView(address: $sendTokenStore.sendAddress)
      }
      .navigationTitle(Strings.Wallet.send)
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
    }
    .onAppear {
      sendTokenStore.fetchAssets()
    }
  }
}

#if DEBUG
struct SendTokenView_Previews: PreviewProvider {
    static var previews: some View {
      SendTokenView(
        keyringStore: .previewStore,
        networkStore: .previewStore,
        sendTokenStore: .previewStore
      )
        .previewColorSchemes()
    }
}
#endif
