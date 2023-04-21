/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import SwiftUI
import BraveCore
import Swift
import Strings

/// Displays a summary of a given transaction
struct TransactionSummaryView: View {
  
  let summary: TransactionSummary
  let displayAccountCreator: Bool
  
  init(
    summary: TransactionSummary,
    displayAccountCreator: Bool = false
  ) {
    self.summary = summary
    self.displayAccountCreator = displayAccountCreator
  }
  
  private let relativeTimeFormatter = RelativeDateTimeFormatter().then {
    $0.unitsStyle = .full
    $0.dateTimeStyle = .numeric
  }
  
  @ViewBuilder private var subtitle: some View {
    // For the time being, use the same subtitle label until we have the ability to parse
    // Swap from/to addresses
    let from = summary.namedFromAddress
    let to = summary.namedToAddress
    Text("\(from) \(Image(systemName: "arrow.right")) \(to)")
      .accessibilityLabel(
        String.localizedStringWithFormat(
          Strings.Wallet.transactionFromToAccessibilityLabel, from, to
        )
      )
  }
  
  private var metadata: Text {
    let date = Text(summary.createdTime, formatter: relativeTimeFormatter)
    if displayAccountCreator {
      return date + Text(" · \(summary.namedFromAddress)")
    }
    return date
  }
  
  var body: some View {
    HStack(spacing: 12) {
      BlockieGroup(
        fromAddress: summary.fromAddress,
        toAddress: summary.toAddress,
        alignVisuallyCentered: false
      )
      .accessibilityHidden(true)
      VStack(alignment: .leading, spacing: 4) {
        Text(summary.title)
          .font(.footnote.weight(.semibold))
          .foregroundColor(Color(.bravePrimary))
        if let gasFee = summary.gasFee {
          HStack(spacing: 4) {
            Image(braveSystemName: "leo.coins.alt2")
            Text(
              String.localizedStringWithFormat(
                Strings.Wallet.transactionSummaryFee,
                gasFee.fee, summary.networkSymbol, gasFee.fiat)
            )
          }
          .accessibilityElement(children: .combine)
          .foregroundColor(Color(.braveLabel))
          .font(.caption)
        }
        subtitle
          .foregroundColor(Color(.braveLabel))
        HStack {
          metadata
            .foregroundColor(Color(.secondaryBraveLabel))
          Spacer()
          HStack(spacing: 4) {
            Image(systemName: "circle.fill")
              .foregroundColor(summary.txStatus.color)
              .imageScale(.small)
              .accessibilityHidden(true)
            Text(summary.txStatus.localizedDescription)
              .foregroundColor(Color(.braveLabel))
              .multilineTextAlignment(.trailing)
          }
          .accessibilityElement(children: .combine)
        }
        .font(.caption)
      }
      .font(.footnote)
    }
    .frame(maxWidth: .infinity, alignment: .leading)
    .padding(.vertical, 6)
  }
}

#if DEBUG
struct TransactionSummaryView_Previews: PreviewProvider {
  static var previews: some View {
    Group {
      TransactionSummaryView(summary: .previewConfirmedSend)
      TransactionSummaryView(summary: .previewConfirmedSwap)
      TransactionSummaryView(summary: .previewConfirmedERC20Approve)
    }
    .padding(12)
    .previewLayout(.sizeThatFits)
  }
}
#endif

extension BraveWallet.TransactionStatus {
  var localizedDescription: String {
    switch self {
    case .confirmed:
      return Strings.Wallet.transactionStatusConfirmed
    case .approved:
      return Strings.Wallet.transactionStatusApproved
    case .rejected:
      return Strings.Wallet.transactionStatusRejected
    case .unapproved:
      return Strings.Wallet.transactionStatusUnapproved
    case .submitted:
      return Strings.Wallet.transactionStatusSubmitted
    case .error:
      return Strings.Wallet.transactionStatusError
    case .dropped:
      return Strings.Wallet.transactionStatusDropped
    case .signed:
      return Strings.Wallet.transactionStatusSigned
    @unknown default:
      return Strings.Wallet.transactionStatusUnknown
    }
  }
  var color: Color {
    switch self {
    case .confirmed, .approved:
      return Color(.braveSuccessLabel)
    case .rejected, .error, .dropped:
      return Color(.braveErrorLabel)
    case .submitted, .signed:
      return Color(.braveWarningLabel)
    case .unapproved:
      return Color(.secondaryButtonTint)
    @unknown default:
      return Color.clear
    }
  }
}
