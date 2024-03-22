// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveStrings
import DesignSystem
import SwiftUI

/// View for showing `SignMessageRequest` for
/// ethSignTypedData, ethStandardSignData, & solanaSignData
struct SignMessageRequestView: View {

  let account: BraveWallet.AccountInfo
  let request: BraveWallet.SignMessageRequest
  let network: BraveWallet.NetworkInfo?
  let requestIndex: Int
  let requestCount: Int
  /// A map between request id and a boolean value indicates this request message needs pilcrow formating.
  @Binding var needPilcrowFormatted: [Int32: Bool]
  /// A map between request id and a boolean value indicates this request message is displayed as
  /// its original content.
  @Binding var showOrignalMessage: [Int32: Bool]
  var nextTapped: () -> Void
  var action: (_ approved: Bool) -> Void

  @Environment(\.sizeCategory) private var sizeCategory
  @ScaledMetric private var blockieSize = 54
  private let maxBlockieSize: CGFloat = 108

  /// Header containing the current requests network chain name, and a `1 of N` & `Next` button when there are multiple requests.
  private var requestsHeader: some View {
    HStack {
      if let network {
        Text(network.chainName)
          .font(.callout)
          .foregroundColor(Color(.braveLabel))
      }
      Spacer()
      if requestCount > 1 {
        NextIndexButton(
          currentIndex: requestIndex,
          count: requestCount,
          nextTapped: nextTapped
        )
      }
    }
  }

  private var accountInfoAndOrigin: some View {
    VStack(spacing: 8) {
      Blockie(address: account.blockieSeed)
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
      Text(originInfo: request.originInfo)
        .font(.caption)
        .foregroundColor(Color(.braveLabel))
        .multilineTextAlignment(.center)
    }
    .accessibilityElement(children: .combine)
  }

  var body: some View {
    ScrollView {
      VStack {
        requestsHeader

        VStack(spacing: 12) {
          accountInfoAndOrigin

          Text(Strings.Wallet.signatureRequestSubtitle)
            .font(.headline)
            .foregroundColor(Color(.bravePrimary))
        }
        .padding(.vertical, 32)

        SignMessageRequestContentView(
          request: request,
          needPilcrowFormatted: $needPilcrowFormatted,
          showOrignalMessage: $showOrignalMessage
        )

        buttonsContainer
          .padding(.top)
          .opacity(sizeCategory.isAccessibilityCategory ? 0 : 1)
          .accessibility(hidden: sizeCategory.isAccessibilityCategory)
      }
      .padding()
    }
    .foregroundColor(Color(.braveLabel))
    .overlay(alignment: .bottom) {
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
    }
    .navigationTitle(Strings.Wallet.signatureRequestTitle)
  }

  /// Cancel & Sign button container
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

  /// Cancel and Sign buttons
  @ViewBuilder private var buttons: some View {
    Button {  // cancel
      action(false)
    } label: {
      Label(Strings.cancelButtonTitle, systemImage: "xmark")
        .imageScale(.large)
    }
    .buttonStyle(BraveOutlineButtonStyle(size: .large))
    .disabled(requestIndex != 0)
    Button {  // approve
      action(true)
    } label: {
      Label(Strings.Wallet.sign, braveSystemImage: "leo.key")
        .imageScale(.large)
    }
    .buttonStyle(BraveFilledButtonStyle(size: .large))
    .disabled(requestIndex != 0)
  }
}

/// View that displays the current index, total number of items and a `Next` button to move to next index.
struct NextIndexButton: View {

  let currentIndex: Int
  let count: Int
  let nextTapped: () -> Void

  var body: some View {
    HStack {
      Text(
        String.localizedStringWithFormat(Strings.Wallet.transactionCount, currentIndex + 1, count)
      )
      .fontWeight(.semibold)
      Button {
        nextTapped()
      } label: {
        Text(Strings.Wallet.next)
          .fontWeight(.semibold)
          .foregroundColor(Color(.braveBlurpleTint))
      }
    }
  }
}
