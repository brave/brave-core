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
  @State private var selectedDomainIds: Set<String> = []
  @State private var showDeleteConfirmation: Bool = false

  private let passwordAPI: BravePasswordAPI
  private let windowProtection: WindowProtection?
  var settingsDelegate: SettingsDelegate?

  private static func domainId(saved: Bool, domain: String) -> String {
    saved ? "saved:\(domain)" : "blocked:\(domain)"
  }

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
    mainContent
      .toolbarBackground(.visible, for: .navigationBar)
      .navigationTitle(Strings.Autofill.managePasswordstTitle)
      .navigationBarTitleDisplayMode(.inline)
      .toolbar { toolbarContent }
      .alert(Strings.Autofill.deleteLoginAlertTitle, isPresented: $showDeleteConfirmation) {
        Button(Strings.Autofill.deleteLoginAlertCancelActionTitle, role: .cancel) {}
        Button(Strings.Autofill.deleteLoginButtonTitle, role: .destructive) {
          performDomainDeletion()
        }
      } message: {
        Text(
          String.localizedStringWithFormat(
            Strings.Autofill.deleteLoginAlertLocalMessage,
            selectedDomainsString
          )
        )
      }
      .onAppear {
        viewModel.fetchLoginInfo()
      }
      .onChange(of: searchText) {
        viewModel.performSearch(query: searchText.lowercased())
      }
  }

  @ViewBuilder
  private var mainContent: some View {
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
  }

  @ToolbarContentBuilder
  private var toolbarContent: some ToolbarContent {
    ToolbarItem(placement: .navigationBarTrailing) {
      Button {
        // TODO: Handle Add New action
      } label: {
        Image(braveSystemName: "leo.plus.add")
      }
      .disabled(isEditMode)
    }

    ToolbarItemGroup(placement: .bottomBar) {
      if isEditMode {
        Button(Strings.Autofill.deleteLoginButtonTitle) {
          showDeleteConfirmation = true
        }
        .foregroundColor(selectedDomainIds.isEmpty ? Color(.braveDisabled) : .red)
        .disabled(selectedDomainIds.isEmpty)
        Spacer()
        Button(Strings.done) {
          withAnimation {
            isEditMode = false
          }
          selectedDomainIds.removeAll()
        }
        .foregroundColor(Color(.braveBlurpleTint))
      } else {
        Spacer()
        Button(Strings.edit) {
          withAnimation {
            isEditMode = true
          }
        }
      }
    }
  }

  private var selectedDomainsString: String {
    selectedDomainIds.reduce(into: "") { result, domainId in
      let savedDomainPrefix = "saved:"
      let blockedDomainPrefix = "blocked:"
      let domain: String

      if domainId.hasPrefix(savedDomainPrefix) {
        domain = String(domainId.dropFirst(savedDomainPrefix.count))
      } else if domainId.hasPrefix(blockedDomainPrefix) {
        domain = String(domainId.dropFirst(blockedDomainPrefix.count))
      } else {
        domain = domainId
      }
      if !result.isEmpty { result += ", " }
      result += domain
    }
  }
  private func toggleSelection(_ domainId: String) {
    if selectedDomainIds.contains(domainId) {
      selectedDomainIds.remove(domainId)
    } else {
      selectedDomainIds.insert(domainId)
    }
  }

  private func performDomainDeletion() {
    var credentialsToRemove: [PasswordForm] = []
    for id in selectedDomainIds {
      if id.hasPrefix("saved:") {
        let domain = String(id.dropFirst(6))
        if let group = viewModel.groupedCredentialList.first(where: { $0.domain == domain }) {
          credentialsToRemove.append(contentsOf: group.credentials)
        }
      } else if id.hasPrefix("blocked:") {
        let domain = String(id.dropFirst(8))
        if let group = viewModel.groupedBlockedList.first(where: { $0.domain == domain }) {
          credentialsToRemove.append(contentsOf: group.credentials)
        }
      }
    }
    viewModel.removeCredentials(credentialsToRemove)
    selectedDomainIds.removeAll()
    withAnimation {
      isEditMode = false
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
          let id = LoginListView.domainId(saved: true, domain: domain)
          LoginListRow(
            domain: domain,
            credentials: credentials,
            isSaved: true,
            domainId: id,
            isEditMode: isEditMode,
            isSelected: selectedDomainIds.contains(id),
            onToggleSelection: { toggleSelection(id) },
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
            let id = LoginListView.domainId(saved: false, domain: domain)
            LoginListRow(
              domain: domain,
              credentials: credentials,
              isSaved: false,
              domainId: id,
              isEditMode: isEditMode,
              isSelected: selectedDomainIds.contains(id),
              onToggleSelection: { toggleSelection(id) },
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
  let isSaved: Bool
  let domainId: String
  let isEditMode: Bool
  let isSelected: Bool
  let onToggleSelection: () -> Void
  let passwordAPI: BravePasswordAPI
  let windowProtection: WindowProtection?
  var settingsDelegate: SettingsDelegate?

  private var rowLabel: some View {
    Group {
      if let realmURL = credentials.first.flatMap({ URL(string: $0.signOnRealm) }),
        !domain.isEmpty
      {
        HStack(spacing: 12) {
          FaviconImage(url: realmURL, isPrivateBrowsing: false)
            .frame(width: 24, height: 24)
            .clipShape(RoundedRectangle(cornerRadius: 6, style: .continuous))
          VStack(alignment: .leading, spacing: 2) {
            Text(domain)
              .foregroundColor(Color(.braveLabel))
            if credentials.count > 1 {
              Text("\(credentials.count) accounts")
                .font(.footnote)
                .foregroundColor(Color(.secondaryBraveLabel))
            }
          }
          Spacer()
        }
        .padding(.vertical, 4)
      }
    }
  }

  @ViewBuilder
  var body: some View {
    if isEditMode {
      editModeRow
    } else {
      navigationRow
    }
  }

  private var editModeRow: some View {
    Button {
      onToggleSelection()
    } label: {
      HStack(spacing: 12) {
        Image(braveSystemName: isSelected ? "leo.check.circle-filled" : "leo.radio.unchecked")
          .font(.title2)
          .foregroundColor(isSelected ? Color(.braveBlurpleTint) : Color(.secondaryBraveLabel))
        rowLabel
      }
      .contentShape(Rectangle())
    }
    .buttonStyle(.plain)
  }

  private var navigationRow: some View {
    NavigationLink {
      LoginInfoListView(
        domain: domain,
        credentials: credentials,
        passwordAPI: passwordAPI,
        windowProtection: windowProtection,
        settingsDelegate: settingsDelegate
      )
    } label: {
      rowLabel
    }
  }
}
