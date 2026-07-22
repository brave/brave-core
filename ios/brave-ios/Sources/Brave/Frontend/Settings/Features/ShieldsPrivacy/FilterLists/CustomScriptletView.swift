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
  @Environment(\.dismiss) private var dismiss: DismissAction
  /// The name for the custom scriptlet
  @State private var customScriptletName = ""
  /// The javascript for the custom scriptlet
  @State private var customScriptletContent = ""
  /// The font size for the custom scriptlet editor
  @ScaledMetric private var editorFontSize: CGFloat = 14
  /// A state for showing/hiding the cancelation alert
  @State private var showCancelAlert = false
  /// Indicates if we are currently saving the custom scriptlet
  @State private var isSaving = false
  
  private var isSaveEnabled: Bool {
    !customScriptletName.isEmpty && !customScriptletContent.isEmpty
  }

  init() {

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
          if !customScriptletContent.isEmpty {
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
          // TODO: Require prefix `user-` and suffix `.js`
          TextField("", text: $customScriptletName, prompt: Text("Name"))
            .autocorrectionDisabled(true)
        },
        header: {
          Text("Name")
        })
      Section(
        content: {
          TextEditor(text: $customScriptletContent)
            .font(.system(size: editorFontSize, weight: .regular).monospaced())
            .frame(height: 400)
            .overlay(
              alignment: .topLeading,
              content: {
                Text("Don’t paste code here that you don’t understand or haven’t reviewed yourself. This could allow attackers to steal your identity or take control of your computer.")
                  .multilineTextAlignment(.leading)
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
          Text("Content")
        }
      )
    }
    .scrollContentBackground(.hidden)
    .scrollDismissesKeyboard(.interactively)
    .background(
      Color(braveSystemName: .pageBackground)
        .edgesIgnoringSafeArea(.all)
    )
    .navigationTitle(Text("Add New Scriptlet"))
    .navigationBarTitleDisplayMode(.inline)
    .toolbar {
      cancelToolbarItem
      saveToolbarItem
    }
  }

  private func saveCustomScriptlet() {
    
  }
}

#Preview {
  NavigationStack(root: {
    CustomScriptletView()
  })
}
