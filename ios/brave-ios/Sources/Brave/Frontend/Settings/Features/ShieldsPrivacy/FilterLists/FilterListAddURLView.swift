// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveShields
import BraveUI
import DesignSystem
import Strings
import SwiftUI

struct FilterListAddURLView: View {
  @ObservedObject private var customFilterListStorage = CustomFilterListStorage.shared
  @Environment(\.presentationMode) @Binding private var presentationMode
  @State private var newURLInput: String = ""
  @State private var errorMessage: String?
  @FocusState private var isURLFieldFocused: Bool

  private var textField: some View {
    TextField(Strings.Shields.filterListsEnterFilterListURL, text: $newURLInput)
      .onChange(of: newURLInput) { newValue in
        errorMessage = nil
      }
      .keyboardType(.URL)
      .textContentType(.URL)
      .autocapitalization(.none)
      .autocorrectionDisabled()
      .focused($isURLFieldFocused, equals: true)
      .onSubmit {
        handleOnSubmit()
      }
  }

  var body: some View {
    NavigationView {
      Form {
        Section(
          content: {
            VStack(alignment: .leading) {
              textField
                .submitLabel(SubmitLabel.done)
            }.listRowBackground(Color(.secondaryBraveGroupedBackground))
          },
          header: {
            Text(Strings.Shields.customFilterListURL)
          },
          footer: {
            VStack(alignment: .leading, spacing: 0) {
              SectionFooterErrorView(errorMessage: errorMessage)

              VStack(alignment: .leading, spacing: 8) {
                Text(Strings.Shields.addCustomFilterListDescription)
                  .fixedSize(horizontal: false, vertical: true)
                Text(LocalizedStringKey(Strings.Shields.addCustomFilterListWarning))
                  .fixedSize(horizontal: false, vertical: true)
              }.padding(.top)
            }
          }
        )
      }
      .animation(.easeInOut, value: errorMessage)
      .listBackgroundColor(Color(UIColor.braveGroupedBackground))
      .listStyle(.insetGrouped)
      .navigationTitle(Strings.Shields.customFilterList)
      .navigationBarTitleDisplayMode(.inline)
      .toolbar {
        ToolbarItemGroup(placement: .confirmationAction) {
          Button(Strings.Shields.filterListsAdd) {
            handleOnSubmit()
          }.disabled(newURLInput.isEmpty)
        }

        ToolbarItemGroup(placement: .cancellationAction) {
          Button(Strings.CancelString) {
            presentationMode.dismiss()
          }
        }
      }
    }.frame(idealWidth: 400, idealHeight: 400)
      .onAppear {
        isURLFieldFocused = true
      }
  }

  private func handleOnSubmit() {
    guard !newURLInput.isEmpty else { return }
    guard let url = URL(string: newURLInput) else {
      self.errorMessage = Strings.Shields.filterListAddInvalidURLError
      return
    }
    guard url.scheme == "https" else {
      self.errorMessage = Strings.Shields.filterListAddOnlyHTTPSAllowedError
      return
    }
    guard
      !customFilterListStorage.filterListsURLs.contains(where: { filterListURL in
        return filterListURL.setting.externalURL == url
      })
    else {
      // Don't allow duplicates
      self.presentationMode.dismiss()
      return
    }

    Task {
      let customURL = FilterListCustomURL(
        externalURL: url,
        isEnabled: true,
        inMemory: !customFilterListStorage.persistChanges
      )

      customFilterListStorage.filterListsURLs.append(customURL)

      await FilterListCustomURLDownloader.shared.startFetching(
        filterListCustomURL: customURL
      )

      self.presentationMode.dismiss()
    }
  }
}

#if DEBUG
struct FilterListAddURLView_Previews: PreviewProvider {
  static var previews: some View {
    FilterListAddURLView()
  }
}
#endif
