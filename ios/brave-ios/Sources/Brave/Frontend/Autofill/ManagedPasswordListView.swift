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

struct ManagedPasswordListView: View {
  @State private var isSceneActive = true
  @ObservedObject private var saveLogins = Preferences.General.saveLogins
  @State private var viewModel: ManagedPasswordListViewModel
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

  private var isContentUnavailable: Bool {
    viewModel.credentialList.isEmpty && viewModel.blockedList.isEmpty && !viewModel.isRefreshing
      && !isSearchActive
  }

  init(
    passwordAPI: BravePasswordAPI,
    windowProtection: WindowProtection?,
    settingsDelegate: SettingsDelegate?,
  ) {
    self.passwordAPI = passwordAPI
    self.windowProtection = windowProtection
    self.settingsDelegate = settingsDelegate
    self._viewModel = State(initialValue: ManagedPasswordListViewModel(passwordAPI: passwordAPI))
  }

  var body: some View {
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
          .tint(Color(braveSystemName: .schemesSurfaceTint))
      }

      Section {
        ForEach(Array(viewModel.groupedCredentialList), id: \.domain) { domain, credentials in
          let id = ManagedPasswordListView.domainId(saved: true, domain: domain)
          ManagedPasswordListRow(
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
          .swipeActions(edge: .trailing, allowsFullSwipe: true) {
            Button(role: .destructive) {
              deleteDomain(id)
            } label: {
              Label(
                Strings.Autofill.managedPasswordDeleteCredentialButtonTitle,
                systemImage: "trash"
              )
            }
          }
        }
      } header: {
        Text(Strings.Autofill.managePasswordsListHeaderTitle)
          .font(.subheadline)
      }

      if !viewModel.blockedList.isEmpty {
        Section(header: Text(Strings.Autofill.loginListNeverSavedListHeaderTitle)) {
          ForEach(Array(viewModel.groupedBlockedList), id: \.domain) { domain, credentials in
            let id = ManagedPasswordListView.domainId(saved: false, domain: domain)
            ManagedPasswordListRow(
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
            .swipeActions(edge: .trailing, allowsFullSwipe: true) {
              Button(role: .destructive) {
                deleteDomain(id)
              } label: {
                Label(
                  Strings.Autofill.managedPasswordDeleteCredentialButtonTitle,
                  systemImage: "trash"
                )
              }
            }
          }
        }
      }
    }
    .background(Color(.braveGroupedBackground))
    .searchable(
      text: $searchText,
      isPresented: $isSearchActive,
      placement: .navigationBarDrawer(displayMode: .always),
      prompt: Strings.Autofill.managePasswordsListSearchWebsitesPrompt
    )
    .scrollContentBackground(.hidden)
    .padding(.top, 100)
    .ignoresSafeArea()
    .overlay {
      if isContentUnavailable {
        ContentUnavailableView {
          Label {
            Text(Strings.Autofill.managePasswordsEmptyListTitle)
              .foregroundStyle(Color(braveSystemName: .textSecondary))
              .font(.title3)
          } icon: {
            Image(braveSystemName: "leo.info.ios-only")
              .foregroundStyle(Color(braveSystemName: .iconSecondary))
          }
        } description: {
          Text(Strings.Autofill.managePasswordsEmptyListDetail)
            .foregroundStyle(Color(braveSystemName: .textTertiary))
            .font(.callout)
        }
        .edgesIgnoringSafeArea(.all)
        .background(Color(braveSystemName: .schemesBackground))
      }
    }
    .overlay {
      if !isSceneActive {
        Color(.braveGroupedBackground)
          .ignoresSafeArea()
      }
    }
    .toolbarBackground(.visible, for: .navigationBar)
    .navigationTitle(Strings.Autofill.managePasswordstTitle)
    .navigationBarTitleDisplayMode(.inline)
    .toolbar {
      ToolbarItemGroup(placement: .bottomBar) {
        if !isContentUnavailable {
          if isEditMode {
            Button(Strings.Autofill.managedPasswordDeleteCredentialButtonTitle) {
              showDeleteConfirmation = true
            }
            .foregroundStyle(
              selectedDomainIds.isEmpty ? Color(braveSystemName: .secondary40) : .red
            )
            .disabled(selectedDomainIds.isEmpty)
            Spacer()
            Button(Strings.done) {
              updateEditMode(false)
              selectedDomainIds.removeAll()
            }
            .foregroundStyle(Color(braveSystemName: .textInteractive))
          } else {
            Spacer()
            Button(Strings.edit) {
              updateEditMode(true)
            }
          }
        }
      }
    }
    .alert(
      Strings.Autofill.managedPasswordDeleteCredentialsAlertTitle,
      isPresented: $showDeleteConfirmation
    ) {
      Button(Strings.Autofill.managedPasswordDeleteCredentialAlertCancelActionTitle, role: .cancel)
      {}
      Button(Strings.Autofill.managedPasswordDeleteCredentialButtonTitle, role: .destructive) {
        performDomainDeletion()
      }
    } message: {
      Text(
        String.localizedStringWithFormat(
          Strings.Autofill.managedPasswordDeleteCredentialAlertLocalMessage,
          selectedDomainsString
        )
      )
    }
    .onAppear {
      isSceneActive = true
      if !searchText.isEmpty {
        isSearchActive = true
        viewModel.performSearch(query: searchText.lowercased())
      } else {
        viewModel.fetchCredentials()
      }
    }
    .onChange(of: searchText) {
      viewModel.performSearch(query: searchText.lowercased())
    }
    .onReceive(NotificationCenter.default.publisher(for: UIScene.willDeactivateNotification)) { _ in
      isSceneActive = false
    }
    .onReceive(NotificationCenter.default.publisher(for: UIScene.didActivateNotification)) { _ in
      isSceneActive = true
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

  private func updateEditMode(_ isEnabled: Bool) {
    withAnimation(.easeIn(duration: 0.5)) {
      self.isEditMode = isEnabled
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
    updateEditMode(false)
  }

  private func deleteDomain(_ domainId: String) {
    var credentialsToRemove: [PasswordForm] = []
    if domainId.hasPrefix("saved:") {
      let domain = String(domainId.dropFirst(6))
      if let group = viewModel.groupedCredentialList.first(where: { $0.domain == domain }) {
        credentialsToRemove.append(contentsOf: group.credentials)
      }
    } else if domainId.hasPrefix("blocked:") {
      let domain = String(domainId.dropFirst(8))
      if let group = viewModel.groupedBlockedList.first(where: { $0.domain == domain }) {
        credentialsToRemove.append(contentsOf: group.credentials)
      }
    }
    viewModel.removeCredentials(credentialsToRemove)
    selectedDomainIds.remove(domainId)
  }
}

private struct ManagedPasswordRowLabel: View {
  let realmURL: URL
  let domain: String
  let credentials: [PasswordForm]

  var body: some View {
    Label {
      VStack(alignment: .leading, spacing: 2) {
        Text(domain)
        if credentials.count > 1 {
          Text("\(credentials.count) accounts")
            .font(.footnote)
            .foregroundColor(Color(.secondaryBraveLabel))
        }
      }
    } icon: {
      FaviconImage(url: realmURL, isPrivateBrowsing: false)
        .frame(width: 24, height: 24)
        .clipShape(RoundedRectangle(cornerRadius: 6, style: .continuous))
    }
  }
}

private struct ManagedPasswordListRow: View {
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

  private var resolvedRealmURL: URL {
    credentials.first.flatMap { URL(string: $0.signOnRealm) } ?? URL(string: "about:blank")!
  }

  private var resolvedDomain: String {
    domain.isEmpty ? Strings.Autofill.managedPasswordListUnknownDomainText : domain
  }

  // TODO: Remove LoginInfoViewControllerRepresentable after implementing ManagePasswordDetailView & ManagedPasswordGroupListView
  var credential: PasswordForm {
    credentials.first!
  }

  @ViewBuilder
  var body: some View {
    if isEditMode {
      Button {
        onToggleSelection()
      } label: {
        ManagedPasswordRowLabel(
          realmURL: resolvedRealmURL,
          domain: resolvedDomain,
          credentials: credentials
        )
        .labelStyle(
          ManagedPasswordRowLabelStyle(
            context: isSelected ? .select : .unselect
          )
        )
        .contentShape(Rectangle())
      }
      .buttonStyle(.plain)
    } else {
      NavigationLink {
        LoginInfoViewControllerRepresentable(
          passwordAPI: passwordAPI,
          credentials: credential,
          windowProtection: windowProtection,
          settingsDelegate: settingsDelegate
        )
      } label: {
        ManagedPasswordRowLabel(
          realmURL: resolvedRealmURL,
          domain: resolvedDomain,
          credentials: credentials
        )
        .labelStyle(ManagedPasswordRowLabelStyle(context: .plain))
      }
    }
  }
}

// TODO: Remove LoginInfoViewControllerRepresentable after implementing ManagePasswordDetailView & ManagedPasswordGroupListView
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
