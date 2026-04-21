// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveStrings
import BraveUI
import Preferences
import SwiftUI

struct MediaSettingsView: View {
  @ObservedObject var enableBackgroundAudio = Preferences.General.mediaAutoBackgrounding
  @ObservedObject var keepYouTubeInBrave = Preferences.General.keepYouTubeInBrave
  @ObservedObject var filterListStorage = FilterListStorage.shared

  @State var youtubeRecommendationsBlocking = false
  @State var youtubeDistractingElementsBlocking = false
  @State var youtubeShortsBlocking = false

  var prefs: any PrefService

  var body: some View {
    Form {
      Section(header: Text(Strings.Settings.mediaGeneralSection)) {
        Toggle(isOn: $enableBackgroundAudio.value) {
          Text(Strings.mediaAutoBackgrounding)
        }
        .toggleStyle(SwitchToggleStyle(tint: .accentColor))
      }
      .listRowBackground(Color(.secondaryBraveGroupedBackground))

      Section(header: Text(Strings.Settings.youtube)) {
        Toggle(isOn: $keepYouTubeInBrave.value) {
          Text(Strings.Settings.openYouTubeInBrave)
        }
        .toggleStyle(SwitchToggleStyle(tint: .accentColor))
        NavigationLink(destination: QualitySettingsView(prefs: prefs)) {
          VStack(alignment: .leading) {
            Text(Strings.Settings.highestQualityPlayback)
            Text(Strings.Settings.highestQualityPlaybackDetail)
              .font(.footnote)
              .foregroundColor(Color(.secondaryBraveLabel))
          }
        }
        Toggle(isOn: $youtubeRecommendationsBlocking) {
          VStack(alignment: .leading) {
            Text(Strings.Settings.blockYoutubeRecommendationsTitle)
            Text(Strings.Settings.blockYoutubeRecommendationsDesc)
              .font(.footnote)
              .foregroundColor(Color(.secondaryBraveLabel))
          }
        }
        .toggleStyle(SwitchToggleStyle(tint: .accentColor))
        Toggle(isOn: $youtubeDistractingElementsBlocking) {
          VStack(alignment: .leading) {
            Text(Strings.Settings.blockYouTubeDistractingElementsTitle)
            Text(Strings.Settings.blockYouTubeDistractingElementsDesc)
              .font(.footnote)
              .foregroundColor(Color(.secondaryBraveLabel))
          }
        }
        .toggleStyle(SwitchToggleStyle(tint: .accentColor))
        Toggle(isOn: $youtubeShortsBlocking) {
          VStack(alignment: .leading) {
            Text(Strings.Settings.blockYouTubeShortsTitle)
            Text(Strings.Settings.blockYouTubeShortsDesc)
              .font(.footnote)
              .foregroundColor(Color(.secondaryBraveLabel))
          }
        }
        .toggleStyle(SwitchToggleStyle(tint: .accentColor))
      }
      .listRowBackground(Color(.secondaryBraveGroupedBackground))
    }
    .navigationBarTitle(Strings.Settings.mediaRootSetting)
    .navigationBarTitleDisplayMode(.inline)
    .background(Color(UIColor.braveGroupedBackground))
    .task {
      // Update the enabled state filter lists
      self.youtubeDistractingElementsBlocking = FilterListStorage.shared.isEnabled(
        for: AdblockFilterListCatalogEntry.youtubeDistractingElementsComponentID
      )
      self.youtubeRecommendationsBlocking = FilterListStorage.shared.isEnabled(
        for: AdblockFilterListCatalogEntry.youtubeMobileRecommendationsComponentID
      )
      self.youtubeShortsBlocking = FilterListStorage.shared.isEnabled(
        for: AdblockFilterListCatalogEntry.youtubeShortsComponentID
      )
    }
    .onChange(of: youtubeRecommendationsBlocking) { _, newValue in
      FilterListStorage.shared.ensureFilterList(
        for: AdblockFilterListCatalogEntry.youtubeMobileRecommendationsComponentID,
        isEnabled: newValue
      )
    }
    .onChange(of: youtubeDistractingElementsBlocking) { _, newValue in
      FilterListStorage.shared.ensureFilterList(
        for: AdblockFilterListCatalogEntry.youtubeDistractingElementsComponentID,
        isEnabled: newValue
      )
    }
    .onChange(of: youtubeShortsBlocking) { _, newValue in
      FilterListStorage.shared.ensureFilterList(
        for: AdblockFilterListCatalogEntry.youtubeShortsComponentID,
        isEnabled: newValue
      )
    }
    .onChange(of: filterListStorage.filterLists) { _, newValue in
      for filterList in newValue {
        switch filterList.entry.componentId {
        case AdblockFilterListCatalogEntry.youtubeDistractingElementsComponentID:
          if filterList.isEnabled != self.youtubeDistractingElementsBlocking {
            self.youtubeDistractingElementsBlocking = filterList.isEnabled
          }
        case AdblockFilterListCatalogEntry.youtubeMobileRecommendationsComponentID:
          if filterList.isEnabled != self.youtubeRecommendationsBlocking {
            self.youtubeRecommendationsBlocking = filterList.isEnabled
          }
        case AdblockFilterListCatalogEntry.youtubeShortsComponentID:
          if filterList.isEnabled != self.youtubeShortsBlocking {
            self.youtubeShortsBlocking = filterList.isEnabled
          }
        default:
          continue
        }
      }
    }
  }
}

private struct QualitySettingsView: View {
  @Environment(\.presentationMode) @Binding var presentationMode

  var prefs: any PrefService

  var body: some View {
    Form {
      Section(header: Text(Strings.Settings.qualitySettings)) {
        Button(
          action: {
            prefs.set(
              YoutubeHighQualityPreference.on.rawValue,
              forPath: kYouTubeAutoQualityMode
            )
            presentationMode.dismiss()
          },
          label: {
            qualityOption(preference: .on)
          }
        )

        Button(
          action: {
            prefs.set(
              YoutubeHighQualityPreference.wifi.rawValue,
              forPath: kYouTubeAutoQualityMode
            )
            presentationMode.dismiss()
          },
          label: {
            qualityOption(preference: .wifi)
          }
        )

        Button(
          action: {
            prefs.set(
              YoutubeHighQualityPreference.off.rawValue,
              forPath: kYouTubeAutoQualityMode
            )
            presentationMode.dismiss()
          },
          label: {
            qualityOption(preference: .off)
          }
        )
      }
      .listRowBackground(Color(.secondaryBraveGroupedBackground))
    }
    .navigationBarTitle(Strings.Settings.highestQualityPlayback)
    .background(Color(UIColor.braveGroupedBackground))
  }

  func qualityOption(preference: YoutubeHighQualityPreference) -> some View {
    HStack {
      Text(preference.displayString)
        .foregroundColor(Color(.braveLabel))
      Spacer()
      if prefs.integer(forPath: kYouTubeAutoQualityMode) == preference.rawValue {
        Image(braveSystemName: "leo.check.normal")
      }
    }
  }
}

enum YoutubeHighQualityPreference: Int, CaseIterable {
  case off
  case on
  case wifi
}

extension YoutubeHighQualityPreference: RepresentableOptionType {
  var displayString: String {
    switch self {
    case .off: return Strings.youtubeMediaQualityOff
    case .wifi: return Strings.youtubeMediaQualityWifi
    case .on: return Strings.youtubeMediaQualityOn
    }
  }
}
