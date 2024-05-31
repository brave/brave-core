// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveShields
import BraveUI
import DesignSystem
import Strings
import SwiftUI

struct CustomFilterListView: View {
  @Environment(\.dismiss) private var dismiss: DismissAction
  @ObservedObject private var customFilterListStorage = CustomFilterListStorage.shared
  /// The passed custom rules which we will be editing
  @Binding private var customRules: String?
  /// Any errors that are seen during saving or editing which we can display
  @State private var rulesError: Error?
  /// A state for showing/hiding the cancelation alert
  @State private var showCancelAlert = false
  /// Tells us if our text content is empty which allows us to show the prompt text
  @State private var isTextEmpty = false
  /// Our coordinator manages the content of the input text and gives us information back
  /// when saving up cancelling
  private var coordinator: FilterListEditor.Coordinator

  /// Tells us if we have changes from the original text
  private var hasChanges: Bool {
    return coordinator.text != customRules ?? ""
  }

  /// The shape of our text input box area
  private var borderShape: some InsettableShape {
    RoundedRectangle(cornerRadius: 12, style: .continuous)
  }

  init(customRules: Binding<String?>) {
    _customRules = customRules
    isTextEmpty = customRules.wrappedValue?.isEmpty ?? true
    coordinator = FilterListEditor.Coordinator()
    coordinator.text = customRules.wrappedValue ?? ""
  }

  private var saveToolbarItem: ToolbarItem<(), some View> {
    ToolbarItem(placement: .confirmationAction) {
      Button(
        action: saveCustomRules,
        label: {
          Label(Strings.saveButtonTitle, braveSystemImage: "leo.check.normal")
            .labelStyle(.titleOnly)
        }
      )
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

  @ViewBuilder private var filterListEditor: some View {
    VStack(alignment: .leading) {
      FilterListEditor(
        coordinator: coordinator,
        error: $rulesError,
        isTextEmpty: $isTextEmpty
      )
      .overlay(
        Text(Strings.Shields.customFiltersPlaceholder)
          .multilineTextAlignment(.leading)
          .padding(.vertical, 14)
          .padding(.horizontal, 16)
          .disabled(true)
          .allowsHitTesting(false)
          .font(.body)
          .frame(
            maxWidth: .infinity,
            maxHeight: .infinity,
            alignment: .topLeading
          )
          .foregroundColor(Color(.placeholderText))
          .opacity(isTextEmpty ? 1 : 0)
          .accessibilityHidden(!isTextEmpty),
        alignment: .topLeading
      )
      .clipShape(borderShape)

      if let error = rulesError {
        SectionFooterErrorView(errorMessage: error.localizedDescription)
          .padding(.horizontal, 12)
          .padding(.bottom, 0)
      }
    }
    .padding()
  }

  var body: some View {
    filterListEditor
      .osAvailabilityModifiers({ view in
        if #available(iOS 16.4, *) {
          view
            .scrollContentBackground(.hidden)
            .scrollDismissesKeyboard(.interactively)
        } else {
          view.introspectTextView { textView in
            textView.backgroundColor = .clear
          }
        }
      })
      .background(
        Color(.secondaryBraveBackground)
          .edgesIgnoringSafeArea(.all)
      )
      .navigationTitle(Text(Strings.Shields.customFilters))
      .navigationBarTitleDisplayMode(.inline)
      .toolbar {
        cancelToolbarItem
        saveToolbarItem
      }
  }

  private func saveCustomRules() {
    let pendingRules = coordinator.text

    Task {
      do {
        if !pendingRules.isEmpty {
          try await customFilterListStorage.save(customRules: pendingRules)
          customRules = pendingRules
        } else {
          try await customFilterListStorage.deleteCustomRules()
          customRules = nil
        }

        dismiss()
      } catch {
        // Could not load the rules
        self.rulesError = error
      }
    }
  }
}

#Preview {
  CustomFilterListView(
    customRules: .constant(
      """
      ! Hide the header on example.com (CF Test)
      example.com,example.net##h1

      ! Hide the brave logo on brave.com (Network test)
      ||brave.com/static-assets/images/brave-logo-sans-text.svg
      """
    )
  )
}

/// An editor for custom filter lists which limits the number of lines entered
///
/// - Note: We don't pass a binding text value
/// as this will cause glitches when editing.
/// Instead we pass bindings for individual states, such as `isTextEmpty`
/// Later we can pull the updated text directly from the coordinator when saving or cancelling.
struct FilterListEditor: UIViewRepresentable {
  /// The coordinator to use when editing
  let coordinator: Coordinator
  /// Any errors that might occur during editing
  @Binding var error: Error?
  /// This is updated everytime the emptiness of the text changes
  /// so we can add things like placeholders
  @Binding var isTextEmpty: Bool

  func makeCoordinator() -> Coordinator {
    return coordinator
  }

  func makeUIView(context: Context) -> UITextView {
    context.coordinator.textView
  }

  func updateUIView(_ uiView: UITextView, context: Context) {
    context.coordinator.errorEditingText = { error in
      self.error = error
    }
    context.coordinator.stringDidChange = { string in
      let isEmpty = string.isEmpty
      guard isEmpty != isTextEmpty else { return }
      isTextEmpty = isEmpty
    }
  }

  class Coordinator: NSObject, UITextViewDelegate {
    lazy var textView: UITextView = {
      let textView = UITextView()
      textView.font = UIFont.monospacedSystemFont(ofSize: 14, weight: .regular)
      textView.textColor = UIColor.braveLabel
      textView.autocorrectionType = .no
      textView.autocapitalizationType = .none
      textView.keyboardType = .alphabet
      textView.backgroundColor = .secondaryBraveGroupedBackground
      textView.textContainerInset = UIEdgeInsets(
        vertical: 16,
        horizontal: 12
      )
      textView.delegate = self
      return textView
    }()

    var stringDidChange: ((String) -> Void)?
    var errorEditingText: ((Error) -> Void)?

    var text: String {
      get {
        return textView.text
      }
      set {
        textView.text = newValue
      }
    }

    func textViewDidChange(_ textView: UITextView) {
      stringDidChange?(textView.text)
    }

    func textView(
      _ textView: UITextView,
      shouldChangeTextIn range: NSRange,
      replacementText text: String
    ) -> Bool {
      let currentText = textView.text ?? ""
      guard let stringRange = Range(range, in: currentText) else { return false }
      let updatedText = currentText.replacingCharacters(in: stringRange, with: text)
      let lines = updatedText.components(separatedBy: .newlines)

      guard lines.count <= CustomFilterListStorage.maxNumberOfCustomRulesLines else {
        errorEditingText?(
          CustomFilterListStorage.CustomRulesError.tooManyLines(
            max: CustomFilterListStorage.maxNumberOfCustomRulesLines
          )
        )
        return false
      }

      return true
    }
  }
}
