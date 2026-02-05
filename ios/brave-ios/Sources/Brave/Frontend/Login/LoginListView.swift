//
//  LoginListView.swift
//  Brave
//
//  Created by Eli Hini on 2026-02-03.
//

import BraveCore
import BraveStrings
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

      if viewModel.credentialList.isEmpty && viewModel.blockedList.isEmpty && !viewModel.isRefreshing {
        emptyStateView
      } else {
        populatedStateView
      }
    }
    .navigationTitle(Strings.Autofill.managePasswordstNavigationBarTitle)
    .navigationBarTitleDisplayMode(.inline)
    .toolbar {
      ToolbarItem(placement: .navigationBarTrailing) {
        Button(Strings.Autofill.managePasswordsAddNewButtonTitle) {
          // TODO: Handle Add New action
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
      viewModel.performSearch(query: searchText)
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
    EmptyView()
  }
}
