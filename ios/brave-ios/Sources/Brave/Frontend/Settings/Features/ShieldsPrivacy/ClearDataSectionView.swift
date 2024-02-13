// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import SwiftUI
import DesignSystem
import BraveUI
import Strings
import Preferences

struct ClearDataSectionView: View {
  @State private var showClearablesAlert = false
  @ObservedObject var settings: AdvancedShieldsSettings
  
  var body: some View {
    Section {
      ForEach($settings.clearableSettings) { $setting in
        ShieldToggleView(
          title: setting.clearable.label,
          subtitle: nil,
          toggle: $setting.isEnabled
        )
      }
      
      clearPrivateDataButton
    } header: {
      Text(Strings.clearPrivateData)
    }
  }
  
  private var clearPrivateDataButton: some View {
    Button {
      showClearablesAlert = true
    } label: {
      VStack(alignment: .center) {
        Text(Strings.clearDataNow)
          .foregroundColor(Color(.braveBlurpleTint))
      }
    }
      .frame(maxWidth: .infinity)
      .alert(isPresented: $showClearablesAlert, content: {
        Alert(
          title: Text(Strings.clearPrivateDataAlertTitle),
          message: Text(Strings.clearPrivateDataAlertMessage),
          primaryButton: .destructive(Text(Strings.clearPrivateDataAlertYesAction), action: {
            clearPrivateData()
          }),
          secondaryButton: .cancel(Text(Strings.cancelButtonTitle))
        )
      })
      .disabled(settings.clearableSettings.allSatisfy({ !$0.isEnabled }))
      .listRowBackground(Color(.secondaryBraveGroupedBackground))
  }
  
  private func clearPrivateData() {
    Preferences.Privacy.clearPrivateDataToggles.value = settings.clearableSettings.map({ $0.isEnabled })
    
    Task { @MainActor in
      await settings.clearPrivateData(settings.clearableSettings.compactMap({
        guard $0.isEnabled else { return nil }
        return $0.clearable
      }))
    }
  }
}
