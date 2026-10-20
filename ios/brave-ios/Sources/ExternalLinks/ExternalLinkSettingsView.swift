// Copyright 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveStrings
import BraveUI
import Strings
import SwiftUI

public struct ExternalLinkSettingsRow: View {
  @Binding var mode: ExternalLinkMode
  var onReset: () -> Void

  public init(mode: Binding<ExternalLinkMode>, onReset: @escaping () -> Void) {
    self._mode = mode
    self.onReset = onReset
  }

  public var body: some View {
    NavigationLink {
      ExternalLinkSettingsView(mode: $mode, onReset: onReset)
    } label: {
      LabeledContent(
        Strings.ExternalLinks.settingTitle,
        value: (mode.isAvailable ? mode : .current).localizedTitle
      )
    }
  }
}

struct ExternalLinkSettingsView: View {
  @Binding var mode: ExternalLinkMode
  var onReset: () -> Void

  @State private var isShowingResetConfirmation = false

  var body: some View {
    Form {
      Section {
        Picker(selection: $mode) {
          ForEach(ExternalLinkMode.allOptions) { option in
            LabelView(
              title: option.localizedTitle,
              subtitle: option.localizedDescription
            )
            .tag(option)
          }
        } label: {
          EmptyView()
        }
        .pickerStyle(.inline)
      } header: {
        Text(Strings.ExternalLinks.defaultSectionTitle)
      }
      Section {
        Button(Strings.ExternalLinks.resetRememberedChoices, role: .destructive) {
          isShowingResetConfirmation = true
        }
      } footer: {
        Text(Strings.ExternalLinks.resetRememberedChoicesFooter)
      }
    }
    .alert(
      Strings.ExternalLinks.resetRememberedChoicesAlertTitle,
      isPresented: $isShowingResetConfirmation
    ) {
      Button(Strings.ExternalLinks.resetRememberedChoicesAlertConfirm, role: .destructive) {
        onReset()
      }
      Button(Strings.cancelButtonTitle, role: .cancel) {}
    } message: {
      Text(Strings.ExternalLinks.resetRememberedChoicesAlertMessage)
    }
    .navigationTitle(Strings.ExternalLinks.settingTitle)
  }
}

#if DEBUG
#Preview("Settings Row") {
  @Previewable @State var mode: ExternalLinkMode = .ask
  NavigationStack {
    Form {
      ExternalLinkSettingsRow(mode: $mode, onReset: {})
    }
  }
}

#Preview("Settings Detail") {
  @Previewable @State var mode: ExternalLinkMode = .current
  NavigationStack {
    ExternalLinkSettingsView(mode: $mode, onReset: {})
  }
}
#endif
