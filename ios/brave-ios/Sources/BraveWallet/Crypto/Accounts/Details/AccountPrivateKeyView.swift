// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import DesignSystem
import Strings
import SwiftUI

struct AccountPrivateKeyView: View {
  @ObservedObject var keyringStore: KeyringStore
  var account: BraveWallet.AccountInfo

  @State private var password = ""
  @State private var error: PasswordEntryError?
  @State private var key: String?
  private var isKeyVisible: Bool { key != nil }

  @Environment(\.pixelLength) private var pixelLength

  private var isShowHideButtonDisabled: Bool {
    if key != nil {
      return false
    } else {
      return password.isEmpty
    }
  }

  private func validateAndShowPrivateKey() {
    keyringStore.privateKey(for: account, password: password) { key in
      if let key = key {
        withAnimation(nil) {
          self.key = key
          self.password = ""
        }
      } else {
        self.error = .incorrectPassword
        UIImpactFeedbackGenerator(style: .medium).vibrate()
      }
    }
  }

  var body: some View {
    ScrollView(.vertical) {
      VStack {
        Text(
          "\(Image(systemName: "exclamationmark.triangle.fill"))  \(Strings.Wallet.accountPrivateKeyDisplayWarning)"
        )
        .font(.subheadline.weight(.medium))
        .foregroundColor(Color(.braveLabel))
        .padding(12)
        .background(
          Color(.braveWarningBackground)
            .overlay(
              RoundedRectangle(cornerRadius: 10, style: .continuous)
                .strokeBorder(
                  Color(.braveWarningBorder),
                  style: StrokeStyle(lineWidth: pixelLength)
                )
            )
            .clipShape(RoundedRectangle(cornerRadius: 10, style: .continuous))
        )
        if let key = key {
          SensitiveTextView(
            text: key,
            isShowingText: Binding<Bool>(
              get: { self.key != nil },
              set: { if !$0 { self.key = nil } }
            )
          )
          .multilineTextAlignment(.center)
          .font(.system(.body, design: .monospaced))
          .padding(40)
        } else {
          PasswordEntryField(
            password: $password,
            error: $error,
            shouldShowBiometrics: true,
            keyringStore: keyringStore,
            onCommit: validateAndShowPrivateKey
          )
          .padding(40)
        }
        Button {
          withAnimation(nil) {
            if isKeyVisible {
              self.key = nil
            } else {
              validateAndShowPrivateKey()
            }
          }
        } label: {
          Text(
            isKeyVisible
              ? Strings.Wallet.hidePrivateKeyButtonTitle : Strings.Wallet.showPrivateKeyButtonTitle
          )
        }
        .buttonStyle(BraveFilledButtonStyle(size: .normal))
        .disabled(isShowHideButtonDisabled)
      }
      .padding()
    }
    .background(Color(.braveBackground))
    .navigationTitle(Strings.Wallet.accountPrivateKey)
    .navigationBarTitleDisplayMode(.inline)
    .onReceive(
      NotificationCenter.default.publisher(for: UIApplication.willResignActiveNotification)
    ) { _ in
      self.key = nil
    }
    .alertOnScreenshot {
      Alert(
        title: Text(Strings.Wallet.screenshotDetectedTitle),
        message: Text(Strings.Wallet.privateKeyScreenshotDetectedMessage),
        dismissButton: .cancel(Text(Strings.OKString))
      )
    }
  }
}

#if DEBUG
struct AccountPrivateKeyView_Previews: PreviewProvider {
  static var previews: some View {
    NavigationView {
      AccountPrivateKeyView(keyringStore: .previewStoreWithWalletCreated, account: .previewAccount)
    }
    .previewSizeCategories()
  }
}
#endif
