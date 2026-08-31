// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveStrings
import BraveUI
import Foundation
import Preferences
import SwiftUI

struct NewTabPageSettingsView: View {
  var isSponsoredBackgroundsSupported: Bool
  var linkTapped: ((URLRequest) -> Void)?

  @ObservedObject private var backgroundImages = Preferences.NewTabPage.backgroundImages
  @ObservedObject private var showNewTabPrivacyHub = Preferences.NewTabPage.showNewTabPrivacyHub
  @ObservedObject private var showNewTabFavourites = Preferences.NewTabPage.showNewTabFavourites

  @ObservedObject private var sponsoredEnabled = Preferences.BraveAds.sponsoredEnabled

  var body: some View {
    Form {
      Section {
        Toggle(Strings.NTP.settingsBackgroundImages, isOn: $backgroundImages.value)
        if backgroundImages.value, isSponsoredBackgroundsSupported {
          NavigationLink {
            BackgroundMediaTypePicker(
              isSponsoredImagesEnabled: $sponsoredEnabled.value
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
              if sponsoredEnabled.value {
                Text(Strings.NTP.settingsSponsoredImagesSelection)
              } else {
                Text(Strings.NTP.settingsDefaultImagesOnly)
              }
            } label: {
              Text(Strings.NTP.settingsBackgroundImageSubMenu)
            }
          }
        }
      } header: {
        Text(Strings.NTP.settingsBackgroundImages)
      }
      Section {
        Toggle(Strings.PrivacyHub.privacyReportsTitle, isOn: $showNewTabPrivacyHub.value)
        Toggle(Strings.Widgets.favoritesWidgetTitle, isOn: $showNewTabFavourites.value)
      } header: {
        Text(Strings.Widgets.widgetTitle)
      }
    }
    .tint(Color(braveSystemName: .primary40))
    .navigationTitle(Strings.NTP.settingsTitle)
    .navigationBarTitleDisplayMode(.inline)
  }

  private struct BackgroundMediaTypePicker: View {
    @Binding var isSponsoredImagesEnabled: Bool

    @Environment(\.dismiss) private var dismiss

    var body: some View {
      Form {
        Section {
          Picker("", selection: $isSponsoredImagesEnabled) {
            Text(Strings.NTP.settingsDefaultImagesOnly)
              .tag(false)
            Text(Strings.NTP.settingsSponsoredImagesSelection)
              .tag(true)
          }
          .pickerStyle(.inline)
          .labelsHidden()
          .onChange(of: isSponsoredImagesEnabled, initial: false) {
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
    }
  }
}

class NTPTableViewController: UIHostingController<NewTabPageSettingsView> {
  var rewards: BraveRewards?
  var linkTapped: ((URLRequest) -> Void)?

  init(rewards: BraveRewards?, linkTapped: ((URLRequest) -> Void)?) {
    super.init(
      rootView: .init(
        isSponsoredBackgroundsSupported: rewards != nil,
        linkTapped: linkTapped
      )
    )
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
}
