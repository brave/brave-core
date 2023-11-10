// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import SwiftUI
import BraveStrings
import BraveCore
import DesignSystem

/// View for displaying an array of `SignMessageRequest`s`
struct SignMessageRequestContainerView: View {

  var requests: [BraveWallet.SignMessageRequest]
  @ObservedObject var keyringStore: KeyringStore
  var cryptoStore: CryptoStore
  @ObservedObject var networkStore: NetworkStore
  var onDismiss: () -> Void

  @State private var requestIndex: Int = 0
  
  /// A map between request index and a boolean value indicates this request message needs pilcrow formating
  @State private var needPilcrowFormatted: [Int32: Bool] = [0: false]
  /// A map between request index and a boolean value indicates this request message is displayed as
  /// its original content
  @State private var showOrignalMessage: [Int32: Bool] = [0: true]
    
  /// The current request
  private var currentRequest: BraveWallet.SignMessageRequest {
    requests[requestIndex]
  }
  
  /// The account for the current request
  private var currentRequestAccount: BraveWallet.AccountInfo {
    keyringStore.allAccounts.first(where: { $0.address == currentRequest.accountId.address }) ?? keyringStore.selectedAccount
  }
  
  /// The network for the current request
  private var currentRequestNetwork: BraveWallet.NetworkInfo? {
    networkStore.allChains.first(where: { $0.chainId == currentRequest.chainId })
  }
  
  var body: some View {
    Group {
      if let ethSiweData = currentRequest.signData.ethSiweData {
        SignInWithEthereumView(
          account: currentRequestAccount,
          originInfo: currentRequest.originInfo,
          message: ethSiweData,
          action: handleAction(approved:)
        )
      } else { // ethSignTypedData, ethStandardSignData, solanaSignData
        SignMessageRequestView(
          account: currentRequestAccount,
          request: currentRequest,
          network: currentRequestNetwork,
          requestIndex: requestIndex,
          requestCount: requests.count,
          needPilcrowFormatted: $needPilcrowFormatted,
          showOrignalMessage: $showOrignalMessage,
          nextTapped: next,
          action: handleAction(approved:)
        )
      }
    }
    .navigationBarTitleDisplayMode(.inline)
    .frame(maxWidth: .infinity, maxHeight: .infinity)
    .background(Color(braveSystemName: .containerHighlight))
  }
  
  /// Advance to the next (or first if displaying the last) sign message request.
  func next() {
    if requestIndex + 1 < requests.count {
      if let nextRequestId = requests[safe: requestIndex + 1]?.id,
         showOrignalMessage[nextRequestId] == nil {
        // if we have not previously assigned a `showOriginalMessage`
        // value for the next request, assign it the default value now.
        showOrignalMessage[nextRequestId] = true
      }
      requestIndex = requestIndex + 1
    } else {
      requestIndex = 0
    }
  }
  
  private func handleAction(approved: Bool) {
    cryptoStore.handleWebpageRequestResponse(.signMessage(approved: approved, id: currentRequest.id))
    if requests.count <= 1 {
      onDismiss()
    }
  }
}
