// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import DesignSystem
import LocalAuthentication
import SwiftUI

import struct Shared.AppConstants

struct BiometricView: View {
  var keyringStore: KeyringStore
  var completion: () -> Void

  @State private var biometricError: OSStatus?

  private var biometricsIcon: Image? {
    let context = LAContext()
    if context.canEvaluatePolicy(.deviceOwnerAuthenticationWithBiometrics, error: nil) {
      switch context.biometryType {
      case .faceID:
        return Image(braveSystemName: "leo.face.id")
      case .touchID:
        return Image(braveSystemName: "leo.biometric.login")
      case .none, .opticID:
        return nil
      @unknown default:
        return nil
      }
    }
    return nil
  }

  private var biometricName: String {
    let context = LAContext()
    if context.canEvaluatePolicy(.deviceOwnerAuthenticationWithBiometrics, error: nil) {
      switch context.biometryType {
      case .faceID:
        return Strings.Wallet.biometricsSetupFaceId
      case .touchID:
        return Strings.Wallet.biometricsSetupTouchId
      default:
        return ""
      }
    }
    return ""
  }

  var body: some View {
    GeometryReader { geometry in
      ScrollView {
        VStack {
          ZStack {
            Circle()
              .strokeBorder(Color(.braveInfoBorder).opacity(0.3))
              .background(
                Circle()
                  .foregroundColor(Color(.braveInfoBackground).opacity(0.5))
              )
              .frame(width: 240, height: 240)
            Rectangle()
              .frame(width: 96, height: 96)
              .foregroundColor(Color(.braveBackground))
              .cornerRadius(20)
            if let biometricsIcon {
              biometricsIcon
                .resizable()
                .aspectRatio(contentMode: .fit)
                .frame(width: 52, height: 52)
                .foregroundColor(Color(.braveInfoLabel))
            }
          }
          Group {
            Text(
              String.localizedStringWithFormat(Strings.Wallet.biometricsSetupTitle, biometricName)
            )
            .font(.title)
            .foregroundColor(.primary)
            .padding(.bottom, 10)
            Text(
              String.localizedStringWithFormat(
                Strings.Wallet.biometricsSetupSubTitle,
                biometricName
              )
            )
            .font(.body)
            .foregroundColor(Color(.secondaryBraveLabel))
          }
          .fixedSize(horizontal: false, vertical: true)
          .multilineTextAlignment(.center)
          .padding(.top, 28)
          Button {
            // Store password in keychain
            if let password = keyringStore.passwordToSaveInBiometric,
              case let status = keyringStore.storePasswordInKeychain(password),
              status != errSecSuccess
            {
              biometricError = status
            } else {
              keyringStore.passwordToSaveInBiometric = nil
              completion()
            }
          } label: {
            Text(
              String.localizedStringWithFormat(
                Strings.Wallet.biometricsSetupEnableButtonTitle,
                biometricName
              )
            )
            .frame(maxWidth: .infinity)
          }
          .buttonStyle(BraveFilledButtonStyle(size: .large))
          .padding(.top, 80)
          Button {
            keyringStore.passwordToSaveInBiometric = nil
            completion()
          } label: {
            Text(Strings.Wallet.skipButtonTitle)
              .font(Font.subheadline.weight(.medium))
              .foregroundColor(Color(.braveLabel))
          }
          .padding(.top, 20)
        }
        .frame(maxWidth: .infinity, minHeight: geometry.size.height)
        .padding(.horizontal, 24)
        .padding(.bottom, 40)
      }
      .alert(
        isPresented: Binding(
          get: { biometricError != nil },
          set: { _, _ in }
        )
      ) {
        Alert(
          title: Text(Strings.Wallet.biometricsSetupErrorTitle),
          message: Text(
            Strings.Wallet.biometricsSetupErrorMessage
              + (AppConstants.isOfficialBuild ? "" : " (\(biometricError ?? -1))")
          ),
          dismissButton: .default(
            Text(Strings.OKString),
            action: {
              keyringStore.passwordToSaveInBiometric = nil
              completion()
            }
          )
        )
      }
      .interactiveDismissDisabled()
      .transparentNavigationBar()
    }
  }
}

#if DEBUG
struct BiometricView_Previews: PreviewProvider {
  static var previews: some View {
    BiometricView(
      keyringStore: .previewStore,
      completion: {}
    )
  }
}
#endif
