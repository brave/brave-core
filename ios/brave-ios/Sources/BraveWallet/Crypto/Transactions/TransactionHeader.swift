// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Strings
import SwiftUI

struct TransactionHeader: View {

  let fromAccountInfo: BraveWallet.AccountInfo
  let fromAccountName: String
  let toAccountAddress: String
  let toAccountName: String
  let isContractAddress: Bool
  let originInfo: BraveWallet.OriginInfo?
  let transactionType: String
  let value: String
  let fiat: String?
  let contractAddressTapped: (String) -> Void

  @Environment(\.sizeCategory) private var sizeCategory
  @ScaledMetric private var blockieSize = 48
  private let maxBlockieSize: CGFloat = 96

  var body: some View {
    VStack(spacing: 8) {
      VStack(spacing: 8) {
        if fromAccountInfo.address == toAccountAddress || toAccountAddress.isEmpty {
          Blockie(address: fromAccountInfo.address)
            .frame(
              width: min(blockieSize, maxBlockieSize),
              height: min(blockieSize, maxBlockieSize)
            )
          AddressView(address: fromAccountInfo.address) {
            Text(fromAccountName)
          }
        } else {
          BlockieGroup(
            fromAddress: fromAccountInfo.blockieSeed,
            toAddress: toAccountAddress,
            size: min(blockieSize, maxBlockieSize)
          )
          Group {
            if sizeCategory.isAccessibilityCategory {
              VStack {
                AddressView(address: fromAccountInfo.address) {
                  Text(fromAccountName)
                }
                Image(systemName: "arrow.down")
                toAddressView
              }
            } else {
              HStack {
                AddressView(address: fromAccountInfo.address) {
                  Text(fromAccountName)
                }
                Image(systemName: "arrow.right")
                toAddressView
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
        label: Text(
          String.localizedStringWithFormat(
            Strings.Wallet.transactionFromToAccessibilityLabel,
            fromAccountName,
            toAccountName
          )
        )
      )
      if let originInfo = originInfo {
        Text(originInfo: originInfo)
          .foregroundColor(Color(.braveLabel))
          .font(.subheadline)
          .multilineTextAlignment(.center)
          .padding(.top, 8)  // match vertical padding for tx type, value & fiat VStack
      }
      VStack(spacing: 4) {
        Text(transactionType)
          .font(.footnote)
        Text(value)
          .fontWeight(.semibold)
          .foregroundColor(Color(.bravePrimary))
        if let fiat = fiat {
          Text(fiat)  // Value in Fiat
            .font(.footnote)
        }
      }
      .padding(.vertical, 8)
    }
  }

  @ViewBuilder private var toAddressView: some View {
    if isContractAddress {
      VStack {
        Text(Strings.Wallet.contractAddressAccessibilityLabel)
          .font(.caption)
        Button(
          action: { contractAddressTapped(toAccountAddress) },
          label: {
            HStack {
              AddressView(address: toAccountAddress) {
                Text(toAccountName.isEmpty ? toAccountAddress.truncatedAddress : toAccountName)
              }
              Image(systemName: "arrow.up.forward.square")
            }
            .foregroundStyle(Color(braveSystemName: .textInteractive))
          }
        )
      }
    } else {
      AddressView(address: toAccountAddress) {
        Text(toAccountName.isEmpty ? toAccountAddress.truncatedAddress : toAccountName)
      }
    }
  }
}
