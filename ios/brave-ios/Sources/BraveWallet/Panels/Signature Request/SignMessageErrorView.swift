// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveStrings
import DesignSystem
import SwiftUI

struct SignMessageErrorView: View {

  var signMessageErrors: [BraveWallet.SignMessageError]
  var cryptoStore: CryptoStore

  @State private var currentIndex: Int = 0
  @Environment(\.presentationMode) @Binding private var presentationMode

  var body: some View {
    ScrollView {
      VStack(spacing: 16) {
        if signMessageErrors.count > 1 {
          signMessageErrorIndexDisplay
        }
        errorContainer
        Button {
          guard let currentRequest = signMessageErrors[safe: currentIndex] else { return }
          let isLastRequest = signMessageErrors.count <= 1
          cryptoStore.handleWebpageRequestResponse(.signMessageError(errorId: currentRequest.id))
          if isLastRequest {
            presentationMode.dismiss()
          }
        } label: {
          Text(Strings.Wallet.confirmedTransactionCloseButtonTitle)
            .frame(maxWidth: .infinity)
        }
        .buttonStyle(BraveFilledButtonStyle(size: .large))
        .disabled(currentIndex > 0)
      }
      .padding(.horizontal, 16)
      .padding(.top, 16)
    }
    .frame(maxWidth: .infinity, maxHeight: .infinity)
    .background(Color(braveSystemName: .containerHighlight).ignoresSafeArea())
    .navigationTitle(Strings.Wallet.securityRiskDetectedTitle)
    .navigationBarTitleDisplayMode(.inline)
  }

  private var signMessageErrorIndexDisplay: some View {
    HStack {
      Text(
        String.localizedStringWithFormat(
          Strings.Wallet.transactionCount,
          currentIndex + 1,
          signMessageErrors.count
        )
      )
      .fontWeight(.semibold)
      Button {
        if currentIndex + 1 < signMessageErrors.count {
          currentIndex += 1
        } else {
          currentIndex = 0
        }
      } label: {
        Text(Strings.Wallet.next)
          .fontWeight(.semibold)
          .foregroundColor(Color(.braveBlurpleTint))
      }
    }
    .frame(maxWidth: .infinity, alignment: .trailing)
  }

  private var errorContainer: some View {
    VStack(spacing: 0) {
      if let currentSignMessageError = signMessageErrors[safe: currentIndex] {
        Image(braveSystemName: "leo.warning.circle-filled")
          .imageScale(.large)
          .foregroundColor(Color(braveSystemName: .systemfeedbackErrorIcon))
          .padding(.bottom, 10)
        Text(verbatim: currentSignMessageError.originInfo.eTldPlusOne)
          .foregroundColor(Color(braveSystemName: .textPrimary))
          .font(.headline.weight(.semibold))
          .padding(.bottom, 8)
        Text(originInfo: currentSignMessageError.originInfo)
          .foregroundColor(Color(braveSystemName: .systemfeedbackErrorText))
          .font(.subheadline)
          .padding(.bottom, 16)
        Text(currentSignMessageError.localizedErrMsg)
          .font(.body)
          .padding(.bottom, 40)
      }
    }
    .multilineTextAlignment(.center)
    .padding(20)
    .background(Color(braveSystemName: .systemfeedbackErrorBackground).cornerRadius(12))
  }
}

#if DEBUG
struct SignMessageErrorView_Previews: PreviewProvider {
  static var previews: some View {
    SignMessageErrorView(
      signMessageErrors: [],
      cryptoStore: .previewStore
    )
  }
}
#endif
