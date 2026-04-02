// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveStrings
import BraveUI
import SwiftUI
import UIKit

/// Displays all login credentials for a selected domain.
/// Each credential is shown as a separate row (username + masked password).
struct ManagePasswordGroupView: View {
  @Environment(\.editMode) private var editMode
  @Environment(\.openURL) private var openURL
  @Environment(\.redactionReasons) private var redactionReasons

  let viewModel: ManagePasswordsViewModel
  let domain: String

  @State private var selectedCredentialIds: Set<String> = []
  @State private var isDeleteSelectionDialogPresented = false

  private var passwords: [CWVPassword] {
    (viewModel.allowedGroups.first { $0.domain == domain }
      ?? viewModel.blockedGroups.first { $0.domain == domain })?.credentials ?? []
  }

  var body: some View {
    List(selection: $selectedCredentialIds) {
      Section {
        ForEach(passwords, id: \.identifier) { password in
          NavigationLink {
            ManagePasswordDetailView(viewModel: viewModel, password: password)
              .environment(\.openURL, openURL)
              .environment(\.redactionReasons, redactionReasons)
          } label: {
            VStack(alignment: .leading, spacing: 4) {
              Text(password.username ?? password.title)
              Text(String(repeating: "•", count: 8))
                .font(.subheadline)
            }
            .frame(maxWidth: .infinity, alignment: .leading)
          }
          .swipeActions(edge: .trailing, allowsFullSwipe: true) {
            Button(role: .destructive) {
              UIImpactFeedbackGenerator(style: .medium).vibrate()
              viewModel.deletePasswords([password])
            } label: {
              Label(
                Strings.Autofill.managePasswordsDeleteCredentialButtonTitle,
                braveSystemImage: "leo.trash"
              )
              .labelStyle(.iconOnly)
            }
          }
          .listRowBackground(Color(.secondaryBraveGroupedBackground))
        }
      }
    }
    .scrollContentBackground(.hidden)
    .background((Color(.braveGroupedBackground)))
    .navigationTitle(redactionReasons.contains(.privacy) ? "" : domain)
    .navigationBarTitleDisplayMode(.inline)
    .toolbarBackground(.visible, for: .navigationBar)
    .toolbar {
      if !redactionReasons.contains(.privacy) {
        ToolbarItem(placement: .topBarTrailing) {
          Button {
            // TODO: Present Add Password Form
          } label: {
            Label(Strings.addButtonTitle, braveSystemImage: "leo.plus.add")
          }
          .disabled(isEditMode)
        }
      }

      if isEditMode {
        ToolbarItem(placement: .bottomBar) {
          Button(Strings.Autofill.managePasswordsDeleteCredentialButtonTitle) {
            isDeleteSelectionDialogPresented = true
          }
          .foregroundStyle(
            Color(
              braveSystemName: selectedCredentialIds.isEmpty
                ? .textDisabled : .systemfeedbackErrorVibrant
            )
          )
          .disabled(selectedCredentialIds.isEmpty)
          .confirmationDialog(
            Strings.Autofill.managePasswordsDeleteCredentialsAlertTitle,
            isPresented: $isDeleteSelectionDialogPresented
          ) {
            Button(Strings.CancelString, role: .cancel) {}
            Button(
              Strings.Autofill.managePasswordsDeleteCredentialButtonTitle,
              role: .destructive
            ) {
              deleteSelections()
            }
          } message: {
            Text(
              String.localizedStringWithFormat(
                Strings.Autofill.managePasswordsDeleteCredentialConfirmMessage,
                selectedCredentialsString
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
          .disabled(passwords.isEmpty)
      }
    }
    .toolbar(redactionReasons.contains(.privacy) ? .hidden : .visible, for: .bottomBar)
    .overlay {
      if redactionReasons.contains(.privacy) { Color(.braveGroupedBackground).ignoresSafeArea() }
    }
  }

  private var isEditMode: Bool {
    editMode?.wrappedValue == .active
  }

  private var selectedCredentialsString: String {
    passwords
      .map({ $0.username ?? $0.title })
      .sorted()
      .formatted()
  }

  private func deleteSelections() {
    UIImpactFeedbackGenerator(style: .medium).vibrate()
    let toDelete = passwords.filter { selectedCredentialIds.contains($0.identifier) }
    viewModel.deletePasswords(toDelete)
    selectedCredentialIds.removeAll()
    exitEditMode()
  }

  private func exitEditMode() {
    editMode?.wrappedValue = .inactive
  }
}
