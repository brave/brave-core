// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveStrings
import BraveUI
import Foundation
import Preferences
import SwiftUI

struct NewTabPageSettingsView: View {
  var shouldShowSponsoredImagesAndVideosSetting: Bool
  var linkTapped: ((URLRequest) -> Void)?

  @ObservedObject private var backgroundImages = Preferences.NewTabPage.backgroundImages
  @ObservedObject private var showNewTabPrivacyHub = Preferences.NewTabPage.showNewTabPrivacyHub
  @ObservedObject private var showNewTabFavourites = Preferences.NewTabPage.showNewTabFavourites

  // This is observed to ensure the view updates correctly, but we instead access
  // Preferences.NewTabPage.backgroundMediaType which accesses backgroundMediaTypeRaw
  @ObservedObject private var backgroundMediaTypeRaw = Preferences.NewTabPage.backgroundMediaTypeRaw

  var body: some View {
    Form {
      Section {
        Toggle(Strings.NTP.settingsBackgroundImages, isOn: $backgroundImages.value)
          .listRowBackground(Color(.secondaryBraveGroupedBackground))
        if backgroundImages.value {
          NavigationLink {
            BackgroundMediaTypePicker(
              shouldShowSponsoredImagesAndVideosSetting: shouldShowSponsoredImagesAndVideosSetting,
              selection: Binding(
                get: { Preferences.NewTabPage.backgroundMediaType },
                set: { Preferences.NewTabPage.backgroundMediaType = $0 }
              )
            )
            .environment(
              \.openURL,
              OpenURLAction { _ in
                self.linkTapped?(URLRequest(url: .brave.newTabTakeoverLearnMoreLinkUrl))
                return .handled
              }
            )
          } label: {
            LabeledContent {
              switch Preferences.NewTabPage.backgroundMediaType {
              case .defaultImages:
                Text(Strings.NTP.settingsDefaultImagesOnly)
              case .sponsoredImages:
                Text(Strings.NTP.settingsSponsoredImagesSelection)
              case .sponsoredImagesAndVideos:
                Text(
                  shouldShowSponsoredImagesAndVideosSetting
                    ? Strings.NTP.settingsSponsoredImagesAndVideosSelection
                    : Strings.NTP.settingsSponsoredImagesSelection
                )
              }
            } label: {
              Text(Strings.NTP.settingsBackgroundImageSubMenu)
            }
          }
          .listRowBackground(Color(.secondaryBraveGroupedBackground))
        }
      } header: {
        Text(Strings.NTP.settingsBackgroundImages)
      }
      Section {
        Toggle(Strings.PrivacyHub.privacyReportsTitle, isOn: $showNewTabPrivacyHub.value)
          .listRowBackground(Color(.secondaryBraveGroupedBackground))
        Toggle(Strings.Widgets.favoritesWidgetTitle, isOn: $showNewTabFavourites.value)
          .listRowBackground(Color(.secondaryBraveGroupedBackground))
      } header: {
        Text(Strings.Widgets.widgetTitle)
      }
    }
    .tint(Color(braveSystemName: .primary40))
    .navigationTitle(Strings.NTP.settingsTitle)
    .navigationBarTitleDisplayMode(.inline)
    .scrollContentBackground(.hidden)
    .background(Color(.braveGroupedBackground))
  }

  private struct BackgroundMediaTypePicker: View {
    var shouldShowSponsoredImagesAndVideosSetting: Bool
    @Binding var selection: BackgroundMediaType

    @Environment(\.dismiss) private var dismiss

    var body: some View {
      Form {
        Section {
          Picker("", selection: $selection) {
            Text(Strings.NTP.settingsDefaultImagesOnly)
              .tag(BackgroundMediaType.defaultImages)
            if shouldShowSponsoredImagesAndVideosSetting {
              Text(Strings.NTP.settingsSponsoredImagesSelection)
                .tag(BackgroundMediaType.sponsoredImages)
            }
            Text(
              shouldShowSponsoredImagesAndVideosSetting
                ? Strings.NTP.settingsSponsoredImagesAndVideosSelection
                : Strings.NTP.settingsSponsoredImagesSelection
            )
            .tag(BackgroundMediaType.sponsoredImagesAndVideos)
          }
          .listRowBackground(Color(.secondaryBraveGroupedBackground))
          .pickerStyle(.inline)
          .labelsHidden()
          .onChange(of: selection, initial: false) {
            dismiss()
          }
        } header: {
          Text(Strings.NTP.settingsBackgroundImageSubMenu)
        } footer: {
          // Contains markdown with a link
          Text(LocalizedStringKey(Strings.NTP.imageTypeSelectionDescription))
            .tint(Color(braveSystemName: .textInteractive))
        }
      }
      .navigationTitle(Strings.NTP.settingsBackgroundImageSubMenu)
      .navigationBarTitleDisplayMode(.inline)
      .scrollContentBackground(.hidden)
      .background(Color(.braveGroupedBackground))
    }
  }
}

class NTPTableViewController: UIHostingController<NewTabPageSettingsView> {
  var rewards: BraveRewards?
  var linkTapped: ((URLRequest) -> Void)?

  init(rewards: BraveRewards?, linkTapped: ((URLRequest) -> Void)?) {
    super.init(
      rootView: .init(
        shouldShowSponsoredImagesAndVideosSetting:
          rewards?.ads.shouldShowSponsoredImagesAndVideosSetting() == true,
        linkTapped: linkTapped
      )
    )
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
}
