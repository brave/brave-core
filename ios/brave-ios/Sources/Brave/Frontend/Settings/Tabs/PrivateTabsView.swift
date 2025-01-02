// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveStrings
import BraveUI
import Data
import LocalAuthentication
import Preferences
import SwiftUI

struct PrivateTabsView: View {
  enum AuthenticationType {
    case faceID, touchID, pinCode, noAuthentication
  }

  @ObservedObject var privateBrowsingOnly = Preferences.Privacy.privateBrowsingOnly
  @ObservedObject var privateBrowsingLock = Preferences.Privacy.privateBrowsingLock

  var tabManager: TabManager?
  var askForAuthentication: (AuthViewType, ((Bool, LAError.Code?) -> Void)?) -> Void

  private var localAuthenticationType: AuthenticationType {
    let context = LAContext()

    if context.canEvaluatePolicy(.deviceOwnerAuthenticationWithBiometrics, error: nil) {
      switch context.biometryType {
      case .faceID:
        return .faceID
      case .touchID:
        return .touchID
      default:
        return .noAuthentication
      }
    }

    var error: NSError?
    let policyEvaluation = context.canEvaluatePolicy(.deviceOwnerAuthentication, error: &error)

    if policyEvaluation {
      return .pinCode
    }

    return .noAuthentication
  }

  private var browsingLockTitle: String {
    var title: String

    switch localAuthenticationType {
    case .faceID:
      title = Strings.TabsSettings.privateBrowsingLockTitleFaceID
    case .touchID:
      title = Strings.TabsSettings.privateBrowsingLockTitleTouchID
    default:
      title = Strings.TabsSettings.privateBrowsingLockTitlePinCode
    }

    return title
  }

  var body: some View {
    Form {
      Section(
        header: Text(Strings.TabsSettings.privateTabsSettingsTitle.uppercased()),
        footer: privateBrowsingOnly.value
          ? Text("") : Text(Strings.TabsSettings.persistentPrivateBrowsingDescription)
      ) {
        if !privateBrowsingOnly.value {
          OptionToggleView(
            title: Strings.TabsSettings.persistentPrivateBrowsingTitle,
            subtitle: nil,
            option: Preferences.Privacy.persistentPrivateBrowsing
          ) { newValue in
            Task { @MainActor in
              if newValue {
                tabManager?.saveAllTabs()
              } else {
                tabManager?.removeAllForCurrentMode(isActiveTabIncluded: true)
              }
            }
          }
        }

        switch localAuthenticationType {
        case .faceID, .touchID, .pinCode:
          ToggleView(
            title: browsingLockTitle,
            subtitle: nil,
            toggle: .init(
              get: {
                privateBrowsingLock.value
              },
              set: { isOn in
                if isOn {
                  privateBrowsingLock.value = true
                } else {
                  askForAuthentication(.general) { success, _ in
                    privateBrowsingLock.value = !success
                  }
                }
              }
            )
          )
        case .noAuthentication:
          Toggle(isOn: .constant(false)) {
            VStack(alignment: .leading, spacing: 4) {
              Text(browsingLockTitle)
                .foregroundColor(Color(.bravePrimary))
            }
            .opacity(0.25)
          }
          .disabled(true)
          .listRowBackground(Color(.secondaryBraveGroupedBackground))
          .toggleStyle(SwitchToggleStyle(tint: .accentColor))
        }
      }
      .listRowBackground(Color(.secondaryBraveGroupedBackground))
    }
    .navigationBarTitle(Strings.TabsSettings.privateTabsSettingsTitle)
    .navigationBarTitleDisplayMode(.inline)
    .listBackgroundColor(Color(UIColor.braveGroupedBackground))
  }
}

#if DEBUG
struct PrivateTabsView_Previews: PreviewProvider {
  static var previews: some View {
    PrivateTabsView(askForAuthentication: { _, _ in })
  }
}
#endif
