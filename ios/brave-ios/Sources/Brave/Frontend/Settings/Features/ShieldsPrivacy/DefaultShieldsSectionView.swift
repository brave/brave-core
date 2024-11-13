// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShared
import BraveShields
import BraveUI
import Data
import DesignSystem
import OSLog
import Preferences
import Strings
import SwiftUI

struct DefaultShieldsSectionView: View {
  enum CookieAlertType: String, Identifiable {
    case confirm
    case failed

    var id: String { rawValue }
  }

  @ObservedObject var settings: AdvancedShieldsSettings
  @State private var cookieAlertType: CookieAlertType?
  /// If we should force show the Block All Cookies row.
  /// If a user disables the toggle with the feature flag disabled, we don't
  /// want the row to disappear on the user. So if we are showing the row
  /// when the view opens, it should remain visible until the view is dismissed.
  @State private var showBlockAllCookies = false

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

      if FeatureList.kBraveHttpsByDefault.enabled {
        if FeatureList.kHttpsOnlyMode.enabled {
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
        } else {
          ToggleView(
            title: Strings.Shields.upgradeConnectionsToHTTPS,
            toggle: Binding(
              get: {
                settings.httpsUpgradeLevel.isEnabled
              },
              set: { newValue in
                settings.httpsUpgradeLevel =
                  !newValue
                  ? .disabled : (ShieldPreferences.httpsUpgradePriorEnabledLevel ?? .standard)
              }
            )
          )
        }
      } else {
        ToggleView(
          title: Strings.HTTPSEverywhere,
          subtitle: Strings.HTTPSEverywhereDescription,
          toggle: Binding(
            get: {
              settings.httpsUpgradeLevel.isEnabled
            },
            set: { newValue in
              settings.httpsUpgradeLevel =
                !newValue
                ? .disabled : (ShieldPreferences.httpsUpgradePriorEnabledLevel ?? .standard)
            }
          )
        )
      }

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

      if showBlockAllCookies || FeatureList.kBlockAllCookiesToggle.enabled
        || Preferences.Privacy.blockAllCookies.value
      {
        OptionToggleView(
          title: Strings.blockAllCookies,
          subtitle: Strings.blockCookiesDescription,
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
              title: Text(Strings.blockAllCookiesAction),
              message: Text(Strings.blockAllCookiesAlertInfo),
              primaryButton: .default(
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

      ToggleView(
        title: Strings.braveShieldsSaveContactInfo,
        subtitle: Strings.braveShieldsSaveContactInfoDescription,
        toggle: $settings.isSaveContactInfoEnabled
      )

      if FeatureList.kBraveShredFeature.enabled {
        Picker(selection: $settings.shredLevel) {
          ForEach(SiteShredLevel.allCases) { level in
            Text(level.localizedTitle)
              .foregroundColor(Color(.secondaryBraveLabel))
              .tag(level)
          }
        } label: {
          LabelView(
            title: Strings.Shields.autoShred,
            subtitle: nil
          )
        }
      }

      NavigationLink {
        FilterListsView()
      } label: {
        LabelView(
          title: Strings.Shields.contentFiltering,
          subtitle: Strings.Shields.contentFilteringDescription
        )
      }
    } header: {
      Text(Strings.shieldsDefaults)
    } footer: {
      Text(Strings.shieldsDefaultsFooter)
    }
    .listRowBackground(Color(.secondaryBraveGroupedBackground))
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
