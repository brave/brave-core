// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import SwiftUI
import Strings
import BraveCore
import BraveShared
import DesignSystem

struct SignatureRequestView: View {
  var requests: [BraveWallet.SignMessageRequest]
  @ObservedObject var keyringStore: KeyringStore
  var cryptoStore: CryptoStore
  
  var onDismiss: () -> Void

  @State private var requestIndex: Int = 0
  @Environment(\.sizeCategory) private var sizeCategory
  @Environment(\.presentationMode) @Binding private var presentationMode
  @ScaledMetric private var blockieSize = 54
  private let maxBlockieSize: CGFloat = 108
  
  private var currentRequest: BraveWallet.SignMessageRequest {
    requests[requestIndex]
  }
  
  private var account: BraveWallet.AccountInfo {
    keyringStore.allAccounts.first(where: { $0.address == currentRequest.address }) ?? keyringStore.selectedAccount
  }
  
  init(
    requests: [BraveWallet.SignMessageRequest],
    keyringStore: KeyringStore,
    cryptoStore: CryptoStore,
    onDismiss: @escaping () -> Void
  ) {
    assert(!requests.isEmpty)
    self.requests = requests
    self.keyringStore = keyringStore
    self.cryptoStore = cryptoStore
    self.onDismiss = onDismiss
  }
  
  var body: some View {
    ScrollView(.vertical) {
      VStack {
        if requests.count > 1 {
          HStack {
            Spacer()
            Text(String.localizedStringWithFormat(Strings.Wallet.transactionCount, requestIndex + 1, requests.count))
              .fontWeight(.semibold)
            Button(action: next) {
              Text(Strings.Wallet.next)
                .fontWeight(.semibold)
                .foregroundColor(Color(.braveBlurpleTint))
            }
          }
        }
        VStack(spacing: 12) {
          VStack(spacing: 8) {
            Blockie(address: account.address)
              .frame(width: min(blockieSize, maxBlockieSize), height: min(blockieSize, maxBlockieSize))
            AddressView(address: account.address) {
              VStack(spacing: 4) {
                Text(account.name)
                  .font(.subheadline.weight(.semibold))
                  .foregroundColor(Color(.braveLabel))
                Text(account.address.truncatedAddress)
                  .font(.subheadline.weight(.semibold))
                  .foregroundColor(Color(.secondaryBraveLabel))
              }
            }
            Text(urlOrigin: currentRequest.originInfo.origin)
              .font(.caption)
              .foregroundColor(Color(.braveLabel))
              .multilineTextAlignment(.center)
          }
          .accessibilityElement(children: .combine)
          Text(Strings.Wallet.signatureRequestSubtitle)
            .font(.headline)
            .foregroundColor(Color(.bravePrimary))
        }
        .padding(.vertical, 32)
        StaticTextView(text: currentRequest.message, isMonospaced: false)
          .frame(maxWidth: .infinity)
          .frame(height: 200)
          .background(Color(.tertiaryBraveGroupedBackground))
          .clipShape(RoundedRectangle(cornerRadius: 5, style: .continuous))
          .padding()
        .background(
          Color(.secondaryBraveGroupedBackground)
        )
        .clipShape(RoundedRectangle(cornerRadius: 10, style: .continuous))
        buttonsContainer
          .padding(.top)
          .opacity(sizeCategory.isAccessibilityCategory ? 0 : 1)
          .accessibility(hidden: sizeCategory.isAccessibilityCategory)
      }
      .padding()
    }
    .overlay(
      Group {
        if sizeCategory.isAccessibilityCategory {
          buttonsContainer
            .frame(maxWidth: .infinity)
            .padding(.top)
            .background(
              LinearGradient(
                stops: [
                  .init(color: Color(.braveGroupedBackground).opacity(0), location: 0),
                  .init(color: Color(.braveGroupedBackground).opacity(1), location: 0.05),
                  .init(color: Color(.braveGroupedBackground).opacity(1), location: 1),
                ],
                startPoint: .top,
                endPoint: .bottom
              )
              .ignoresSafeArea()
              .allowsHitTesting(false)
            )
        }
      },
      alignment: .bottom
    )
    .frame(maxWidth: .infinity)
    .navigationTitle(Strings.Wallet.signatureRequestTitle)
    .navigationBarTitleDisplayMode(.inline)
    .foregroundColor(Color(.braveLabel))
    .background(Color(.braveGroupedBackground).edgesIgnoringSafeArea(.all))
    .toolbar {
      ToolbarItemGroup(placement: .cancellationAction) {
        Button(action: { presentationMode.dismiss() }) {
          Text(Strings.cancelButtonTitle)
            .foregroundColor(Color(.braveOrange))
        }
      }
    }
  }
  
  private var isButtonsDisabled: Bool {
    requestIndex != 0
  }
  
  @ViewBuilder private var buttonsContainer: some View {
    if sizeCategory.isAccessibilityCategory {
      VStack {
        buttons
      }
    } else {
      HStack {
        buttons
      }
    }
  }
  
  @ViewBuilder private var buttons: some View {
    Button(action: { // cancel
      cryptoStore.handleWebpageRequestResponse(.signMessage(approved: false, id: currentRequest.id))
      if requests.count == 1 {
        onDismiss()
      }
    }) {
      Label(Strings.cancelButtonTitle, systemImage: "xmark")
        .imageScale(.large)
    }
    .buttonStyle(BraveOutlineButtonStyle(size: .large))
    .disabled(isButtonsDisabled)
    Button(action: { // approve
      cryptoStore.handleWebpageRequestResponse(.signMessage(approved: true, id: currentRequest.id))
      if requests.count == 1 {
        onDismiss()
      }
    }) {
      Label(Strings.Wallet.sign, braveSystemImage: "brave.key")
        .imageScale(.large)
    }
    .buttonStyle(BraveFilledButtonStyle(size: .large))
    .disabled(isButtonsDisabled)
  }
  
  private func next() {
    if requestIndex + 1 < requests.count {
      requestIndex += 1
    } else {
      requestIndex = 0
    }
  }
}

#if DEBUG
struct SignatureRequestView_Previews: PreviewProvider {
  static var previews: some View {
    SignatureRequestView(
      requests: [.previewRequest],
      keyringStore: .previewStoreWithWalletCreated,
      cryptoStore: .previewStore,
      onDismiss: { }
    )
  }
}
#endif
