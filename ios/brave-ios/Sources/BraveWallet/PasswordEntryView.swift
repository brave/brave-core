// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveStrings
import DesignSystem
import LocalAuthentication
import SwiftUI

struct PasswordEntryError: LocalizedError, Equatable {
  let message: String
  var errorDescription: String? { message }

  static let incorrectPassword = Self(message: Strings.Wallet.incorrectPasswordErrorMessage)
  static let failedToEnableBiometrics = Self(message: Strings.Wallet.biometricsSetupErrorMessage)
}

/// Field for entering wallet password, with optional biometrics support
struct PasswordEntryField: View {

  /// The password being entered
  @Binding var password: String
  /// The error displayed under the password field
  @Binding var error: PasswordEntryError?
  /// Password field placeholder
  let placeholder: String
  /// If we should show the biometrics icon to allow biometics unlock (if available & password stored in keychain)
  let shouldShowBiometrics: Bool
  let keyringStore: KeyringStore
  let onCommit: () -> Void

  @State private var attemptedBiometricsUnlock: Bool = false

  init(
    password: Binding<String>,
    error: Binding<PasswordEntryError?>,
    placeholder: String = Strings.Wallet.passwordPlaceholder,
    shouldShowBiometrics: Bool,
    keyringStore: KeyringStore,
    onCommit: @escaping () -> Void
  ) {
    self._password = password
    self._error = error
    self.placeholder = placeholder
    self.shouldShowBiometrics = shouldShowBiometrics
    self.onCommit = onCommit
    self.keyringStore = keyringStore
  }

  private var biometricsIcon: Image? {
    let context = LAContext()
    if context.canEvaluatePolicy(.deviceOwnerAuthenticationWithBiometrics, error: nil) {
      switch context.biometryType {
      case .faceID:
        return Image(systemName: "faceid")
      case .touchID:
        return Image(systemName: "touchid")
      case .opticID:
        return Image(systemName: "opticid")
      case .none:
        return nil
      @unknown default:
        return nil
      }
    }
    return nil
  }

  private func fillPasswordFromKeychain() {
    if let password = keyringStore.retrievePasswordFromKeychain() {
      self.password = password
      onCommit()
    }
  }

  var body: some View {
    HStack {
      SecureField(placeholder, text: $password, onCommit: onCommit)
        .textContentType(.password)
        .font(.subheadline)
        .introspectTextField(customize: { tf in
          // Fix for animation issue when pushing SwiftUI view onto navigation
          // stack when trying to show keyboard immediately #6267 / #6297
          DispatchQueue.main.asyncAfter(deadline: .now() + 0.5) {
            tf.becomeFirstResponder()
          }
        })
        .textFieldStyle(BraveValidatedTextFieldStyle(error: error))
      if shouldShowBiometrics, keyringStore.isKeychainPasswordStored, let icon = biometricsIcon {
        Button(action: fillPasswordFromKeychain) {
          icon
            .imageScale(.large)
            .font(.headline)
            .foregroundColor(Color(.braveBlurpleTint))
        }
      }
    }
  }
}

#if DEBUG
struct PasswordEntryField_Previews: PreviewProvider {
  static var previews: some View {
    PasswordEntryField(
      password: Binding(get: { "" }, set: { _ in }),
      error: Binding(get: { nil }, set: { _ in }),
      shouldShowBiometrics: false,
      keyringStore: .previewStore,
      onCommit: {}
    )
  }
}
#endif

/// View for entering a password with an title and message displayed, and optional biometrics
struct PasswordEntryView: View {

  let keyringStore: KeyringStore
  let title: String
  let message: String
  let shouldShowBiometrics: Bool
  let action: (_ password: String, _ completion: @escaping (PasswordEntryError?) -> Void) -> Void

  init(
    keyringStore: KeyringStore,
    title: String = Strings.Wallet.confirmPasswordTitle,
    message: String,
    shouldShowBiometrics: Bool = true,
    action: @escaping (_ password: String, _ completion: @escaping (PasswordEntryError?) -> Void) ->
      Void
  ) {
    self.keyringStore = keyringStore
    self.title = title
    self.message = message
    self.shouldShowBiometrics = shouldShowBiometrics
    self.action = action
  }

  @State private var password = ""
  @State private var error: PasswordEntryError?
  @Environment(\.presentationMode) @Binding private var presentationMode

  private var isPasswordValid: Bool {
    !password.isEmpty
  }

  private func validate() {
    action(password) { entryError in
      DispatchQueue.main.async {
        if let entryError = entryError {
          self.error = entryError
          UIImpactFeedbackGenerator(style: .medium).vibrate()
        } else {
          presentationMode.dismiss()
        }
      }
    }
  }

  var body: some View {
    NavigationView {
      ScrollView(.vertical) {
        VStack(spacing: 36) {
          Image("graphic-lock", bundle: .module)
            .accessibilityHidden(true)
          VStack {
            Text(message)
              .font(.headline)
              .padding(.bottom)
              .multilineTextAlignment(.center)
              .fixedSize(horizontal: false, vertical: true)
            PasswordEntryField(
              password: $password,
              error: $error,
              shouldShowBiometrics: shouldShowBiometrics,
              keyringStore: keyringStore,
              onCommit: validate
            )
            .padding(.horizontal, 48)
          }
          Button(action: validate) {
            Text(Strings.Wallet.confirm)
          }
          .buttonStyle(BraveFilledButtonStyle(size: .normal))
          .disabled(!isPasswordValid)
        }
        .padding()
        .padding(.vertical)
        .navigationBarTitleDisplayMode(.inline)
        .navigationTitle(title)
        .toolbar {
          ToolbarItemGroup(placement: .cancellationAction) {
            Button {
              presentationMode.dismiss()
            } label: {
              Text(Strings.cancelButtonTitle)
                .foregroundColor(Color(.braveBlurpleTint))
            }
          }
        }
      }
    }
    .navigationViewStyle(.stack)
  }
}

#if DEBUG
struct PasswordEntryView_Previews: PreviewProvider {
  static var previews: some View {
    PasswordEntryView(
      keyringStore: .previewStore,
      message: String.localizedStringWithFormat(
        Strings.Wallet.removeAccountConfirmationMessage,
        "Account 1"
      ),
      action: { _, _ in }
    )
  }
}
#endif
