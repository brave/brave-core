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
  @State private var deleteConfirmation: PasswordForm?

  private let passwordAPI: BravePasswordAPI
  private let windowProtection: WindowProtection?
  var settingsDelegate: SettingsDelegate?
  var onCredentialSelected: ((PasswordForm) -> Void)?

  init(
    passwordAPI: BravePasswordAPI,
    windowProtection: WindowProtection?,
    settingsDelegate: SettingsDelegate?,
    onCredentialSelected: ((PasswordForm) -> Void)? = nil
  ) {
    self.passwordAPI = passwordAPI
    self.windowProtection = windowProtection
    self.settingsDelegate = settingsDelegate
    self.onCredentialSelected = onCredentialSelected
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
        //ToolbarSpacer(.fixed, placement: .automatic)
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
    .alert(
      Strings.deleteLoginAlertTitle,
      isPresented: Binding(
        get: { deleteConfirmation != nil },
        set: { if !$0 { deleteConfirmation = nil } }
      )
    ) {
      Button(Strings.deleteLoginButtonTitle, role: .destructive) {
        if let credential = deleteConfirmation {
          viewModel.removeLogin(credential)
          deleteConfirmation = nil
        }
      }
      Button(Strings.cancelButtonTitle, role: .cancel) {
        deleteConfirmation = nil
      }
    } message: {
      Text(Strings.Login.loginEntryDeleteAlertMessage)
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
        ForEach(
          Array(viewModel.credentialList.enumerated()),
          id: \.element.signOnRealm
        ) { index, credential in
          LoginListRow(
            credential: credential,
            isEditMode: isEditMode,
            passwordAPI: passwordAPI,
            windowProtection: windowProtection,
            settingsDelegate: settingsDelegate,
            onTap: {
              if !isEditMode {
                onCredentialSelected?(credential)
              }
            },
            onDelete: {
              deleteConfirmation = credential
            }
          )
          .listRowBackground(Color(.secondaryBraveGroupedBackground))
        }
      } header: {
        Text(Strings.Autofill.managePasswordsListHeaderTitle)
          .font(.subheadline)
      }

      if !viewModel.blockedList.isEmpty {
        Section(header: Text(Strings.Autofill.loginListNeverSavedListHeaderTitle)) {
          ForEach(Array(viewModel.blockedList.enumerated()), id: \.element.signOnRealm) {
            index,
            credential in
            LoginListRow(
              credential: credential,
              isEditMode: isEditMode,
              passwordAPI: passwordAPI,
              windowProtection: windowProtection,
              settingsDelegate: settingsDelegate,
              onTap: {
                if !isEditMode {
                  onCredentialSelected?(credential)
                }
              },
              onDelete: {
                deleteConfirmation = credential
              }
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
  let credential: PasswordForm
  let isEditMode: Bool
  let passwordAPI: BravePasswordAPI
  let windowProtection: WindowProtection?
  let settingsDelegate: SettingsDelegate?
  let onTap: () -> Void
  let onDelete: () -> Void

  var body: some View {
    NavigationLink {
      LoginInfoViewControllerRepresentable(
        passwordAPI: passwordAPI,
        credentials: credential,
        windowProtection: windowProtection,
        settingsDelegate: settingsDelegate
      )
    } label: {
      if let realmURL = URL(string: credential.signOnRealm),
        let baseDomain = realmURL.baseDomain,
        !baseDomain.isEmpty
      {
        HStack(spacing: 12) {
          FaviconImage(url: realmURL, isPrivateBrowsing: false)
            .frame(width: 24, height: 24)
            .clipShape(RoundedRectangle(cornerRadius: 6, style: .continuous))
          Text(baseDomain)
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

// TODO: Remove LoginInfoViewControllerRepresentable after updting LoginInfoView to new design
private struct LoginInfoViewControllerRepresentable: UIViewControllerRepresentable {
  let passwordAPI: BravePasswordAPI
  let credentials: PasswordForm
  let windowProtection: WindowProtection?
  let settingsDelegate: SettingsDelegate?

  func makeUIViewController(context: Context) -> LoginInfoViewController {
    let vc = LoginInfoViewController(
      passwordAPI: passwordAPI,
      credentials: credentials,
      windowProtection: windowProtection
    )
    vc.settingsDelegate = settingsDelegate
    return vc
  }

  func updateUIViewController(_ uiViewController: LoginInfoViewController, context: Context) {
  }
}
