// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import SwiftUI
import Strings
import Data
import DesignSystem
import BraveUI
import Preferences
import BraveShields

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
        ShieldLabelView(
          title: Strings.Shields.trackersAndAdsBlocking,
          subtitle: Strings.Shields.trackersAndAdsBlockingDescription
        )
      }
      .listRowBackground(Color(.secondaryBraveGroupedBackground))
      
      OptionToggleView(
        title: Strings.HTTPSEverywhere,
        subtitle: Strings.HTTPSEverywhereDescription,
        option: Preferences.Shields.httpsEverywhere
      )
      OptionToggleView(
        title: Strings.autoRedirectAMPPages,
        subtitle: Strings.autoRedirectAMPPagesDescription,
        option: Preferences.Shields.autoRedirectAMPPages
      )
      OptionToggleView(
        title: Strings.autoRedirectTrackingURLs,
        subtitle: Strings.autoRedirectTrackingURLsDescription,
        option: Preferences.Shields.autoRedirectTrackingURLs
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
            primaryButton: .default(Text(Strings.blockAllCookiesAction), action: {
              toggleCookieSetting(with: true)
            }),
            secondaryButton: .cancel(Text(Strings.cancelButtonTitle), action: {
              Preferences.Privacy.blockAllCookies.value = false
            })
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
      
      ShieldToggleView(
        title: Strings.blockCookieConsentNotices,
        subtitle: nil,
        toggle: $settings.cookieConsentBlocking
      )
      
      NavigationLink {
        FilterListsView()
      } label: {
        ShieldLabelView(
          title: Strings.contentFiltering,
          subtitle: Strings.contentFilteringDescription
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
