// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import SwiftUI
import BraveUI
import LocalAuthentication
import Shared

struct PasscodeMigrationContainerView: View {
    var dismiss: ((_ enableBrowserLock: Bool) -> Void)?
    @State private var context = LAContext()
    
    var body: some View {
        let isUserAuthenticationAvailable = context
            .canEvaluatePolicy(.deviceOwnerAuthentication, error: nil)
        let biometryType = context.biometryType
        PasscodeMigrationView(
            isUserAuthenticationAvailable: isUserAuthenticationAvailable,
            availableBiometryMethod: biometryType
        ) {
            if context.canEvaluatePolicy(.deviceOwnerAuthentication, error: nil) {
                // User must authenticate to continue
                context.evaluatePolicy(
                    .deviceOwnerAuthentication,
                    localizedReason: Strings.authenticationLoginsTouchReason
                ) { success, error in
                    if success {
                        DispatchQueue.main.async {
                            dismiss?(true)
                        }
                    }
                }
            } else {
                dismiss?(false)
            }
        }
        .onReceive(
            NotificationCenter.default.publisher(
                for: UIApplication.willEnterForegroundNotification
            )
        ) { _ in
            // Create a new `LAContext` context if the user leaves the app during this time
            context = LAContext()
        }
    }
}

/// Displays information regarding the migration from the old passcode system to the new
/// version which uses the users passcode instead of a custom one
private struct PasscodeMigrationView: View {
    /// Whether or not the user has at the very least a PIN set on their device
    var isUserAuthenticationAvailable: Bool
    /// The type of biometry authentication type is available (effecting copy)
    var availableBiometryMethod: LABiometryType
    /// The action performed when the user taps the continue button
    var continueAction: () -> Void
    
    var body: some View {
        VStack(spacing: 60) {
            Image("pin-migration-graphic")
                .resizable()
                .aspectRatio(contentMode: .fit)
                .frame(maxWidth: 343.5, maxHeight: 317)
            VStack(spacing: 12) {
                Text(Strings.browserLockMigrationTitle)
                    .font(.headline)
                    .foregroundColor(Color(.braveLabel))
                Text(Strings.browserLockMigrationSubtitle)
                    .font(.subheadline)
                    .foregroundColor(Color(.braveLabel))
                if !isUserAuthenticationAvailable {
                    Text(settingsAuthenticationString)
                        .font(.subheadline)
                        .foregroundColor(Color(.secondaryBraveLabel))
                }
                Button(action: continueAction) {
                    Text(Strings.browserLockMigrationContinueButtonTitle)
                        .frame(maxWidth: .infinity)
                        .padding(.vertical, 4)
                }
                .buttonStyle(BraveFilledButtonStyle(size: .normal))
                .padding(.top)
            }
            .fixedSize(horizontal: false, vertical: true)
        }
        .multilineTextAlignment(.center)
        .padding(32)
        .accessibilityEmbedInScrollView()
    }
    
    private var settingsAuthenticationString: String {
        let settingsPath: String = {
            switch availableBiometryMethod {
            case .faceID:
                return Strings.authenticationFaceIDPasscodeSetting
            case .touchID:
                return Strings.authenticationTouchIDPasscodeSetting
            case .none:
                return Strings.authenticationPasscode
            @unknown default:
                return Strings.authenticationPasscode
            }
        }()
        return String(format: Strings.browserLockMigrationNoPasscodeSetup, settingsPath)
    }
}

struct PasscodeMigrationView_Previews: PreviewProvider {
    static var previews: some View {
        Group {
            ForEach([true, false], id: \.self) { isUserAuthenticationAvailable in
                ForEach([LABiometryType.none, .faceID, .touchID], id: \.self) { availableBiometryMethod in
                    PasscodeMigrationView(
                        isUserAuthenticationAvailable: isUserAuthenticationAvailable,
                        availableBiometryMethod: availableBiometryMethod,
                        continueAction: { }
                    )
                }
            }
            .previewLayout(.sizeThatFits)
            PasscodeMigrationView(
                isUserAuthenticationAvailable: false,
                availableBiometryMethod: .faceID,
                continueAction: { }
            )
            .environment(\.sizeCategory, .accessibilityExtraExtraLarge)
        }
    }
}
