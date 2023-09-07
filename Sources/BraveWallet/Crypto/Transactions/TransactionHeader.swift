// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import BraveCore
import Strings
import SwiftUI

struct TransactionHeader: View {
  
  let fromAccountAddress: String
  let fromAccountName: String
  let toAccountAddress: String
  let toAccountName: String
  let originInfo: BraveWallet.OriginInfo?
  let transactionType: String
  let value: String
  let fiat: String?
  
  @Environment(\.sizeCategory) private var sizeCategory
  @ScaledMetric private var blockieSize = 48
  private let maxBlockieSize: CGFloat = 96
  
  var body: some View {
    VStack(spacing: 8) {
      VStack(spacing: 8) {
        if fromAccountAddress == toAccountAddress || toAccountAddress.isEmpty {
          Blockie(address: fromAccountAddress)
            .frame(width: min(blockieSize, maxBlockieSize), height: min(blockieSize, maxBlockieSize))
          AddressView(address: fromAccountAddress) {
            Text(fromAccountName)
          }
        } else {
          BlockieGroup(
            fromAddress: fromAccountAddress,
            toAddress: toAccountAddress,
            size: min(blockieSize, maxBlockieSize)
          )
          Group {
            if sizeCategory.isAccessibilityCategory {
              VStack {
                AddressView(address: fromAccountAddress) {
                  Text(fromAccountName)
                }
                Image(systemName: "arrow.down")
                AddressView(address: toAccountAddress) {
                  Text(toAccountName)
                }
              }
            } else {
              HStack {
                AddressView(address: fromAccountAddress) {
                  Text(fromAccountName)
                }
                .frame(minWidth: 0, maxWidth: .infinity)
                Image(systemName: "arrow.right")
                AddressView(address: toAccountAddress) {
                  Text(toAccountName)
                }
                .frame(minWidth: 0, maxWidth: .infinity)
              }
            }
          }
          .foregroundColor(Color(.bravePrimary))
          .font(.callout)
        }
      }
      .accessibilityElement()
      .accessibility(addTraits: .isStaticText)
      .accessibility(
        label: Text(String.localizedStringWithFormat(
          Strings.Wallet.transactionFromToAccessibilityLabel, fromAccountName, toAccountName
        ))
      )
      if let originInfo = originInfo {
        Text(originInfo: originInfo)
          .foregroundColor(Color(.braveLabel))
          .font(.subheadline)
          .multilineTextAlignment(.center)
          .padding(.top, 8) // match vertical padding for tx type, value & fiat VStack
      }
      VStack(spacing: 4) {
        Text(transactionType)
          .font(.footnote)
        Text(value)
          .fontWeight(.semibold)
          .foregroundColor(Color(.bravePrimary))
        if let fiat = fiat {
          Text(fiat) // Value in Fiat
            .font(.footnote)
        }
      }
      .padding(.vertical, 8)
    }
  }
}
