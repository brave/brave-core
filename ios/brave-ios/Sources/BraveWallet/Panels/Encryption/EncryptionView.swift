// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import DesignSystem
import SwiftUI

struct EncryptionView: View {

  enum EncryptionType: Hashable {
    case getEncryptionPublicKey(BraveWallet.GetEncryptionPublicKeyRequest)
    case decrypt(BraveWallet.DecryptRequest)

    var address: String {
      switch self {
      case .getEncryptionPublicKey(let request):
        return request.accountId.address
      case .decrypt(let request):
        return request.accountId.address
      }
    }

    var originInfo: BraveWallet.OriginInfo {
      switch self {
      case .getEncryptionPublicKey(let request):
        return request.originInfo
      case .decrypt(let request):
        return request.originInfo
      }
    }
  }

  var request: EncryptionType
  @ObservedObject var cryptoStore: CryptoStore
  @ObservedObject var keyringStore: KeyringStore
  var onDismiss: () -> Void

  @State private var isShowingDecryptMessage = false

  @ScaledMetric private var blockieSize = 54
  private let maxBlockieSize: CGFloat = 108
  @Environment(\.sizeCategory) private var sizeCategory

  private var account: BraveWallet.AccountInfo {
    keyringStore.allAccounts.first(where: {
      $0.address.caseInsensitiveCompare(request.address) == .orderedSame
    }) ?? keyringStore.selectedAccount
  }

  private var navigationTitle: String {
    switch request {
    case .getEncryptionPublicKey:
      return Strings.Wallet.getEncryptionPublicKeyRequestTitle
    case .decrypt:
      return Strings.Wallet.decryptRequestTitle
    }
  }

  private var subtitle: String {
    switch request {
    case .getEncryptionPublicKey:
      return Strings.Wallet.getEncryptionPublicKeyRequestSubtitle
    case .decrypt:
      return Strings.Wallet.decryptRequestSubtitle
    }
  }

  var body: some View {
    ScrollView(.vertical) {
      VStack(spacing: 12) {
        VStack(spacing: 8) {
          Blockie(address: request.address)
            .frame(
              width: min(blockieSize, maxBlockieSize),
              height: min(blockieSize, maxBlockieSize)
            )
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
        Text(subtitle)
          .font(.headline)
          .foregroundColor(Color(.bravePrimary))
      }
      .padding(.vertical, 32)
      .multilineTextAlignment(.center)
      Group {
        if case .getEncryptionPublicKey = request {
          ScrollView {
            Text(originInfo: request.originInfo)
              + Text(" \(Strings.Wallet.getEncryptionPublicKeyRequestMessage)")
          }
          .padding(20)
        } else if case .decrypt(let decryptRequest) = request {
          ScrollView {
            SensitiveTextView(
              text: decryptRequest.unsafeMessage,
              isCopyEnabled: false,
              isShowingText: $isShowingDecryptMessage
            )
          }
          .frame(maxWidth: .infinity, maxHeight: .infinity, alignment: .topLeading)
          .overlay(
            Group {
              if !isShowingDecryptMessage {
                Button {
                  isShowingDecryptMessage.toggle()
                } label: {
                  Text(Strings.Wallet.decryptRequestReveal)
                }
                .buttonStyle(BraveFilledButtonStyle(size: .normal))
              }
            }
          )
          .alertOnScreenshot {
            Alert(
              title: Text(Strings.Wallet.screenshotDetectedTitle),
              message: Text(Strings.Wallet.decryptMessageScreenshotDetectedMessage),
              dismissButton: .cancel(Text(Strings.OKString))
            )
          }
        }
      }
      .frame(maxWidth: .infinity)
      .frame(height: 200)
      .background(Color(.tertiaryBraveGroupedBackground))
      .clipShape(RoundedRectangle(cornerRadius: 5, style: .continuous))
      .padding()
      .background(
        Color(.secondaryBraveGroupedBackground)
      )
      .clipShape(RoundedRectangle(cornerRadius: 10, style: .continuous))

      buttonsContainer
        .padding(.top, 20)
        .opacity(sizeCategory.isAccessibilityCategory ? 0 : 1)
        .accessibility(hidden: sizeCategory.isAccessibilityCategory)
    }
    .padding()
    .overlay(
      Group {
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
      },
      alignment: .bottom
    )
    .frame(maxWidth: .infinity)
    .navigationTitle(navigationTitle)
    .navigationBarTitleDisplayMode(.inline)
    .foregroundColor(Color(.braveLabel))
    .background(Color(.braveGroupedBackground).edgesIgnoringSafeArea(.all))
    .toolbar {
      ToolbarItemGroup(placement: .cancellationAction) {
        Button {
          onDismiss()
        } label: {
          Text(Strings.cancelButtonTitle)
            .foregroundColor(Color(.braveBlurpleTint))
        }
      }
    }
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

  private var approveButtonTitle: String {
    switch request {
    case .getEncryptionPublicKey:
      return Strings.Wallet.getEncryptionPublicKeyRequestApprove
    case .decrypt:
      return Strings.Wallet.decryptRequestApprove
    }
  }

  @ViewBuilder private var buttons: some View {
    Button {  // cancel
      handleAction(approved: false)
    } label: {
      Label(Strings.cancelButtonTitle, systemImage: "xmark")
        .imageScale(.large)
    }
    .buttonStyle(BraveOutlineButtonStyle(size: .large))
    Button {  // approve
      handleAction(approved: true)
    } label: {
      Label(approveButtonTitle, braveSystemImage: "leo.check.circle-filled")
        .imageScale(.large)
    }
    .buttonStyle(BraveFilledButtonStyle(size: .large))
  }

  private func handleAction(approved: Bool) {
    switch request {
    case .getEncryptionPublicKey(let request):
      cryptoStore.handleWebpageRequestResponse(
        .getEncryptionPublicKey(approved: approved, requestId: request.requestId)
      )
    case .decrypt(let request):
      cryptoStore.handleWebpageRequestResponse(
        .decrypt(approved: approved, requestId: request.requestId)
      )
    }
    onDismiss()
  }
}

#if DEBUG
struct EncryptionView_Previews: PreviewProvider {
  static var previews: some View {
    let account = BraveWallet.AccountInfo.previewAccount
    let originInfo = BraveWallet.OriginInfo(
      originSpec: WalletConstants.braveWalletOriginSpec,
      eTldPlusOne: ""
    )
    let requests: [EncryptionView.EncryptionType] = [
      .getEncryptionPublicKey(
        .init(
          requestId: UUID().uuidString,
          originInfo: originInfo,
          accountId: account.accountId
        )
      ),
      .decrypt(
        .init(
          requestId: UUID().uuidString,
          originInfo: originInfo,
          accountId: account.accountId,
          unsafeMessage: "Secret message"
        )
      ),
    ]
    Group {
      ForEach(requests, id: \.self) { request in
        EncryptionView(
          request: request,
          cryptoStore: .previewStore,
          keyringStore: .previewStoreWithWalletCreated,
          onDismiss: {}
        )
      }
    }
  }
}
#endif
