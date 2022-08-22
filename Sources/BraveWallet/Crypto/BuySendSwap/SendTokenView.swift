// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import SwiftUI
import Strings
import DesignSystem
import BigNumber

struct SendTokenView: View {
  @ObservedObject var keyringStore: KeyringStore
  @ObservedObject var networkStore: NetworkStore
  @ObservedObject var sendTokenStore: SendTokenStore

  @State private var isShowingScanner = false
  @State private var isShowingError = false

  @ScaledMetric private var length: CGFloat = 16.0
  
  var completion: ((_ success: Bool) -> Void)?
  var onDismiss: () -> Void

  private var isSendDisabled: Bool {
    guard let sendAmount = BDouble(sendTokenStore.sendAmount.normalizedDecimals),
      let balance = sendTokenStore.selectedSendTokenBalance,
      let token = sendTokenStore.selectedSendToken,
      !sendTokenStore.isMakingTx
    else {
      return true
    }

    let weiFormatter = WeiFormatter(decimalFormatStyle: .decimals(precision: Int(token.decimals)))
    if weiFormatter.weiString(from: sendTokenStore.sendAmount.normalizedDecimals, radix: .decimal, decimals: Int(token.decimals)) == nil {
      return true
    }

    return sendAmount == 0 || sendAmount > balance || sendTokenStore.sendAmount.isEmpty || sendTokenStore.sendAddress.isEmpty || (sendTokenStore.addressError != nil && sendTokenStore.addressError != .missingChecksum)
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
          .padding(.bottom, -16)  // Get it a bit closer
        ) {
        }
        Section(
          header: WalletListHeaderView(title: Text(Strings.Wallet.sendCryptoFromTitle))
        ) {
          NavigationLink(
            destination:
              SendTokenSearchView(
                sendTokenStore: sendTokenStore,
                network: networkStore.selectedChain
              )
          ) {
            HStack {
              if let token = sendTokenStore.selectedSendToken {
                AssetIconView(
                  token: token,
                  network: networkStore.selectedChain,
                  length: 26
                )
              }
              Text(sendTokenStore.selectedSendToken?.symbol ?? "")
                .font(.title3.weight(.semibold))
                .foregroundColor(Color(.braveLabel))
              Spacer()
              Text(sendTokenStore.selectedSendTokenBalance?.decimalDescription ?? "0.0000")
                .font(.title3.weight(.semibold))
                .foregroundColor(Color(.braveLabel))
            }
            .padding(.vertical, 8)
          }
          .listRowBackground(Color(.secondaryBraveGroupedBackground))
        }
        Section(
          header:
            WalletListHeaderView(
              title: Text(
                String.localizedStringWithFormat(
                  Strings.Wallet.sendCryptoAmountTitle,
                  sendTokenStore.selectedSendToken?.symbol ?? "")
              )
            ),
          footer: ShortcutAmountGrid(action: { amount in
            sendTokenStore.suggestedAmountTapped(amount)
          })
          .listRowInsets(.zero)
          .padding(.bottom, 8)
        ) {
          TextField(
            String.localizedStringWithFormat(
              Strings.Wallet.amountInCurrency,
              sendTokenStore.selectedSendToken?.symbol ?? ""),
            text: $sendTokenStore.sendAmount
          )
          .keyboardType(.decimalPad)
          .listRowBackground(Color(.secondaryBraveGroupedBackground))
        }
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
            TextField(Strings.Wallet.sendToCryptoAddressPlaceholder, text: $sendTokenStore.sendAddress)
            Button(action: {
              if let string = UIPasteboard.general.string {
                sendTokenStore.sendAddress = string
              }
            }) {
              Label(Strings.Wallet.pasteFromPasteboard, braveSystemImage: "brave.clipboard")
                .labelStyle(.iconOnly)
                .foregroundColor(Color(.primaryButtonTint))
                .font(.body)
            }
            .buttonStyle(PlainButtonStyle())
            Button(action: {
              isShowingScanner = true
            }) {
              Label(Strings.Wallet.scanQRCodeAccessibilityLabel, braveSystemImage: "brave.qr-code")
                .labelStyle(.iconOnly)
                .foregroundColor(Color(.primaryButtonTint))
                .font(.body)
            }
            .buttonStyle(PlainButtonStyle())
          }
          .listRowBackground(Color(.secondaryBraveGroupedBackground))
        }
        Section(
          header:
            WalletLoadingButton(
              isLoading: sendTokenStore.isMakingTx,
              action: {
                sendTokenStore.sendToken(
                  amount: sendTokenStore.sendAmount
                ) { success, _ in
                  isShowingError = !success
                  completion?(success)
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
          Button(action: { onDismiss() }) {
            Text(Strings.cancelButtonTitle)
              .foregroundColor(Color(.braveOrange))
          }
        }
      }
    }
    .onAppear {
      sendTokenStore.fetchAssets()
    }
    .navigationViewStyle(.stack)
  }
}

#if DEBUG
struct SendTokenView_Previews: PreviewProvider {
  static var previews: some View {
    SendTokenView(
      keyringStore: .previewStore,
      networkStore: .previewStore,
      sendTokenStore: .previewStore,
      onDismiss: {}
    )
    .previewColorSchemes()
  }
}
#endif
