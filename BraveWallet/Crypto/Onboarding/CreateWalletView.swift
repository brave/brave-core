/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import SwiftUI
import LocalAuthentication
import BraveUI

@available(iOS 14.0, *)
struct CreateWalletContainerView: View {
  @ObservedObject var keyringStore: KeyringStore
  
  var body: some View {
    ScrollView(.vertical) {
      CreateWalletView(keyringStore: keyringStore)
        .background(Color(.braveBackground))
    }
    .background(Color(.braveBackground).edgesIgnoringSafeArea(.all))
    .navigationTitle("Crypto") // NSLocalizedString
    .navigationBarTitleDisplayMode(.inline)
    .introspectViewController { vc in
      vc.navigationItem.backButtonTitle = "Create Password" // NSLocalizedString
      vc.navigationItem.backButtonDisplayMode = .minimal
    }
  }
}

@available(iOS 14.0, *)
private struct CreateWalletView: View {
  @ObservedObject var keyringStore: KeyringStore
  
  private enum ValidationError: LocalizedError, Equatable {
    case passwordTooShort
    case inputsDontMatch
    
    var errorDescription: String? {
      switch self {
      case .passwordTooShort:
        return "Password must be 7 or more characters" // NSLocalizedString
      case .inputsDontMatch:
        // TODO: Get real copy
        return "Passwords don't match" // NSLocalizedString
      }
    }
  }
  
  @State private var password: String = ""
  @State private var repeatedPassword: String = ""
  @State private var validationError: ValidationError?
  @State private var isShowingBiometricsPrompt: Bool = false
  @State private var isSkippingBiometricsPrompt: Bool = false
  
  private func createWallet() {
    if !validate() { return }
    keyringStore.createWallet(password: password) { mnemonic in
      if !mnemonic.isEmpty {
        if isBiometricsAvailable {
          isShowingBiometricsPrompt = true
        } else {
          isSkippingBiometricsPrompt = true
        }
      }
    }
  }
  
  private var isBiometricsAvailable: Bool {
    LAContext().canEvaluatePolicy(.deviceOwnerAuthenticationWithBiometrics, error: nil)
  }
  
  private func validate() -> Bool {
    if password.count < 7 {
      validationError = .passwordTooShort
    } else if password != repeatedPassword {
      validationError = .inputsDontMatch
    } else {
      validationError = nil
    }
    return validationError == nil
  }
  
  private func handlePasswordChanged(_ value: String) {
    if validationError == .passwordTooShort {
      // Reset validation on user changing
      validationError = nil
    }
  }
  
  private func handleRepeatedPasswordChanged(_ value: String) {
    if validationError == .inputsDontMatch {
      // Reset validation on user changing
      validationError = nil
    }
  }
  
  var body: some View {
    VStack(spacing: 0) {
      VStack(spacing: 46) {
        Image("graphic-lock")
          .padding(.bottom)
        VStack {
          Text("Secure your crypto with a password") // NSLocalizedString
            .font(.headline)
            .padding(.bottom)
            .multilineTextAlignment(.center)
            .fixedSize(horizontal: false, vertical: true)
          VStack {
            SecureField("Password", text: $password) // NSLocalizedString
              .textFieldStyle(BraveValidatedTextFieldStyle(error: validationError, when: .passwordTooShort))
            SecureField("Re-type password", text: $repeatedPassword, onCommit: createWallet) // NSLocalizedString
              .textFieldStyle(BraveValidatedTextFieldStyle(error: validationError, when: .inputsDontMatch))
          }
          .font(.subheadline)
          .padding(.horizontal, 48)
        }
        Button(action: createWallet) {
          Text("Continue")
        }
        .buttonStyle(BraveFilledButtonStyle(size: .normal))
      }
      .frame(maxHeight: .infinity, alignment: .top)
      .padding()
      .padding(.vertical)
      .background(BiometricsPromptView(isPresented: $isShowingBiometricsPrompt) { enabled, navController in
        if enabled {
          // Store password in keychain
          if !KeyringStore.storePasswordInKeychain(password) {
            // TODO: Get real copy
            let alert = UIAlertController(title: "Failed to enable biometrics", message: "There was an error while trying to enable biometrics. Please try again later", preferredStyle: .alert)
            alert.addAction(.init(title: "OK", style: .default, handler: nil))
            navController?.presentedViewController?.present(alert, animated: true)
            return false
          }
        }
        let controller = UIHostingController(rootView: BackupWalletView(keyringStore: keyringStore))
        navController?.pushViewController(controller, animated: true)
        return true
      })
      .background(
        NavigationLink(
          destination: BackupWalletView(keyringStore: keyringStore),
          isActive: $isSkippingBiometricsPrompt
        ) {
          EmptyView()
        }
      )
      .onChange(of: password, perform: handlePasswordChanged)
      .onChange(of: repeatedPassword, perform: handleRepeatedPasswordChanged)
    }
  }
}

private struct BiometricsPromptView: UIViewControllerRepresentable {
  @Binding var isPresented: Bool
  var action: (Bool, UINavigationController?) -> Bool
  
  func makeUIViewController(context: Context) -> UIViewController {
    .init()
  }
  
  func updateUIViewController(_ uiViewController: UIViewController, context: Context) {
    if isPresented {
      if uiViewController.presentedViewController != nil {
        return
      }
      let controller = PopupViewController(rootView: EnableBiometricsView(action: { enabled in
        if action(enabled, uiViewController.navigationController) {
          uiViewController.dismiss(animated: true) {
            isPresented = false
          }
        }
      }))
      uiViewController.present(controller, animated: true)
    } else {
      if uiViewController.presentedViewController != nil {
        uiViewController.dismiss(animated: true)
      }
    }
  }
}

#if DEBUG
@available(iOS 14.0, *)
struct CreateWalletView_Previews: PreviewProvider {
  static var previews: some View {
    NavigationView {
      CreateWalletContainerView(keyringStore: .previewStore)
    }
    .previewLayout(.sizeThatFits)
    .previewColorSchemes()
  }
}
#endif
