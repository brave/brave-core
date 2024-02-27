// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import DesignSystem
import Foundation
import Introspect
import Strings
import SwiftUI

enum OnboardingSetupOption {
  case new
  case restore
}

struct SetupCryptoView: View {
  @ObservedObject var keyringStore: KeyringStore
  // Used to dismiss all of Wallet
  let dismissAction: () -> Void

  @State private var setupOption: OnboardingSetupOption?

  var body: some View {
    ScrollView {
      VStack(spacing: 32) {
        VStack(spacing: 14) {
          Text(Strings.Wallet.setupCryptoTitle)
            .foregroundColor(Color(uiColor: WalletV2Design.textPrimary))
            .font(.largeTitle)
          Text(Strings.Wallet.setupCryptoSubtitle)
            .foregroundColor(Color(uiColor: WalletV2Design.textSecondary))
            .font(.subheadline)
        }
        .fixedSize(horizontal: false, vertical: true)
        .multilineTextAlignment(.center)
        VStack(spacing: 24) {
          Button {
            setupOption = .new
          } label: {
            HStack(alignment: .top, spacing: 16) {
              Image(braveSystemName: "leo.plus.add")
                .frame(width: 32, height: 32)
                .background(Color(.secondaryButtonTint).opacity(0.3))
                .clipShape(Circle())
              VStack(alignment: .leading, spacing: 12) {
                Text(Strings.Wallet.setupCryptoCreateNewTitle)
                  .font(.title3.weight(.medium))
                  .foregroundColor(Color(uiColor: WalletV2Design.textPrimary))
                Text(Strings.Wallet.setupCryptoCreateNewSubTitle)
                  .font(.subheadline)
                  .foregroundColor(Color(uiColor: WalletV2Design.textSecondary))
              }
              .fixedSize(horizontal: false, vertical: true)
              .multilineTextAlignment(.leading)
              Spacer()
            }
            .padding(28)
            .background(Color(.braveBackground))
            .cornerRadius(16)
            .frame(maxWidth: .infinity)
          }
          Button {
            setupOption = .restore
          } label: {
            HStack(alignment: .top, spacing: 16) {
              Image(braveSystemName: "leo.import.arrow")
                .frame(width: 32, height: 32)
                .background(Color(.secondaryButtonTint).opacity(0.3))
                .clipShape(Circle())
              VStack(alignment: .leading, spacing: 12) {
                Group {
                  Text(Strings.Wallet.setupCryptoRestoreTitle)
                    .font(.title3.weight(.medium))
                    .foregroundColor(Color(uiColor: WalletV2Design.textPrimary))
                  Text(Strings.Wallet.setupCryptoRestoreSubTitle)
                    .font(.subheadline)
                    .foregroundColor(Color(uiColor: WalletV2Design.textSecondary))
                }
                .fixedSize(horizontal: false, vertical: true)
                .multilineTextAlignment(.leading)
                HStack(spacing: 14) {
                  Group {
                    Image("wallet-brave-icon", bundle: .module)
                      .resizable()
                    Image("wallet-phantom", bundle: .module)
                      .resizable()
                    Image("wallet-metamask", bundle: .module)
                      .resizable()
                    Image("wallet-coinbase", bundle: .module)
                      .resizable()
                  }
                  .aspectRatio(contentMode: .fit)
                  .frame(width: 20, height: 20)
                }
              }
              Spacer()
            }
            .padding(28)
            .background(Color(.braveBackground))
            .cornerRadius(16)
          }
        }
        Text(Strings.Wallet.setupCryptoDisclaimer)
          .font(.caption2)
          .foregroundColor(Color(.secondaryBraveLabel))
          .multilineTextAlignment(.center)
      }
      .frame(maxWidth: .infinity, maxHeight: .infinity)
      .padding(24)
    }
    .padding(.top, 80)
    .background(
      Image("wallet-background", bundle: .module)
        .resizable()
        .aspectRatio(contentMode: .fill)
    )
    .edgesIgnoringSafeArea(.all)
    .background(
      NavigationLink(
        isActive: Binding(
          get: { setupOption != nil },
          set: { if !$0 { setupOption = nil } }
        ),
        destination: {
          if let option = setupOption {
            LegalView(
              keyringStore: keyringStore,
              setupOption: option,
              dismissAction: dismissAction
            )
          }
        },
        label: {
          EmptyView()
        }
      )
    )
    .accessibilityEmbedInScrollView()
    .transparentNavigationBar(
      backButtonTitle: Strings.Wallet.setupCryptoButtonBackButtonTitle,
      backButtonDisplayMode: .generic
    )
    .onAppear {
      keyringStore.reportP3AOnboarding(action: .shown)
    }
  }
}

#if DEBUG
struct SetupCryptoView_Previews: PreviewProvider {
  static var previews: some View {
    NavigationView {
      SetupCryptoView(
        keyringStore: .previewStore,
        dismissAction: {}
      )
    }
    .previewLayout(.sizeThatFits)
    .previewColorSchemes()
  }
}
#endif
