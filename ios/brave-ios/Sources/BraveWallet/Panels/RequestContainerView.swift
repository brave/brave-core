// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveUI
import Foundation
import SwiftUI

/// A container to present when a webpage wants to present some request to the user such as transaction
/// confirmations, adding networks, switch networks, add tokens, sign message, etc.
struct RequestContainerView<DismissContent: ToolbarContent>: View {
  @ObservedObject var keyringStore: KeyringStore
  @ObservedObject var cryptoStore: CryptoStore
  var toolbarDismissContent: DismissContent

  var onDismiss: () -> Void

  var body: some View {
    NavigationView {
      Group {
        if let pendingRequest = cryptoStore.pendingRequest {
          switch pendingRequest {
          case .transactions:
            TransactionConfirmationView(
              confirmationStore: cryptoStore.openConfirmationStore(),
              networkStore: cryptoStore.networkStore,
              keyringStore: keyringStore,
              onDismiss: onDismiss
            )
          case .addSuggestedToken(let request):
            AddSuggestedTokenView(
              token: request.token,
              originInfo: request.origin,
              cryptoStore: cryptoStore,
              onDismiss: onDismiss
            )
          case .switchChain(let request):
            SuggestedNetworkView(
              mode: .switchNetworks(request),
              cryptoStore: cryptoStore,
              keyringStore: keyringStore,
              networkStore: cryptoStore.networkStore,
              onDismiss: onDismiss
            )
          case .addChain(let request):
            SuggestedNetworkView(
              mode: .addNetwork(request),
              cryptoStore: cryptoStore,
              keyringStore: keyringStore,
              networkStore: cryptoStore.networkStore,
              onDismiss: onDismiss
            )
          case .signMessage(let requests):
            SignMessageRequestContainerView(
              store: cryptoStore.signMessageRequestStore(for: requests),
              keyringStore: keyringStore,
              cryptoStore: cryptoStore,
              networkStore: cryptoStore.networkStore,
              onDismiss: onDismiss
            )
          case .signMessageError(let signMessageErrors):
            SignMessageErrorView(
              signMessageErrors: signMessageErrors,
              cryptoStore: cryptoStore
            )
          case .getEncryptionPublicKey(let request):
            EncryptionView(
              request: .getEncryptionPublicKey(request),
              cryptoStore: cryptoStore,
              keyringStore: keyringStore,
              onDismiss: onDismiss
            )
          case .decrypt(let request):
            EncryptionView(
              request: .decrypt(request),
              cryptoStore: cryptoStore,
              keyringStore: keyringStore,
              onDismiss: onDismiss
            )
          case .signSolTransactions(let requests):
            SignTransactionView(
              keyringStore: keyringStore,
              networkStore: cryptoStore.networkStore,
              requests: requests,
              cryptoStore: cryptoStore,
              onDismiss: onDismiss
            )
          }
        }
      }
      .toolbar {
        toolbarDismissContent
      }
    }
    .navigationViewStyle(.stack)
    .onDisappear {
      // `onDisappear` on individual views will trigger for navigation pushes.
      // Close stores when navigation covers manual dismiss & onDismiss() cases.
      cryptoStore.closeConfirmationStore()
      cryptoStore.closeSignMessageRequestStore()
    }
  }
}
