// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import SwiftUI
import BraveCore
import BraveUI
import struct Shared.Strings

struct AccountPrivateKeyView: View {
  @ObservedObject var keyringStore: KeyringStore
  var account: BraveWallet.AccountInfo
  
  @State private var key: String = ""
  @State private var isKeyVisible: Bool = false
  
  @Environment(\.pixelLength) private var pixelLength
  
  var body: some View {
    VStack {
      Text("\(Image(systemName: "exclamationmark.triangle.fill"))  \(Strings.Wallet.accountPrivateKeyDisplayWarning)")
        .font(.subheadline.weight(.medium))
        .foregroundColor(Color(.braveLabel))
        .padding(12)
        .background(
          Color(.braveWarningBackground)
            .overlay(
              RoundedRectangle(cornerRadius: 10, style: .continuous)
                .strokeBorder(Color(.braveWarningBorder), style: StrokeStyle(lineWidth: pixelLength))
            )
            .clipShape(RoundedRectangle(cornerRadius: 10, style: .continuous))
        )
      VStack(spacing: 32) {
        Text(key)
          .multilineTextAlignment(.center)
          .font(.system(.body, design: .monospaced))
          .blur(radius: isKeyVisible ? 0 : 5)
        Button(action: {
          UIPasteboard.general.string = key
        }) {
          Text("\(Strings.Wallet.copyToPasteboard) \(Image("brave.clipboard"))")
            .font(.subheadline)
            .foregroundColor(Color(.braveBlurple))
        }
      }
      .padding(20)
      .background(
        Color(.secondaryBraveBackground).clipShape(RoundedRectangle(cornerRadius: 10, style: .continuous))
      )
      .padding(40)
      
      Button(action: {
        withAnimation {
          isKeyVisible.toggle()
        }
      }) {
        Text(isKeyVisible ? Strings.Wallet.hidePrivateKeyButtonTitle :
              Strings.Wallet.showPrivateKeyButtonTitle)
      }
      .buttonStyle(BraveFilledButtonStyle(size: .normal))
      .animation(nil, value: isKeyVisible)
    }
    .padding()
    .frame(maxHeight: .infinity, alignment: .top)
    .onAppear {
      keyringStore.privateKey(for: account) { key in
        // TODO: Error state?
        self.key = key ?? ""
      }
    }
    .background(Color(.braveBackground))
    .navigationTitle(Strings.Wallet.accountPrivateKey)
    .navigationBarTitleDisplayMode(.inline)
  }
}

#if DEBUG
struct AccountPrivateKeyView_Previews: PreviewProvider {
  static var previews: some View {
    NavigationView {
      AccountPrivateKeyView(keyringStore: .previewStoreWithWalletCreated, account: .previewAccount)
    }
  }
}
#endif
