// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import BraveShared
import BraveUI
import Data
import Growth
import Preferences
import Strings
import SwiftUI

struct OtherPrivacySettingsSectionView: View {
  @State private var showPrivateBrowsingConfirmation = false
  @ObservedObject var settings: AdvancedShieldsSettings

  @Environment(\.openURL) private var openSettingsURL

  var body: some View {
    Section {
      OptionToggleView(
        title: Strings.privateBrowsingOnly,
        subtitle: nil,
        option: Preferences.Privacy.privateBrowsingOnly,
        onChange: { newValue in
          if newValue {
            showPrivateBrowsingConfirmation = true
          }
        }
      )
      .alert(
        isPresented: $showPrivateBrowsingConfirmation,
        content: {
          Alert(
            title: Text(Strings.privateBrowsingOnly),
            message: Text(Strings.privateBrowsingOnlyWarning),
            primaryButton: .default(
              Text(Strings.OKString),
              action: {
                Task { @MainActor in
                  try await Task.sleep(nanoseconds: NSEC_PER_MSEC * 100)

                  Preferences.Privacy.persistentPrivateBrowsing.value = false
                  await settings.clearPrivateData([CookiesAndCacheClearable()])

                  // First remove all tabs so that only a blank tab exists.
                  settings.tabManager.removeAll()

                  // Reset tab configurations and delete all webviews..
                  settings.tabManager.reset()

                  // Restore all existing tabs by removing the blank tabs and recreating new ones..
                  settings.tabManager.removeAll()
                }
              }
            ),
            secondaryButton: .cancel(
              Text(Strings.cancelButtonTitle),
              action: {
                Preferences.Privacy.privateBrowsingOnly.value = false
              }
            )
          )
        }
      )
      ToggleView(
        title: Strings.Shields.blockMobileAnnoyances,
        subtitle: nil,
        toggle: $settings.blockMobileAnnoyances
      )
      OptionToggleView(
        title: Strings.followUniversalLinks,
        subtitle: nil,
        option: Preferences.General.followUniversalLinks
      )
      OptionToggleView(
        title: Strings.googleSafeBrowsing,
        subtitle: String.localizedStringWithFormat(
          Strings.googleSafeBrowsingUsingWebKitDescription,
          URL.brave.safeBrowsingHelp.absoluteString
        ),
        option: Preferences.Shields.googleSafeBrowsing
      )
      if !ProcessInfo.processInfo.isiOSAppOnVisionOS {
        // Vision OS does not support ScreenTime for web pages
        OptionToggleView(
          title: Strings.screenTimeSetting,
          subtitle: String.localizedStringWithFormat(
            Strings.screenTimeSettingDescription,
            URL.brave.screenTimeHelp.absoluteString
          ),
          option: Preferences.Privacy.screenTimeEnabled
        )
      }
      ToggleView(
        title: Strings.P3A.settingTitle,
        subtitle: Strings.P3A.settingSubtitle,
        toggle: $settings.isP3AEnabled
      )
      OptionToggleView(
        title: Strings.Settings.sendUsagePingTitle,
        subtitle: Strings.Settings.sendUsagePingDescription,
        option: Preferences.DAU.sendUsagePing
      )
    } header: {
      Text(Strings.otherPrivacySettingsSection)
    }
  }
}
