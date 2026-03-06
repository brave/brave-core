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
import UIKit

struct ManagePasswordsView: View {
  private typealias GroupID = ManagePasswordsViewModel.GroupID
  @Bindable var viewModel: ManagePasswordsViewModel

  @Environment(\.editMode) private var editMode
  @ObservedObject private var saveLogins = Preferences.General.saveLogins
  @State private var isSceneActive = true
  @State private var selectedGroupIds: Set<GroupID> = []
  @State private var isDeleteSelectionDialogPresented: Bool = false

  private var isSearchActive: Bool {
    !viewModel.searchText.isEmpty
  }

  private var isContentAvailable: Bool {
    !viewModel.allowedGroups.isEmpty || !viewModel.blockedGroups.isEmpty
  }

  private var isEditMode: Bool {
    editMode?.wrappedValue == .active
  }

  var body: some View {
    List(selection: $selectedGroupIds) {
      if !isSearchActive {
        Section {
          Text(Strings.Autofill.managePasswordsInstructions)
            .foregroundStyle(Color(braveSystemName: .textTertiary))
            .font(.footnote)
            .listRowBackground(Color.clear)
            .listRowSeparator(.hidden)
        }
      }

      if !isSearchActive {
        Toggle(Strings.Autofill.managePasswordsOfferToSavePasswords, isOn: $saveLogins.value)
          .tint(Color(braveSystemName: .primary40))
      }

      if !viewModel.filteredAllowedGroups.isEmpty {
        Section {
          ForEach(viewModel.filteredAllowedGroups, id: \.domain) { domain, credentials in
            ManagePasswordListRow(
              domain: domain,
              credentials: credentials,
              autofillDataManager: viewModel.autofillDataManager
            )
            .tag(GroupID.saved(domain: domain))
            .swipeActions(edge: .trailing, allowsFullSwipe: true) {
              Button(role: .destructive) {
                UIImpactFeedbackGenerator(style: .medium).vibrate()
                viewModel.deletePasswords(credentials)
              } label: {
                Label(
                  Strings.Autofill.managePasswordsDeleteCredentialButtonTitle,
                  braveSystemImage: "leo.trash"
                )
                .labelStyle(.iconOnly)
              }
            }
          }
        } header: {
          Text(Strings.Autofill.managePasswordsSavedListHeaderTitle)
            .font(.subheadline)
        }
      }

      if !viewModel.filteredBlockedGroups.isEmpty {
        Section {
          ForEach(viewModel.filteredBlockedGroups, id: \.domain) { domain, credentials in
            ManagePasswordListRow(
              domain: domain,
              credentials: credentials,
              autofillDataManager: viewModel.autofillDataManager
            )
            .tag(GroupID.blocked(domain: domain))
            .swipeActions(edge: .trailing, allowsFullSwipe: true) {
              Button(role: .destructive) {
                UIImpactFeedbackGenerator(style: .medium).vibrate()
                viewModel.deletePasswords(credentials)
              } label: {
                Label(
                  Strings.Autofill.managePasswordsDeleteCredentialButtonTitle,
                  braveSystemImage: "leo.trash"
                )
                .labelStyle(.iconOnly)
              }
            }
          }
        } header: {
          Text(Strings.Autofill.managePasswordsNeverSavedListHeaderTitle)
            .font(.subheadline)
        }
      }

      if isSearchActive && viewModel.filteredAllowedGroups.isEmpty
        && viewModel.filteredBlockedGroups.isEmpty
      {
        Section {
          ContentUnavailableView.search(text: viewModel.searchText)
            .listRowBackground(Color.clear)
        }
      }
    }
    .searchable(
      text: $viewModel.searchText,
      placement: .navigationBarDrawer(displayMode: .always),
      prompt: Strings.Autofill.managePasswordsListSearchWebsitesPrompt
    )
    .overlay {
      if viewModel.isFetching && viewModel.allowedGroups.isEmpty && viewModel.blockedGroups.isEmpty
      {
        ProgressView()
          .frame(maxWidth: .infinity, maxHeight: .infinity)
      }
    }
    .overlay {
      if !isSceneActive {
        Color(.braveGroupedBackground)
          .ignoresSafeArea()
      }
    }
    .toolbarBackground(.visible, for: .navigationBar)
    .navigationTitle(Strings.Autofill.managePasswordsTitle)
    .navigationBarTitleDisplayMode(.inline)
    .toolbar {
      ToolbarItem(placement: .topBarTrailing) {
        Button {
          //TODO: Present Add Password Form
        } label: {
          Label(Strings.addButtonTitle, braveSystemImage: "leo.plus.add")
        }
        .disabled(isEditMode)
      }

      if isEditMode {
        ToolbarItem(placement: .bottomBar) {
          Button(Strings.Autofill.managePasswordsDeleteCredentialButtonTitle) {
            isDeleteSelectionDialogPresented = true
          }
          .foregroundStyle(
            selectedGroupIds.isEmpty ? Color(braveSystemName: .textSecondary) : .red
          )
          .disabled(selectedGroupIds.isEmpty)
          .confirmationDialog(
            Strings.Autofill.managePasswordsDeleteCredentialsAlertTitle,
            isPresented: $isDeleteSelectionDialogPresented
          ) {
            Button(Strings.CancelString, role: .cancel) {}
            Button(
              Strings.Autofill.managePasswordsDeleteCredentialButtonTitle,
              role: .destructive
            ) {
              deleteSelectedGroups()
            }
          } message: {
            Text(
              String.localizedStringWithFormat(
                Strings.Autofill.managePasswordsDeleteCredentialConfirmMessage,
                selectedDomainsString
              )
            )
          }
        }
      }

      if #available(iOS 26.0, *), LiquidGlassMode.isEnabled {
        ToolbarSpacer(.flexible, placement: .bottomBar)
      } else {
        ToolbarItem(placement: .bottomBar) {
          Spacer()
        }
      }

      ToolbarItem(placement: .bottomBar) {
        EditButton()
          .disabled(viewModel.allowedGroups.isEmpty && viewModel.blockedGroups.isEmpty)
      }
    }
    .toolbar(isContentAvailable || viewModel.isFetching ? .visible : .hidden, for: .bottomBar)
    .onReceive(NotificationCenter.default.publisher(for: UIScene.willDeactivateNotification)) { _ in
      isSceneActive = false
    }
    .onReceive(NotificationCenter.default.publisher(for: UIScene.didActivateNotification)) { _ in
      isSceneActive = true
    }
  }

  private var selectedDomainsString: String {
    selectedGroupIds.map { $0.domain }.sorted().joined(separator: ", ")
  }

  private func deleteSelectedGroups() {
    UIImpactFeedbackGenerator(style: .medium).vibrate()
    viewModel.deletePasswords(forGroupIds: selectedGroupIds)
    selectedGroupIds.removeAll()
    exitEditMode()
  }

  private func exitEditMode() {
    editMode?.wrappedValue = .inactive
  }
}

private struct ManagePasswordListRow: View {
  let domain: String
  let credentials: [CWVPassword]
  let autofillDataManager: CWVAutofillDataManager
  @Environment(\.autofillManagementContext) private var context

  private var resolvedRealmURL: URL {
    credentials.first.flatMap { URL(string: $0.site) } ?? URL(string: "about:blank")!
  }

  private var resolvedDomain: String {
    domain.isEmpty ? Strings.Autofill.managePasswordsUnknownDomainText : domain
  }

  @ViewBuilder
  var body: some View {
    NavigationLink {
      if let password = credentials.first, let context {
        ManagePasswordDetailView(
          viewModel: ManagePasswordDetailViewModel(
            mode: .view(password),
            autofillDataManager: autofillDataManager
          )
        )
        .environment(\.autofillManagementContext, context)
      }
    } label: {
      Label {
        VStack(alignment: .leading, spacing: 2) {
          Text(resolvedDomain)
          if credentials.count > 1 {
            Text("\(credentials.count) \(Strings.Autofill.managePasswordMultipleAccounts)")
              .font(.footnote)
              .foregroundStyle(Color(braveSystemName: .textSecondary))
          }
        }
      } icon: {
        FaviconImage(url: resolvedRealmURL, isPrivateBrowsing: false)
          .frame(width: 24, height: 24)
          .clipShape(RoundedRectangle(cornerRadius: 6, style: .continuous))
      }
    }
  }
}
