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
  @Environment(\.autofillManagementContext) private var context
  @Environment(\.dismiss) private var dismiss
  @Environment(\.editMode) private var editMode

  @Bindable var viewModel: ManagePasswordGroupViewModel

  @State private var isSceneActive = true
  @State private var selectedCredentialIds: Set<String> = []
  @State private var isDeleteSelectionDialogPresented = false

  var body: some View {
    List(selection: $selectedCredentialIds) {
      Section {
        ForEach(viewModel.passwords, id: \.identifier) { password in
          NavigationLink {
            ManagePasswordDetailView(
              viewModel: ManagePasswordDetailViewModel(
                mode: .view(password),
                autofillDataManager: viewModel.autofillDataManager
              )
            )
            .environment(\.autofillManagementContext, context)
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
              viewModel.delete(password)
            } label: {
              Label(
                Strings.Autofill.managePasswordsDeleteCredentialButtonTitle,
                braveSystemImage: "leo.trash"
              )
              .labelStyle(.iconOnly)
            }
          }
        }
      }
    }
    .navigationTitle(viewModel.domain)
    .navigationBarTitleDisplayMode(.inline)
    .toolbarBackground(.visible, for: .navigationBar)
    .toolbar {
      ToolbarItem(placement: .topBarTrailing) {
        Button {
          // TODO: Present Add Password Form
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
            Color(braveSystemName: selectedCredentialIds.isEmpty ? .textSecondary : .primitiveRed0)
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
          .disabled(viewModel.passwords.isEmpty)
      }
    }
    .overlay {
      if !isSceneActive {
        Color(.braveGroupedBackground)
          .ignoresSafeArea()
      }
    }
    .onChange(of: viewModel.isEmpty) { _, isEmpty in
      if isEmpty { dismiss() }
    }
    .onReceive(NotificationCenter.default.publisher(for: UIScene.willDeactivateNotification)) { _ in
      isSceneActive = false
    }
    .onReceive(NotificationCenter.default.publisher(for: UIScene.didActivateNotification)) { _ in
      isSceneActive = true
    }
  }

  private var isEditMode: Bool {
    editMode?.wrappedValue == .active
  }

  private var selectedCredentialsString: String {
    viewModel.passwords
      .filter { selectedCredentialIds.contains($0.identifier) }
      .compactMap { $0.username }
      .sorted()
      .joined(separator: ", ")
  }

  private func deleteSelections() {
    UIImpactFeedbackGenerator(style: .medium).vibrate()
    viewModel.deleteCredentials(forIds: selectedCredentialIds)
    selectedCredentialIds.removeAll()
    exitEditMode()
  }

  private func exitEditMode() {
    editMode?.wrappedValue = .inactive
  }
}
