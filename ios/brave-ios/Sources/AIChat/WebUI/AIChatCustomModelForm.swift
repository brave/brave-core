// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShared
import BraveUI
import DesignSystem
import SwiftUI

/// A form for entering details about a single custom model to be used in Leo
struct CustomModelForm: View {
  /// The initial model to populate the form with, if the user is editing a preexisting model
  var initialModel: AiChat.Model?
  /// The settings helper that exposes save/update methods
  var helper: any AIChatSettingsHelper

  @State private var data: FormData = .init()
  @State private var validationError: FormValidationError?
  @FocusState private var focus: Field?

  @Environment(\.dismiss) private var dismiss

  private struct FormData {
    var displayName: String = ""
    var requestName: String = ""
    var serverEndpoint: String = ""
    var contextSize: Int?
    var apiKey: String?
    var systemPrompt: String?
    var visionSupport: Bool = false

    var isRequiredDataFilled: Bool {
      return !displayName.isEmpty && !requestName.isEmpty && !serverEndpoint.isEmpty
    }
  }

  /// A set of fields that can be focused by the keyboard
  private enum Field: CaseIterable {
    case label
    case requestName
    case endpoint
    case contextSize
    case apiKey
    case systemPrompt
  }

  /// Errors that can occur during a save/update
  fileprivate enum FormValidationError: LocalizedError {
    /// The server endpoint inputted is not a valid URL
    case invalidUrl
    /// The context size inputted is not valid
    case invalidContextSize
    /// The server endpoint inputted is valid only if the user enables the private IPs feature flag
    case urlValidAsPrivateEndpoint

    var localizedDescription: String {
      switch self {
      case .invalidUrl:
        return Strings.AIChat.customModelFormInvalidUrlErrorMessage
      case .invalidContextSize:
        return Strings.AIChat.customModelFormInvalidContextSizeErrorMessage
      case .urlValidAsPrivateEndpoint:
        return Strings.AIChat.customModelFormPrivateEndpointErrorMessage
      }
    }

    init?(result: AiChat.OperationResult) {
      switch result {
      case .invalidUrl: self = .invalidUrl
      case .invalidContextSize: self = .invalidContextSize
      case .urlValidAsPrivateEndpoint: self = .urlValidAsPrivateEndpoint
      case .success: return nil
      @unknown default: return nil
      }
    }
  }

  var body: some View {
    Form {
      Section {
        LabeledContent {
          TextField(Strings.AIChat.customModelFormLabelFieldPlaceholder, text: $data.displayName)
            .focused($focus, equals: .label)
            .submitLabel(.next)
        } label: {
          Text(Strings.AIChat.customModelFormLabelFieldTitle)
          Text(Strings.AIChat.customModelFormRequiredFieldIndicator)
            .foregroundStyle(Color(braveSystemName: .systemfeedbackErrorText))
        }
        .labeledContentStyle(.formInput)
        .listRowBackground(Color(.secondaryBraveGroupedBackground))
      } footer: {
        Text(Strings.AIChat.customModelFormLabelFieldFooter)
      }

      Section {
        LabeledContent {
          TextField(
            Strings.AIChat.customModelFormRequestNameFieldPlaceholder,
            text: $data.requestName
          )
          .focused($focus, equals: .requestName)
          .submitLabel(.next)
          .autocorrectionDisabled()
          .textInputAutocapitalization(.never)
        } label: {
          Text(Strings.AIChat.customModelFormRequestNameFieldTitle)
          Text(Strings.AIChat.customModelFormRequiredFieldIndicator)
            .foregroundStyle(Color(braveSystemName: .systemfeedbackErrorText))
        }
        .labeledContentStyle(.formInput)
        .listRowBackground(Color(.secondaryBraveGroupedBackground))
      } footer: {
        Text(Strings.AIChat.customModelFormRequestNameFieldFooter)
      }

      Section {
        LabeledContent {
          VStack(alignment: .leading) {
            TextField(
              Strings.AIChat.customModelFormDetailsFieldPlaceholder,
              text: $data.serverEndpoint
            )
            .focused($focus, equals: .endpoint)
            .submitLabel(.next)
            .keyboardType(.URL)
            .autocorrectionDisabled()
            .textInputAutocapitalization(.never)
            if URL(string: data.serverEndpoint)?.scheme == "http" {
              HStack(spacing: 4) {
                Image(braveSystemName: "leo.lock.open")
                Text(Strings.AIChat.customModelFormUnsafeEndpointWarning)
              }
              .foregroundStyle(Color(braveSystemName: .systemfeedbackErrorText))
              .padding(.horizontal, 4)
              .padding(.vertical, 2)
              .background(
                Color(braveSystemName: .systemfeedbackErrorBackground),
                in: .rect(cornerRadius: 4, style: .continuous)
              )
              .font(.footnote)
            }
          }
        } label: {
          Text(Strings.AIChat.customModelFormServerEndpointFieldTitle)
          Text(Strings.AIChat.customModelFormRequiredFieldIndicator)
            .foregroundStyle(Color(braveSystemName: .systemfeedbackErrorText))
        }
        .labeledContentStyle(.formInput)
        .listRowBackground(Color(.secondaryBraveGroupedBackground))
      } footer: {
        // Contains Markdown
        Text(LocalizedStringKey(Strings.AIChat.customModelFormServerEndpointFieldFooter))
      }

      Section {
        LabeledContent {
          TextField(
            "\(AIChatDefaultCustomModelContextSize)",
            value: $data.contextSize,
            format: .number
          )
          .focused($focus, equals: .contextSize)
          .submitLabel(.next)
          .keyboardType(.numberPad)
        } label: {
          Text(Strings.AIChat.customModelFormContextSizeFieldTitle)
        }
        .labeledContentStyle(.formInput)
        .listRowBackground(Color(.secondaryBraveGroupedBackground))
      } footer: {
        Text(Strings.AIChat.customModelFormContextSizeFieldFooter)
      }

      Section {
        LabeledContent {
          TextField(
            Strings.AIChat.customModelFormDetailsFieldPlaceholder,
            text: $data.apiKey.coalesced
          )
          .focused($focus, equals: .apiKey)
          .submitLabel(.next)
          .autocorrectionDisabled()
          .textInputAutocapitalization(.never)
        } label: {
          Text(Strings.AIChat.customModelFormApiKeyFieldTitle)
        }
        .labeledContentStyle(.formInput)
        .listRowBackground(Color(.secondaryBraveGroupedBackground))
      } footer: {
        Text(Strings.AIChat.customModelFormApiKeyFieldFooter)
      }

      Section {
        Toggle(isOn: $data.visionSupport) {
          Text(Strings.AIChat.customModelFormVisionSupportFieldTitle)
          Text(Strings.AIChat.customModelFormVisionSupportFieldDescription)
        }
        .tint(Color(braveSystemName: .primary40))
        .listRowBackground(Color(.secondaryBraveGroupedBackground))
      }

      Section {
        LabeledContent {
          TextField(
            helper.defaultCustomModelSystemPrompt,
            text: $data.systemPrompt.coalesced,
            axis: .vertical
          )
          .focused($focus, equals: .systemPrompt)
          .submitLabel(.return)
          .lineLimit(8...)
        } label: {
          Text(Strings.AIChat.customModelFormSystemPromptFieldTitle)
        }
        .labeledContentStyle(.formInput)
        .listRowBackground(Color(.secondaryBraveGroupedBackground))
      } footer: {
        Text(Strings.AIChat.customModelFormSystemPromptFieldFooter)
      }
    }
    .scrollContentBackground(.hidden)
    .background(Color(.braveGroupedBackground))
    .onSubmit(of: .text) {
      guard let focus, let currentIndex = Field.allCases.firstIndex(of: focus) else {
        return
      }
      // At the moment the final form field is a multiline text field for the system prompt, so
      // there will be no onSubmit called for that field so there is no reason to call `save`
      // when the focus ends
      self.focus = Field.allCases[safe: currentIndex + 1]
    }
    .alert(
      Strings.AIChat.customModelFormFailedToSaveAlertTitle,
      isPresented: $validationError.isPresented,
      presenting: validationError,
      actions: { _ in
        Button(Strings.OKString) {}
      },
      message: { error in
        Text(error.localizedDescription)
      }
    )
    .navigationTitle(
      initialModel == nil
        ? Strings.AIChat.customModelFormAddModelNavigationTitle
        : Strings.AIChat.customModelFormEditModelNavigationTitle
    )
    .navigationBarTitleDisplayMode(.inline)
    .onAppear {
      if let initialModel {
        data.displayName = initialModel.displayName
        data.visionSupport = initialModel.visionSupport
        if let customOptions = initialModel.options.customModelOptions {
          data.requestName = customOptions.modelRequestName
          data.serverEndpoint = customOptions.endpoint.absoluteString
          data.systemPrompt = customOptions.modelSystemPrompt
          data.apiKey = customOptions.apiKey
          data.contextSize = Int(customOptions.contextSize)
        }
      }
    }
    .toolbar {
      ToolbarItemGroup(placement: .keyboard) {
        Spacer()
        Button(Strings.AIChat.customModelFormDoneButtonTitle) {
          focus = nil
        }
      }
      ToolbarItemGroup(placement: .confirmationAction) {
        Group {
          if #available(iOS 26.0, *), LiquidGlassMode.isEnabled {
            Button(role: .confirm) {
              Task {
                await save()
              }
            }
            .accessibilityLabel(Text(Strings.AIChat.customModelFormSaveButtonTitle))
          } else {
            Button(Strings.AIChat.customModelFormSaveButtonTitle) {
              Task {
                await save()
              }
            }
          }
        }
        .disabled(!data.isRequiredDataFilled)
      }
      if initialModel == nil {
        ToolbarItemGroup(placement: .cancellationAction) {
          if #available(iOS 26.0, *) {
            Button(role: .cancel) {
              dismiss()
            }
          } else {
            Button(Strings.CancelString) {
              dismiss()
            }
          }
        }
      }
    }
  }

  private func save() async {
    guard let endpointURL = URL(string: data.serverEndpoint) else {
      validationError = .invalidUrl
      return
    }
    let model = AiChat.Model()
    model.visionSupport = data.visionSupport
    model.displayName = data.displayName
    model.options = .init(
      customModelOptions: .init(
        modelRequestName: data.requestName,
        contextSize: UInt32(data.contextSize ?? AIChatDefaultCustomModelContextSize),
        maxAssociatedContentLength: 0,
        longConversationWarningCharacterLimit: 0,
        modelSystemPrompt: data.systemPrompt,
        endpoint: endpointURL,
        apiKey: data.apiKey ?? ""
      )
    )
    let result: AiChat.OperationResult
    if let initialModel {
      model.key = initialModel.key
      if let index = helper.customModels.firstIndex(where: { $0.key == model.key }) {
        result = await helper.updateCustomModel(at: index, model: model)
      } else {
        // The model no longer exists to update?
        dismiss()
        return
      }
    } else {
      result = await helper.addCustomModel(model)
    }
    if result == .success {
      dismiss()
      return
    }
    validationError = .init(result: result)
  }
}

// Binding<String?> format helper
extension String? {
  fileprivate var coalesced: String {
    get { self ?? "" }
    set { self = newValue.isEmpty ? nil : newValue }
  }
}

// Binding<FormValidationError?> presentation helper
extension CustomModelForm.FormValidationError? {
  fileprivate var isPresented: Bool {
    get { self != nil }
    set { if !newValue { self = nil } }
  }
}

private struct CustomModelFormInputLabeledContentStyle: LabeledContentStyle {
  func makeBody(configuration: Configuration) -> some View {
    VStack(alignment: .leading) {
      HStack {
        configuration.label
          .foregroundStyle(Color(braveSystemName: .textPrimary))
          .fontWeight(.medium)
      }
      .font(.footnote)
      configuration.content
    }
  }
}

extension LabeledContentStyle where Self == CustomModelFormInputLabeledContentStyle {
  fileprivate static var formInput: Self { .init() }
}

#if DEBUG
private class MockAIChatSettingsHelper: AIChatSettingsHelper {
  var delegate: (any AIChatSettingsHelperDelegate)?

  var modelsWithSubtitles: [AiChat.ModelWithSubtitle] = []

  var defaultModelKey: String = "chat-automatic"

  func fetchPremiumStatus(_ handler: @escaping (AiChat.PremiumStatus, AiChat.PremiumInfo?) -> Void)
  {
  }

  func resetLeoData() {
  }

  var customModels: [AiChat.Model] = []

  func addCustomModel(_ model: AiChat.Model) async -> AiChat.OperationResult {
    mockResult(for: model)
  }

  func updateCustomModel(at index: Int, model: AiChat.Model) async -> AiChat.OperationResult {
    mockResult(for: model)
  }

  func mockResult(for model: AiChat.Model) -> AiChat.OperationResult {
    let endpoint = model.options.customModelOptions?.endpoint.absoluteString
    switch endpoint {
    case "invalid": return .invalidUrl
    case "private": return .urlValidAsPrivateEndpoint
    default: return .success
    }
  }

  func deleteCustomModel(at index: Int) {
  }

  var defaultCustomModelSystemPrompt: String = """
    The current time and date is %datetime%.
    You are **Leo**, a helpful AI assistant by Brave. Assist Brave browser users with clear, concise, and polite responses.
    """
}
extension AiChat.Model {
  fileprivate static var mockCustomModel: Self {
    Self(
      options: .init(
        customModelOptions: .init(
          modelRequestName: "custom-model-vision-3",
          contextSize: 8000,
          maxAssociatedContentLength: 0,
          longConversationWarningCharacterLimit: 0,
          modelSystemPrompt: "Custom System Prompt",
          endpoint: URL(string: "http://localhost:11434/v1/chat/completions")!,
          apiKey: ""
        )
      ),
      key: "custom:model",
      displayName: "Custom Model",
      visionSupport: true,
      supportsTools: false,
      isSuggestedModel: false,
      isNearModel: false
    )
  }
}
#Preview("Add New Model") {
  NavigationStack {
    CustomModelForm(helper: MockAIChatSettingsHelper())
  }
}
#Preview("Edit Existing Model") {
  NavigationStack {
    CustomModelForm(
      initialModel: .mockCustomModel,
      helper: MockAIChatSettingsHelper()
    )
  }
}
#endif
