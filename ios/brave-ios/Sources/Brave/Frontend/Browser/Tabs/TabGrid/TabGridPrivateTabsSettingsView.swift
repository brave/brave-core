// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveStrings
import LocalAuthentication
import Preferences
import Strings
import SwiftUI

struct TabGridPrivateTabsSettings: View {
  var viewModel: TabGridViewModel

  @State private var context: LAContext = .init()
  @Environment(\.dismiss) private var dismiss
  @ObservedObject private var persistentPrivateBrowsing = Preferences.Privacy
    .persistentPrivateBrowsing
  @ObservedObject private var privateBrowsingLock = Preferences.Privacy.privateBrowsingLock

  private var authenticationKind: LABiometryType? {
    if context.canEvaluatePolicy(.deviceOwnerAuthentication, error: nil) {
      return context.biometryType
    }
    return nil
  }

  private var privateBrowsingLockBinding: Binding<Bool> {
    .init {
      privateBrowsingLock.value
    } set: { newValue in
      // Avoiding the use of WindowProtection on purpose in this contxt because its being used
      // to control private mode in general inside of the tab tray and we really only need to it
      // flip a pref here, not control the visibility of the window.
      if !newValue, context.canEvaluatePolicy(.deviceOwnerAuthentication, error: nil) {
        context.evaluatePolicy(
          .deviceOwnerAuthentication,
          localizedReason: Strings.authenticationLoginsTouchReason
        ) { success, _ in
          DispatchQueue.main.async { [self] in
            privateBrowsingLock.value = !success
          }
        }
      } else {
        privateBrowsingLock.value = newValue
      }
    }
  }

  var body: some View {
    NavigationStack {
      Form {
        Toggle(isOn: $persistentPrivateBrowsing.value) {
          Label {
            Text(Strings.TabsSettings.persistentPrivateBrowsingTitle)
              .foregroundStyle(Color(braveSystemName: .textPrimary))
            Text(Strings.TabsSettings.persistentPrivateBrowsingDescription)
              .foregroundStyle(Color(braveSystemName: .textTertiary))
              .font(.footnote)
          } icon: {
            Image(braveSystemName: "leo.browser.mobile-tabs")
              .foregroundStyle(Color(braveSystemName: .iconDefault))
          }
        }
        .tint(Color.accentColor)
        .listRowBackground(Color(uiColor: .secondaryBraveGroupedBackground))
        if let authenticationKind {
          Toggle(isOn: privateBrowsingLockBinding) {
            Label {
              let (title, body) =
                switch authenticationKind {
                case .faceID:
                  (
                    Strings.TabsSettings.privateBrowsingLockTitleFaceID,
                    Strings.TabsSettings.privateBrowsingLockDescriptionFaceID
                  )
                case .touchID:
                  (
                    Strings.TabsSettings.privateBrowsingLockTitleTouchID,
                    Strings.TabsSettings.privateBrowsingLockDescriptionTouchID
                  )
                default:
                  (
                    Strings.TabsSettings.privateBrowsingLockTitlePinCode,
                    Strings.TabsSettings.privateBrowsingLockDescriptionPinCode
                  )
                }
              Text(title)
                .foregroundStyle(Color(braveSystemName: .textPrimary))
              Text(body)
                .foregroundStyle(Color(braveSystemName: .textTertiary))
                .font(.footnote)
            } icon: {
              Group {
                switch authenticationKind {
                case .faceID:
                  Image(braveSystemName: "leo.face.id")
                case .touchID:
                  Image(braveSystemName: "leo.biometric.login")
                default:
                  Image(braveSystemName: "leo.lock")
                }
              }
              .foregroundStyle(Color(braveSystemName: .iconDefault))
            }
          }
          .tint(Color.accentColor)
          .listRowBackground(Color(uiColor: .secondaryBraveGroupedBackground))
        }
      }
      .scrollContentBackground(.hidden)
      .background(Color(uiColor: .braveGroupedBackground))
      .navigationTitle(Strings.TabGrid.privateTabsSettingsTitle)
      .navigationBarTitleDisplayMode(.inline)
      .toolbar {
        ToolbarItemGroup(placement: .confirmationAction) {
          Button(Strings.done) {
            dismiss()
          }
          .foregroundStyle(Color(braveSystemName: .textPrimary))
        }
      }
    }
  }
}
