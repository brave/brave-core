// Copyright 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveShields
import BraveUI
import DesignSystem
import Strings
import SwiftUI

struct CustomScriptletView: View {
  /// A wrapper around a save failure so it can drive an alert
  private struct SaveError: Identifiable {
    let id = UUID()
    let message: String
  }

  @Environment(\.dismiss) private var dismiss: DismissAction
  /// The scriptlet being edited, if any. `nil` when creating a new scriptlet.
  private let editingScriptlet: CustomScriptlet?
  /// The name for the custom scriptlet
  @State private var customScriptletName: String
  /// The javascript for the custom scriptlet
  @State private var customScriptletContent: String
  /// The font size for the custom scriptlet editor
  @ScaledMetric private var editorFontSize: CGFloat = 14
  /// A state for showing/hiding the cancelation alert
  @State private var showCancelAlert = false
  /// Indicates if we are currently saving the custom scriptlet
  @State private var isSaving = false
  /// The error to display if saving the custom scriptlet failed
  @State private var saveError: SaveError?

  /// The full name of the scriptlet, including `user-` and `.js`
  private var fullCustomScriptletName: String {
    CustomScriptlet.namePrefix + customScriptletName + CustomScriptlet.nameExtension
  }

  private var isSaveEnabled: Bool {
    CustomScriptlet.isValidName(fullCustomScriptletName) && !customScriptletContent.isEmpty
  }

  /// Whether the user has made any changes to the scriptlet
  private var hasChanges: Bool {
    fullCustomScriptletName != (editingScriptlet?.name ?? "")
      || customScriptletContent != (editingScriptlet?.content ?? "")
  }

  init(customScriptlet: CustomScriptlet? = nil) {
    self.editingScriptlet = customScriptlet
    // strip `user-` and `.js` from name, user only edits the other pieces
    var name = ""
    if let customName = customScriptlet?.name {
      name = String(customName.deletingPathExtension.trimmingPrefix(CustomScriptlet.namePrefix))
    }
    self._customScriptletName = State(wrappedValue: name)
    self._customScriptletContent = State(wrappedValue: customScriptlet?.content ?? "")
  }

  private var saveToolbarItem: ToolbarItem<(), some View> {
    ToolbarItem(placement: .confirmationAction) {
      if isSaving {
        ProgressView()
      } else {
        Button(
          action: saveCustomScriptlet,
          label: {
            Label(Strings.saveButtonTitle, braveSystemImage: "leo.check.normal")
              .labelStyle(.titleOnly)
          }
        )
        .disabled(!isSaveEnabled)
      }
    }
  }

  private var cancelToolbarItem: ToolbarItem<(), some View> {
    ToolbarItem(placement: .cancellationAction) {
      Button(
        action: {
          if hasChanges {
            showCancelAlert = true
          } else {
            dismiss()
          }
        },
        label: {
          Text(Strings.CancelString)
        }
      )
      .disabled(isSaving)
      .alert(
        isPresented: $showCancelAlert,
        content: {
          return Alert(
            title: Text(Strings.dismissChangesConfirmationTitle),
            message: Text(Strings.dismissChangesConfirmationMessage),
            primaryButton: .destructive(
              Text(Strings.dismissChangesButtonTitle),
              action: {
                dismiss()
              }
            ),
            secondaryButton: .cancel(
              Text(Strings.cancelButtonTitle)
            )
          )
        }
      )
    }
  }

  var body: some View {
    Form {
      Section(
        content: {
          HStack(alignment: .firstTextBaseline, spacing: 0) {
            Text(CustomScriptlet.namePrefix)
              .foregroundStyle(Color(braveSystemName: .textSecondary))
              .padding(4)
              .background(Color(white: 0.95))
              .clipShape(RoundedRectangle(cornerRadius: 4))
            TextField(
              "",
              text: $customScriptletName,
              prompt: Text(Strings.Shields.customScriptletNameSectionTitle.lowercased())
            )
            .autocorrectionDisabled(true)
            .textInputAutocapitalization(.never)
            Spacer()
            Text(CustomScriptlet.nameExtension)
              .foregroundStyle(Color(braveSystemName: .textSecondary))
              .padding(4)
              .background(Color(white: 0.95))
              .clipShape(RoundedRectangle(cornerRadius: 4))
          }
        },
        header: {
          Text(Strings.Shields.customScriptletNameSectionTitle)
        }
      )
      Section(
        content: {
          TextEditor(text: $customScriptletContent)
            .font(.system(size: editorFontSize, weight: .regular).monospaced())
            .frame(height: 400)
            .overlay(
              alignment: .topLeading,
              content: {
                Text(Strings.Shields.customScriptletContentWarning)
                  .multilineTextAlignment(.leading)
                  .autocorrectionDisabled(true)
                  .textInputAutocapitalization(.never)
                  .padding(.vertical, 8)
                  .padding(.horizontal, 8)
                  .disabled(true)
                  .allowsHitTesting(false)
                  .font(.body)
                  .frame(
                    maxWidth: .infinity,
                    maxHeight: .infinity,
                    alignment: .topLeading
                  )
                  .foregroundColor(Color(braveSystemName: .systemfeedbackErrorText))
                  .opacity(customScriptletContent.isEmpty ? 1 : 0)
                  .accessibilityHidden(customScriptletContent.isEmpty)
              }
            )
            .background(
              Color(.secondarySystemGroupedBackground),
              in: RoundedRectangle(cornerRadius: 12, style: .continuous)
            )
        },
        header: {
          Text(Strings.Shields.customScriptletContentSectionTitle)
        }
      )
    }
    .scrollContentBackground(.hidden)
    .scrollDismissesKeyboard(.interactively)
    .background(
      Color(braveSystemName: .pageBackground)
        .edgesIgnoringSafeArea(.all)
    )
    .navigationTitle(
      Text(
        editingScriptlet == nil
          ? Strings.Shields.addNewScriptletTitle : Strings.Shields.editScriptletTitle
      )
    )
    .navigationBarTitleDisplayMode(.inline)
    .toolbar {
      cancelToolbarItem
      saveToolbarItem
    }
    .alert(item: $saveError) { error in
      Alert(
        title: Text(Strings.genericErrorTitle),
        message: Text(error.message),
        dismissButton: .default(Text(Strings.OKString))
      )
    }
  }

  private func saveCustomScriptlet() {
    guard isSaveEnabled, !isSaving else { return }
    isSaving = true

    Task { @MainActor in
      defer { isSaving = false }

      do {
        try await CustomFilterListStorage.shared.save(
          customScriptlet: CustomScriptlet(
            name: fullCustomScriptletName,
            content: customScriptletContent
          )
        )
        // Remove the old file if the scriptlet was renamed
        if let editingScriptlet, editingScriptlet.name != fullCustomScriptletName {
          try await CustomFilterListStorage.shared.deleteCustomScriptlet(
            named: editingScriptlet.name
          )
        }
        dismiss()
      } catch {
        saveError = SaveError(message: error.localizedDescription)
      }
    }
  }
}

#Preview {
  NavigationStack(root: {
    CustomScriptletView()
  })
}

extension String {
  var deletingPathExtension: String {
    (self as NSString).deletingPathExtension
  }
}
