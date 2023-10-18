// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import SwiftUI
import BraveStrings
import Preferences
import BraveUI

struct MediaSettingsView: View {
  @ObservedObject var enableBackgroundAudio = Preferences.General.mediaAutoBackgrounding
  @ObservedObject var keepYouTubeInBrave = Preferences.General.keepYouTubeInBrave
  
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
        NavigationLink(destination: QualitySettingsView()) {
          VStack(alignment: .leading) {
            Text(Strings.Settings.highestQualityPlayback)
            Text(Strings.Settings.highestQualityPlaybackDetail)
              .font(.footnote)
              .foregroundColor(Color(.secondaryBraveLabel))
          }
        }
      }
      .listRowBackground(Color(.secondaryBraveGroupedBackground))
    }
    .navigationBarTitle(Strings.Settings.mediaRootSetting)
    .navigationBarTitleDisplayMode(.inline)
    .listBackgroundColor(Color(UIColor.braveGroupedBackground))
  }
}

fileprivate struct QualitySettingsView: View {
  @Environment(\.presentationMode) @Binding var presentationMode
  @ObservedObject var qualityOption = Preferences.General.youtubeHighQuality
  
  var body: some View {
    Form {
      Section(header: Text(Strings.Settings.qualitySettings)) {
        Button(action: {
          qualityOption.value = YoutubeHighQualityPreference.on.rawValue
          presentationMode.dismiss()
        }, label: {
          qualityOption(preference: .on)
        })
        
        Button(action: {
          qualityOption.value = YoutubeHighQualityPreference.wifi.rawValue
          presentationMode.dismiss()
        }, label: {
          qualityOption(preference: .wifi)
        })
        
        Button(action: {
          qualityOption.value = YoutubeHighQualityPreference.off.rawValue
          presentationMode.dismiss()
        }, label: {
          qualityOption(preference: .off)
        })
      }
      .listRowBackground(Color(.secondaryBraveGroupedBackground))
    }
    .navigationBarTitle(Strings.Settings.highestQualityPlayback)
    .listBackgroundColor(Color(UIColor.braveGroupedBackground))
  }
  
  func qualityOption(preference: YoutubeHighQualityPreference) -> some View {
    HStack {
      Text(preference.displayString)
        .foregroundColor(Color(.braveLabel))
      Spacer()
      if Preferences.General.youtubeHighQuality.value == preference.rawValue {
        Image(systemName: "checkmark")
      }
    }
  }
}

enum YoutubeHighQualityPreference: String, CaseIterable {
  case wifi
  case on
  case off
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

#if DEBUG
struct MediaSettingsView_Previews: PreviewProvider {
  static var previews: some View {
    MediaSettingsView()
  }
}
#endif
