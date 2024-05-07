// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveShields
import BraveUI
import Data
import DesignSystem
import Preferences
import Strings
import SwiftUI

struct DefaultShieldsViewView: View {
  enum CookieAlertType: String, Identifiable {
    case confirm
    case failed

    var id: String { rawValue }
  }

  @ObservedObject var settings: AdvancedShieldsSettings
  @State private var cookieAlertType: CookieAlertType?

  var body: some View {
    Section {
      Picker(selection: $settings.adBlockAndTrackingPreventionLevel) {
        ForEach(ShieldLevel.allCases) { level in
          Text(level.localizedTitle)
            .foregroundColor(Color(.secondaryBraveLabel))
            .tag(level)
        }
      } label: {
        LabelView(
          title: Strings.Shields.trackersAndAdsBlocking,
          subtitle: Strings.Shields.trackersAndAdsBlockingDescription
        )
      }
      .listRowBackground(Color(.secondaryBraveGroupedBackground))

      Picker(selection: $settings.httpsUpgradeLevel) {
        ForEach(HTTPSUpgradeLevel.allCases) { level in
          Text(level.localizedTitle)
            .foregroundColor(Color(.secondaryBraveLabel))
            .tag(level)
        }
      } label: {
        LabelView(
          title: Strings.Shields.upgradeConnectionsToHTTPS,
          subtitle: nil
        )
      }
      .listRowBackground(Color(.secondaryBraveGroupedBackground))

      ToggleView(
        title: Strings.autoRedirectAMPPages,
        subtitle: Strings.autoRedirectAMPPagesDescription,
        toggle: $settings.isDeAmpEnabled
      )
      ToggleView(
        title: Strings.autoRedirectTrackingURLs,
        subtitle: Strings.autoRedirectTrackingURLsDescription,
        toggle: $settings.isDebounceEnabled
      )
      OptionToggleView(
        title: Strings.blockScripts,
        subtitle: Strings.blockScriptsDescription,
        option: Preferences.Shields.blockScripts
      )

      OptionToggleView(
        title: Strings.blockAllCookies,
        subtitle: Strings.blockCookiesDescription,
        option: Preferences.Privacy.blockAllCookies,
        onChange: { newValue in
          if newValue {
            cookieAlertType = .confirm
          } else {
            toggleCookieSetting(with: false)
          }
        }
      )
      .alert(item: $cookieAlertType) { cookieAlertType in
        switch cookieAlertType {
        case .confirm:
          return Alert(
            title: Text(Strings.blockAllCookiesAction),
            message: Text(Strings.blockAllCookiesAlertInfo),
            primaryButton: .default(
              Text(Strings.blockAllCookiesAction),
              action: {
                toggleCookieSetting(with: true)
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

      OptionToggleView(
        title: Strings.fingerprintingProtection,
        subtitle: Strings.fingerprintingProtectionDescription,
        option: Preferences.Shields.fingerprintingProtection
      )

      OptionToggleView(
        title: Strings.Shields.enableGPCLabel,
        subtitle: Strings.Shields.enableGPCDescription,
        option: ShieldPreferences.enableGPC
      )

      ToggleView(
        title: Strings.blockCookieConsentNotices,
        subtitle: nil,
        toggle: $settings.cookieConsentBlocking
      )

      NavigationLink {
        FilterListsView()
      } label: {
        LabelView(
          title: Strings.Shields.contentFiltering,
          subtitle: Strings.Shields.contentFilteringDescription
        )
      }.listRowBackground(Color(.secondaryBraveGroupedBackground))
    } header: {
      Text(Strings.shieldsDefaults)
    } footer: {
      Text(Strings.shieldsDefaultsFooter)
    }
  }

  private func toggleCookieSetting(with status: Bool) {
    let success = FileManager.default.setFolderAccess([
      (.cookie, status),
      (.webSiteData, status),
    ])

    if success {
      if Preferences.Privacy.blockAllCookies.value != status {
        Preferences.Privacy.blockAllCookies.value = status
      }
    } else if status {
      // Revert the changes. Not handling success here to avoid a loop.
      FileManager.default.setFolderAccess([
        (.cookie, false),
        (.webSiteData, false),
      ])

      if Preferences.Privacy.blockAllCookies.value != false {
        Preferences.Privacy.blockAllCookies.value = false
      }

      cookieAlertType = .failed
    }
  }
}

extension ShieldLevel: Identifiable {
  public var id: String {
    return rawValue
  }

  public var localizedTitle: String {
    switch self {
    case .aggressive: return Strings.Shields.trackersAndAdsBlockingAggressive
    case .disabled: return Strings.Shields.trackersAndAdsBlockingDisabled
    case .standard: return Strings.Shields.trackersAndAdsBlockingStandard
    }
  }
}

extension HTTPSUpgradeLevel: Identifiable {
  public var id: String {
    return rawValue
  }

  public var localizedTitle: String {
    switch self {
    case .strict: return Strings.Shields.httpsUpgradeLevelStrict
    case .disabled: return Strings.Shields.trackersAndAdsBlockingDisabled
    case .standard: return Strings.Shields.trackersAndAdsBlockingStandard
    }
  }
}
