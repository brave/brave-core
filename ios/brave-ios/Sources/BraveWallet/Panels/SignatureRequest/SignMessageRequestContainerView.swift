// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveStrings
import DesignSystem
import SwiftUI

/// View for displaying an array of `SignMessageRequest`s`
struct SignMessageRequestContainerView: View {

  @ObservedObject var store: SignMessageRequestStore
  @ObservedObject var keyringStore: KeyringStore
  var cryptoStore: CryptoStore
  @ObservedObject var networkStore: NetworkStore
  var onDismiss: () -> Void

  /// The network for the current request
  private var currentRequestNetwork: BraveWallet.NetworkInfo? {
    networkStore.allChains.first(where: { $0.chainId == store.currentRequest.request.chainId })
  }

  var body: some View {
    Group {
      if let ethSiweData = store.currentRequest.request.signData.ethSiweData {
        SignInWithEthereumView(
          account: store.currentRequest.account,
          originInfo: store.currentRequest.request.originInfo,
          message: ethSiweData,
          action: handleAction(approved:)
        )
      } else if let cowSwapOrder = store.currentRequest.request.signData.ethSignTypedData?.meta?
        .cowSwapOrder
      {
        SaferSignMessageRequestContainerView(
          account: store.currentRequest.account,
          request: store.currentRequest.request,
          network: currentRequestNetwork,
          requestIndex: store.requestIndex,
          requestCount: store.requests.count,
          receiverAddress: cowSwapOrder.receiver,
          namedReceiverAddress: NamedAddresses.name(
            for: cowSwapOrder.receiver,
            accounts: keyringStore.allAccounts
          ),
          cowSwapOrder: cowSwapOrder,
          ethSwapDetails: store.ethSwapDetails[store.currentRequest.id],
          needPilcrowFormatted: $store.needPilcrowFormatted,
          showOrignalMessage: $store.showOrignalMessage,
          nextTapped: store.next,
          action: handleAction(approved:)
        )
      } else {  // ethSignTypedData, ethStandardSignData, solanaSignData
        SignMessageRequestView(
          account: store.currentRequest.account,
          request: store.currentRequest.request,
          network: currentRequestNetwork,
          requestIndex: store.requestIndex,
          requestCount: store.requests.count,
          needPilcrowFormatted: $store.needPilcrowFormatted,
          showOrignalMessage: $store.showOrignalMessage,
          nextTapped: store.next,
          action: handleAction(approved:)
        )
      }
    }
    .navigationBarTitleDisplayMode(.inline)
    .frame(maxWidth: .infinity, maxHeight: .infinity)
    .background(Color(braveSystemName: .containerHighlight))
    .onAppear {
      store.update()
    }
  }

  private func handleAction(approved: Bool) {
    cryptoStore.handleWebpageRequestResponse(
      .signMessage(approved: approved, id: store.currentRequest.id)
    )
    if store.requests.count <= 1 {
      onDismiss()
    }
  }
}
