// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveStrings
import DesignSystem
import SwiftUI

/// View for showing `SignMessageRequest` for  ethSiweData
struct SignInWithEthereumView: View {

  let account: BraveWallet.AccountInfo
  let originInfo: BraveWallet.OriginInfo
  let message: BraveWallet.SIWEMessage
  var action: (_ approved: Bool) -> Void

  @State private var isShowingDetails: Bool = false
  @Environment(\.sizeCategory) private var sizeCategory

  var body: some View {
    ScrollView {
      VStack(spacing: 10) {
        faviconAndOrigin

        messageContainer

        buttonsContainer
          .padding(.top)
          .opacity(sizeCategory.isAccessibilityCategory ? 0 : 1)
          .accessibility(hidden: sizeCategory.isAccessibilityCategory)
      }
      .padding()
    }
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
    .background(Color(braveSystemName: .containerHighlight))
    .navigationTitle(Strings.Wallet.signInWithBraveWallet)
  }

  private var faviconAndOrigin: some View {
    VStack(spacing: 8) {
      OriginInfoFavicon(originInfo: originInfo)
      Text(verbatim: originInfo.eTldPlusOne)
      Text(originInfo: originInfo)
        .font(.caption)
        .foregroundColor(Color(.braveLabel))
        .multilineTextAlignment(.center)
    }
  }

  private var messageContainer: some View {
    VStack(alignment: .leading, spacing: 10) {
      AddressView(address: account.address) {
        AccountView(account: account)
      }

      // 'You are signing into xyz. Brave Wallet will share your wallet address with xyz.'
      Text(
        String.localizedStringWithFormat(
          Strings.Wallet.signInWithBraveWalletMessage,
          originInfo.eTldPlusOne,
          originInfo.eTldPlusOne
        )
      )

      NavigationLink(
        destination: SignInWithEthereumDetailsView(
          originInfo: originInfo,
          message: message
        )
      ) {
        Text(Strings.Wallet.seeDetailsButtonTitle)
          .fontWeight(.semibold)
          .foregroundColor(Color(braveSystemName: .textInteractive))
          .contentShape(Rectangle())
      }

      if let statement = message.statement, let resources = message.resources {
        Divider()

        VStack(alignment: .leading, spacing: 6) {
          Text(Strings.Wallet.siweMessageLabel)
            .font(.headline)
          Text(verbatim: statement)
            .textSelection(.enabled)
            .font(.subheadline)
        }

        VStack(alignment: .leading, spacing: 6) {
          Text(Strings.Wallet.siweResourcesLabel)
            .font(.headline)
          ForEach(resources.indices, id: \.self) { index in
            if let resource = resources[safe: index] {
              Text(verbatim: resource.absoluteString)
                .textSelection(.enabled)
                .font(.subheadline)
            }
          }
        }
      }
    }
    .padding()
    .foregroundColor(Color(braveSystemName: .textPrimary))
    .multilineTextAlignment(.leading)
    .background(
      Color(braveSystemName: .containerBackground)
        .cornerRadius(12)
    )
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
    Button {  // cancel
      action(false)
    } label: {
      Text(Strings.cancelButtonTitle)
    }
    .buttonStyle(BraveOutlineButtonStyle(size: .large))
    Button {  // approve
      action(true)
    } label: {
      Text(Strings.Wallet.siweSignInButtonTitle)
    }
    .buttonStyle(BraveFilledButtonStyle(size: .large))
  }
}

/// The view pushed when user taps to view request details.
private struct SignInWithEthereumDetailsView: View {

  let originInfo: BraveWallet.OriginInfo
  let message: BraveWallet.SIWEMessage

  var body: some View {
    ScrollView {
      LazyVStack {
        LazyVStack {
          Group {  // Max view count on `LazyVStack`
            detailRow(title: Strings.Wallet.siweOriginLabel, value: Text(originInfo: originInfo))
            Divider()
            detailRow(
              title: Strings.Wallet.siweAddressLabel,
              value: Text(verbatim: message.address)
            )
            if let statement = message.statement {
              Divider()
              detailRow(title: Strings.Wallet.siweStatementLabel, value: Text(verbatim: statement))
            }
            Divider()
            detailRow(
              title: Strings.Wallet.siweURILabel,
              value: Text(verbatim: message.uri.absoluteString)
            )
          }
          Group {  // Max view count on `LazyVStack`
            Divider()
            detailRow(
              title: Strings.Wallet.siweVersionLabel,
              value: Text(verbatim: "\(message.version)")
            )
            Divider()
            detailRow(
              title: Strings.Wallet.siweChainIDLabel,
              value: Text(verbatim: "\(message.chainId)")
            )
            Divider()
            detailRow(
              title: Strings.Wallet.siweIssuedAtLabel,
              value: Text(verbatim: message.issuedAt)
            )
            if let expirationTime = message.expirationTime {
              Divider()
              detailRow(
                title: Strings.Wallet.siweExpirationTimeLabel,
                value: Text(verbatim: expirationTime)
              )
            }
            Divider()
            detailRow(title: Strings.Wallet.siweNonceLabel, value: Text(verbatim: message.nonce))
            if let resources = message.resources {
              Divider()
              detailRow(
                title: Strings.Wallet.siweResourcesLabel,
                value: Text(verbatim: resources.map(\.absoluteString).joined(separator: "\n"))
              )
            }
          }
        }
        .frame(maxWidth: .infinity)
      }
      .padding(16)
      .multilineTextAlignment(.leading)
    }
    .navigationTitle(Strings.Wallet.siweDetailsTitle)
    .navigationBarTitleDisplayMode(.inline)
    .background(Color(braveSystemName: .containerHighlight))
  }

  private func detailRow(title: String, value: String) -> some View {
    detailRow(title: title, value: Text(verbatim: value))
  }

  private func detailRow(title: String, value: Text) -> some View {
    HStack(spacing: 12) {
      Text(title)
        .fontWeight(.semibold)
        .foregroundColor(Color(braveSystemName: .textSecondary))
        .frame(width: 100, alignment: .leading)
      value
        .foregroundColor(Color(braveSystemName: .textPrimary))
        .textSelection(.enabled)
      Spacer()
    }
    .padding(.vertical, 8)
    .frame(maxWidth: .infinity)
  }
}
