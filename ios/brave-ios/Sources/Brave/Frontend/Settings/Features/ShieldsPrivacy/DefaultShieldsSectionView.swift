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
  @ObservedObject var settings: AdvancedShieldsSettings

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

      OptionToggleView(
        title: Strings.fingerprintingProtection,
        subtitle: Strings.fingerprintingProtectionDescription,
        option: Preferences.Shields.fingerprintingProtection
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
