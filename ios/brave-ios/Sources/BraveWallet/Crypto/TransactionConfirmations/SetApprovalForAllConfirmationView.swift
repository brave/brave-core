// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/

import BraveCore
import Strings
import SwiftUI

struct SetApprovalForAllConfirmationView: View {
  @ObservedObject var confirmationStore: TransactionConfirmationStore

  var body: some View {
    VStack {
      Image(braveSystemName: "leo.warning.circle-filled")
        .foregroundColor(Color(braveSystemName: .systemfeedbackWarningIcon))
        .font(.title)
      Text(Strings.Wallet.erc721SetApprovalForAllConfirmationWarningTitle)
        .font(.subheadline.weight(.bold))
        .padding(.top, 10)
      Text(
        LocalizedStringKey(
          String.localizedStringWithFormat(
            Strings.Wallet.erc721SetApprovalForAllConfirmationWarningDescription,
            confirmationStore.activeParsedTransaction.toAddress.truncatedAddress
          )
        )
      )
      .font(.subheadline.weight(.regular))
      .multilineTextAlignment(.center)
      .padding(.top, 10)
      .padding(.horizontal, 10)
      Button(
        action: {
          Task {
            await confirmationStore.reject(
              transaction: confirmationStore.activeParsedTransaction.transaction
            )
          }
        },
        label: {
          Text(Strings.Wallet.erc721SetApprovalForAllCancelButtonTitle)
            .frame(maxWidth: .infinity)
        }
      )
      .buttonStyle(.filled)
      .controlSize(.large)
      .padding(.top, 32)
      Button(
        action: {
          Task {
            await confirmationStore.confirm(
              transaction: confirmationStore.activeParsedTransaction.transaction
            )
          }
        },
        label: {
          Text(Strings.Wallet.erc721SetApprovalForAllConfirmButtonTitle)
            .frame(maxWidth: .infinity)
        }
      )
      .buttonStyle(.outline)
      .controlSize(.large)
    }
    .frame(maxWidth: .infinity, maxHeight: .infinity)
    .padding(16)
    .background {
      Color(braveSystemName: .systemfeedbackWarningBackground).ignoresSafeArea()
    }
  }
}
