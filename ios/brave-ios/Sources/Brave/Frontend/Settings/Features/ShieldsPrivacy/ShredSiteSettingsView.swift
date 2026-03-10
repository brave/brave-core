// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveShields
import BraveUI
import Data
import Preferences
import Strings
import SwiftUI
import Web

/// View for displaying site-specific Shred settings
struct ShredSiteSettingsView: View {
  @ObservedObject private var viewModel: ShieldsSettingsViewModel
  private let shredSiteDataNow: () -> Void
  @State private var showConfirmation = false

  init(viewModel: ShieldsSettingsViewModel, shredSiteDataNow: @escaping () -> Void) {
    self.viewModel = viewModel
    self.shredSiteDataNow = shredSiteDataNow
  }

  var body: some View {
    Form {
      Section {
        FormPicker(selection: $viewModel.autoShredLevel) {
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

        Button(
          action: {
            showConfirmation = true
          },
          label: {
            HStack(alignment: .center) {
              Text(Strings.Shields.shredSiteDataNow)
                .frame(maxWidth: .infinity, alignment: .leading)
              Image(braveSystemName: "leo.shred.data")
            }
          }
        ).alert(
          isPresented: $showConfirmation,
          content: {
            confirmationAlert
          }
        )
      } header: {
        Text(
          viewModel.visibleURL?.displayURL?.baseDomain ?? viewModel.visibleURL?.absoluteString ?? ""
        )
      }.listRowBackground(Color(.secondaryBraveGroupedBackground))
    }
    .scrollContentBackground(.hidden)
    .background(Color(.braveGroupedBackground))
    .navigationTitle(Strings.Shields.shredSiteData)
    .toolbar(.visible)
  }

  private var confirmationAlert: Alert {
    Alert(
      title: Text(Strings.Shields.shredSiteDataConfirmationTitle),
      message: Text(
        Strings.Shields.shredSiteDataConfirmationMessage
      ),
      primaryButton: .destructive(
        Text(Strings.Shields.shredDataButtonTitle),
        action: {
          shredSiteDataNow()
        }
      ),
      secondaryButton: .cancel(Text(Strings.cancelButtonTitle))
    )
  }
}

extension SiteShredLevel: Identifiable {
  public var id: String {
    return rawValue
  }

  var localizedTitle: String {
    switch self {
    case .never:
      return Strings.Shields.shredNever
    case .whenSiteClosed:
      return Strings.Shields.shredOnSiteTabsClosed
    case .appExit:
      return Strings.Shields.shredOnAppClose
    }
  }
}
