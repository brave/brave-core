// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShared
import BraveShields
import BraveUI
import Data
import Growth
import OSLog
import Preferences
import Strings
import SwiftUI

struct OtherPrivacySettingsSectionView: View {
  private enum CookieAlertType: String, Identifiable {
    case confirm
    case failed

    var id: String { rawValue }
  }

  @State private var showPrivateBrowsingConfirmation = false
  @ObservedObject var settings: AdvancedShieldsSettings
  @State private var cookieAlertType: CookieAlertType?
  /// If we should force show the Block All Cookies row.
  /// If a user disables the toggle with the feature flag disabled, we don't
  /// want the row to disappear on the user. So if we are showing the row
  /// when the view opens, it should remain visible until the view is dismissed.
  @State private var showBlockAllCookies = false

  @Environment(\.openURL) private var openSettingsURL

  var body: some View {
    Section {
      OptionToggleView(
        title: Strings.Shields.enableGPCLabel,
        subtitle: Strings.Shields.enableGPCDescription,
        option: ShieldPreferences.enableGPC
      )
      if showBlockAllCookies || FeatureList.kBlockAllCookiesToggle.enabled
        || Preferences.Privacy.blockAllCookies.value
      {
        OptionToggleView(
          title: Strings.blockAllCookies,
          subtitle: Strings.blockAllCookiesDescription,
          option: Preferences.Privacy.blockAllCookies,
          onChange: { newValue in
            if newValue {
              cookieAlertType = .confirm
            } else {
              Task {
                await toggleCookieSetting(with: false)
              }
            }
          }
        )
        .alert(item: $cookieAlertType) { cookieAlertType in
          switch cookieAlertType {
          case .confirm:
            return Alert(
              title: Text(Strings.blockAllCookiesAlertTitle),
              message: Text(Strings.blockAllCookiesDescription),
              primaryButton: .destructive(
                Text(Strings.blockAllCookiesAction),
                action: {
                  Task {
                    await toggleCookieSetting(with: true)
                  }
                }
              ),
              secondaryButton: .cancel(
                Text(Strings.cancelButtonTitle),
                action: {
                  Preferences.Privacy.blockAllCookies.value = false
                }
              )
            )
          case .failed:
            return Alert(
              title: Text(Strings.blockAllCookiesFailedAlertMsg)
            )
          }
        }
      }
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
    .onAppear {
      showBlockAllCookies = Preferences.Privacy.blockAllCookies.value
    }
  }

  private func toggleCookieSetting(with status: Bool) async {
    do {
      try await AsyncFileManager.default.setWebDataAccess(atPath: .cookie, lock: status)
      try await AsyncFileManager.default.setWebDataAccess(atPath: .websiteData, lock: status)

      if Preferences.Privacy.blockAllCookies.value != status {
        Preferences.Privacy.blockAllCookies.value = status
      }
    } catch {
      Logger.module.error("Failed to change web data access to \(status)")
      if status {
        // Revert the changes. Not handling success here to avoid a loop.
        try? await AsyncFileManager.default.setWebDataAccess(atPath: .cookie, lock: false)
        try? await AsyncFileManager.default.setWebDataAccess(atPath: .websiteData, lock: false)

        if Preferences.Privacy.blockAllCookies.value != false {
          Preferences.Privacy.blockAllCookies.value = false
        }

        cookieAlertType = .failed
      }
    }
  }
}
