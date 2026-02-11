// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveStrings
import BraveUI
import Favicon
import Preferences
import SwiftUI

struct LoginListView: View {
  @ObservedObject private var saveLogins = Preferences.General.saveLogins
  @State private var viewModel: LoginListViewModel
  @State private var searchText: String = ""
  @State private var isSearchActive: Bool = false
  @State private var isEditMode: Bool = false

  private let passwordAPI: BravePasswordAPI
  private let windowProtection: WindowProtection?
  var settingsDelegate: SettingsDelegate?

  init(
    passwordAPI: BravePasswordAPI,
    windowProtection: WindowProtection?,
    settingsDelegate: SettingsDelegate?,
  ) {
    self.passwordAPI = passwordAPI
    self.windowProtection = windowProtection
    self.settingsDelegate = settingsDelegate
    self._viewModel = State(initialValue: LoginListViewModel(passwordAPI: passwordAPI))
  }

  var body: some View {
    ZStack {
      Color(.braveGroupedBackground)
        .ignoresSafeArea()

      if viewModel.credentialList.isEmpty && viewModel.blockedList.isEmpty
        && !viewModel.isRefreshing && !isSearchActive
      {
        emptyStateView
      } else {
        populatedStateView
      }
    }
    .toolbarBackground(.visible, for: .navigationBar)
    .navigationTitle(Strings.Autofill.managePasswordstTitle)
    .navigationBarTitleDisplayMode(.inline)
    .toolbar {
      ToolbarItem(placement: .navigationBarTrailing) {
        Button {
          // TODO: Handle Add New action
        } label: {
          Image(braveSystemName: "leo.plus.add")
        }
      }

      ToolbarItemGroup(placement: .bottomBar) {
        Spacer()
        Button(isEditMode ? Strings.done : Strings.edit) {
          isEditMode.toggle()
        }
      }
    }
    .onAppear {
      viewModel.fetchLoginInfo()
    }
    .onChange(of: searchText) {
      viewModel.performSearch(query: searchText.lowercased())
    }
  }

  private var emptyStateView: some View {
    VStack(spacing: 16) {
      Image(systemName: "info.circle")
        .foregroundColor(Color(.secondaryBraveLabel))

      Text(Strings.Autofill.managePasswordsEmptyListTitle)
        .font(.headline)
        .foregroundColor(Color(.braveLabel))

      Text(Strings.Autofill.managePasswordsEmptyListDetail)
        .font(.subheadline)
        .foregroundColor(Color(.secondaryBraveLabel))
        .multilineTextAlignment(.center)
        .padding(.horizontal, 32)
    }
  }

  private var populatedStateView: some View {
    Form {
      Section {
        EmptyView()
      } footer: {
        if !isSearchActive {
          Text(Strings.Autofill.managePasswordsInstructions)
            .font(.footnote)
        }
      }
      .listRowBackground(Color.clear)
      .background(Color.clear)
      .listRowSeparator(.hidden)

      if !isSearchActive {
        Toggle(Strings.Autofill.managePasswordsOfferToSavePasswords, isOn: $saveLogins.value)
          .tint(Color(braveSystemName: .primary40))
          .listRowBackground(Color(.secondaryBraveGroupedBackground))
      }

      Section {
        ForEach(Array(viewModel.groupedCredentialList), id: \.domain) { domain, credentials in
          LoginListRow(
            domain: domain,
            credentials: credentials,
            isEditMode: isEditMode,
            passwordAPI: passwordAPI,
            windowProtection: windowProtection,
            settingsDelegate: settingsDelegate
          )
          .listRowBackground(Color(.secondaryBraveGroupedBackground))
        }
      } header: {
        Text(Strings.Autofill.managePasswordsListHeaderTitle)
          .font(.subheadline)
      }

      if !viewModel.blockedList.isEmpty {
        Section(header: Text(Strings.Autofill.loginListNeverSavedListHeaderTitle)) {
          ForEach(Array(viewModel.groupedBlockedList), id: \.domain) { domain, credentials in
            LoginListRow(
              domain: domain,
              credentials: credentials,
              isEditMode: isEditMode,
              passwordAPI: passwordAPI,
              windowProtection: windowProtection,
              settingsDelegate: settingsDelegate
            )
            .listRowBackground(Color(.secondaryBraveGroupedBackground))
          }
        }
      }
    }
    .listRowBackground(Color(.secondaryBraveGroupedBackground))
    .searchable(text: $searchText, prompt: Strings.Autofill.managePasswordsListSearchWebsitesPrompt)
    .scrollContentBackground(.hidden)
    .padding(.top, 112)
    .ignoresSafeArea(.all, edges: .vertical)
  }
}

private struct LoginListRow: View {
  let domain: String
  let credentials: [PasswordForm]
  let isEditMode: Bool
  let passwordAPI: BravePasswordAPI
  let windowProtection: WindowProtection?
  let settingsDelegate: SettingsDelegate?

  var body: some View {
    NavigationLink {
      LoginInfoListView(
        domain: domain,
        credentials: credentials,
        passwordAPI: passwordAPI,
        windowProtection: windowProtection,
        settingsDelegate: settingsDelegate
      )
    } label: {
      if let realmURL = credentials.first.flatMap({ URL(string: $0.signOnRealm) }),
        !domain.isEmpty
      {
        HStack(spacing: 12) {
          FaviconImage(url: realmURL, isPrivateBrowsing: false)
            .frame(width: 24, height: 24)
            .clipShape(RoundedRectangle(cornerRadius: 6, style: .continuous))
          Text(domain)
          Spacer()
        }
        .foregroundColor(Color(.braveLabel))
        .padding(.vertical, 4)
      } else {
        EmptyView()
      }
    }
    .disabled(isEditMode)
  }
}
