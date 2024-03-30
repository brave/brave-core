// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import DesignSystem
import SwiftUI

struct LegalView: View {
  @ObservedObject var keyringStore: KeyringStore
  let setupOption: OnboardingSetupOption
  // Used to dismiss all of Wallet
  let dismissAction: () -> Void

  @State private var isResponsibilityCheckboxChecked: Bool = false
  @State private var isTermsCheckboxChecked: Bool = false
  @State private var isShowingCreateNewWallet: Bool = false
  @State private var isShowingRestoreExistedWallet: Bool = false

  private var isContinueDisabled: Bool {
    !isResponsibilityCheckboxChecked || !isTermsCheckboxChecked
  }

  var body: some View {
    ScrollView {
      VStack(spacing: 20) {
        VStack(spacing: 8) {
          Text(Strings.Wallet.legalTitle)
            .font(.title)
            .foregroundColor(Color(uiColor: WalletV2Design.textPrimary))
          Text(Strings.Wallet.legalDescription)
            .font(.subheadline)
            .foregroundColor(Color(uiColor: WalletV2Design.textSecondary))
        }
        .multilineTextAlignment(.center)
        .fixedSize(horizontal: false, vertical: true)
        .padding(.bottom, 20)
        HStack(alignment: .top, spacing: 8) {
          LegalCheckbox(isChecked: $isResponsibilityCheckboxChecked)
            .font(.title2)
          Text(Strings.Wallet.legalUserResponsibility)
            .foregroundColor(Color(uiColor: WalletV2Design.textPrimary))
            .font(.subheadline)
            .frame(maxWidth: .infinity, alignment: .leading)
            .onTapGesture {
              isResponsibilityCheckboxChecked.toggle()
            }
        }
        HStack(spacing: 8) {
          LegalCheckbox(isChecked: $isTermsCheckboxChecked)
            .font(.title2)
          Text(
            LocalizedStringKey(
              String.localizedStringWithFormat(
                Strings.Wallet.legalTermOfUse,
                WalletConstants.braveWalletTermsOfUse.absoluteDisplayString
              )
            )
          )
          .foregroundColor(Color(uiColor: WalletV2Design.textPrimary))
          .tint(Color(.braveBlurpleTint))
          .font(.subheadline)
          .frame(maxWidth: .infinity, alignment: .leading)
          .onTapGesture {
            isTermsCheckboxChecked.toggle()
          }
        }
        Button {
          if setupOption == .new {
            isShowingCreateNewWallet = true
          } else {
            isShowingRestoreExistedWallet = true
          }
        } label: {
          Text(Strings.Wallet.continueButtonTitle)
            .frame(maxWidth: .infinity)
        }
        .buttonStyle(BraveFilledButtonStyle(size: .large))
        .disabled(isContinueDisabled)
        .padding(.top, 40)
      }
    }
    .padding()
    .background(
      NavigationLink(
        destination: CreateWalletView(
          keyringStore: keyringStore,
          setupOption: setupOption,
          dismissAction: dismissAction
        ),
        isActive: $isShowingCreateNewWallet,
        label: {
          EmptyView()
        }
      )
    )
    .background(
      NavigationLink(
        destination: RestoreWalletView(
          keyringStore: keyringStore,
          dismissAction: dismissAction
        ),
        isActive: $isShowingRestoreExistedWallet,
        label: {
          EmptyView()
        }
      )
    )
    .accessibilityEmbedInScrollView()
    .background(Color(.braveBackground).edgesIgnoringSafeArea(.all))
    .transparentNavigationBar(
      backButtonTitle: Strings.Wallet.web3DomainInterstitialPageTAndU.capitalizeFirstLetter,
      backButtonDisplayMode: .generic
    )
  }
}

struct LegalCheckbox: View {
  @Binding var isChecked: Bool

  var body: some View {
    Button {
      isChecked.toggle()
    } label: {
      Image(braveSystemName: isChecked ? "leo.checkbox.checked" : "leo.checkbox.unchecked")
        .renderingMode(.template)
        .foregroundColor(Color(isChecked ? .braveBlurpleTint : .braveDisabled))
    }
  }
}

#if DEBUG
struct LegalView_Previews: PreviewProvider {
  static var previews: some View {
    LegalView(
      keyringStore: .previewStore,
      setupOption: .new,
      dismissAction: {}
    )
  }
}
#endif
