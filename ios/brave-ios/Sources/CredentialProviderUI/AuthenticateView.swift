// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import DesignSystem
import Foundation
import LocalAuthentication
import Strings
import SwiftUI

struct AuthenticateView: View {
  @Binding var isAuthenticated: Bool
  var onCancel: () -> Void

  @State private var isManualUnlockVisible = false
  @State private var isShowingSetPasscodeAlert = false

  var body: some View {
    ZStack(alignment: .bottom) {
      Image(braveSystemName: "leo.key.lock")
        .font(.system(size: 100))
        .foregroundStyle(LinearGradient(braveSystemName: .heroLegacy))
        .frame(maxWidth: .infinity, maxHeight: .infinity)
      if isManualUnlockVisible {
        VStack(spacing: 16) {
          Button {
            performAuthentication()
          } label: {
            Text(Strings.CredentialProvider.authenticationUnlockButtonTitle)
              .frame(minWidth: 230)
          }
          .buttonStyle(BraveFilledButtonStyle(size: .large))
          Button {
            onCancel()
          } label: {
            Text(Strings.CredentialProvider.cancelButtonTitle)
              .frame(minWidth: 230)
          }
          .buttonStyle(BraveOutlineButtonStyle(size: .large))
        }
        .transition(.opacity.animation(.default))
      }
    }
    .padding(.horizontal, 20)
    .padding(.vertical, 80)
    .background(.thickMaterial)
    .onAppear {
      performAuthentication()
    }
    .alert(isPresented: $isShowingSetPasscodeAlert) {
      Alert(
        title: Text(Strings.CredentialProvider.setPasscodeAlertTitle),
        message: Text(Strings.CredentialProvider.setPasscodeAlertMessage),
        dismissButton: .cancel(onCancel)
      )
    }
  }

  private func performAuthentication() {
    let context = LAContext()
    if !context.canEvaluatePolicy(.deviceOwnerAuthentication, error: nil) {
      isShowingSetPasscodeAlert = true
      return
    }
    context.evaluatePolicy(
      .deviceOwnerAuthentication,
      localizedReason: Strings.CredentialProvider.authenticationReason
    ) { success, error in
      DispatchQueue.main.async { [self] in
        isAuthenticated = success
        if !success {
          isManualUnlockVisible = true
        }
      }
    }
  }
}

#if DEBUG
#Preview {
  AuthenticateView(isAuthenticated: .constant(false), onCancel: {})
}
#endif
