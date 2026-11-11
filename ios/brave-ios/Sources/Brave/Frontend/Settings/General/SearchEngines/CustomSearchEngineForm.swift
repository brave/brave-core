// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveShared
import BraveUI
import Shared
import SwiftUI

/// A form for adding or editing a custom search engine
struct CustomSearchEngineForm: View {
  enum Mode: Identifiable, Hashable {
    case add
    case edit(SearchEngine)

    var id: String {
      switch self {
      case .add: return "add"
      case .edit(let engine): return engine.id
      }
    }
  }

  private enum FormError: Error {
    case missingInformation
    case invalidQuery
    case duplicate
    case failedToSave

    var title: String {
      switch self {
      case .invalidQuery:
        return Strings.CustomSearchEngine.thirdPartySearchEngineIncorrectFormErrorTitle
      case .missingInformation, .duplicate, .failedToSave:
        return Strings.CustomSearchEngine.thirdPartySearchEngineAddErrorTitle
      }
    }

    var message: String {
      switch self {
      case .missingInformation:
        return Strings.CustomSearchEngine.thirdPartySearchEngineMissingInfoErrorDescription
      case .invalidQuery:
        return Strings.CustomSearchEngine.thirdPartySearchEngineIncorrectFormErrorDescription
      case .duplicate:
        return Strings.CustomSearchEngine.thirdPartySearchEngineDuplicateErrorDescription
      case .failedToSave:
        return Strings.CustomSearchEngine.thirdPartySearchEngineAddErrorDescription
      }
    }
  }

  private static let searchTermsPlaceholder = "%s"
  private static let searchTermsTemplate = "{searchTerms}"

  var viewModel: SearchSettingsViewModel
  var mode: Mode

  @Environment(\.dismiss) private var dismiss

  @State private var title: String = ""
  @State private var query: String = ""
  @State private var error: FormError?

  init(viewModel: SearchSettingsViewModel, mode: Mode) {
    self.viewModel = viewModel
    self.mode = mode
  }

  var body: some View {
    Form {
      Section {
        TextField(Strings.CustomSearchEngine.engineTitlePlaceholder, text: $title)
      } header: {
        Text(Strings.title)
      }
      Section {
        TextField(Strings.URL, value: $query, format: CustomEngineURLFormatStyle())
          .keyboardType(.URL)
          .textInputAutocapitalization(.never)
          .autocorrectionDisabled()
          .lineLimit(1...4)
      } header: {
        Text(Strings.URL)
      } footer: {
        Text(Strings.CustomSearchEngine.customEngineAddDesription)
      }
    }
    .navigationTitle(Strings.searchSettingAddCustomEngineCellTitle)
    .navigationBarTitleDisplayMode(.inline)
    .toolbar {
      ToolbarItem(placement: .cancellationAction) {
        if #available(iOS 26.0, *) {
          Button(role: .cancel) {
            dismiss()
          }
        } else {
          Button(Strings.cancelButtonTitle) {
            dismiss()
          }
        }
      }
      ToolbarItem(placement: .confirmationAction) {
        Group {
          if #available(iOS 26.0, *) {
            Button(role: .confirm) {
              save()
            }
          } else {
            Button(Strings.saveButtonTitle) {
              save()
            }
          }
        }
        .disabled(trimmedTitle.isEmpty || trimmedQuery.isEmpty)
      }
    }
    .alert(
      error?.title ?? "",
      isPresented: $error.isPresented,
      presenting: error
    ) { _ in
      Button(Strings.OKString) {}
    } message: { error in
      Text(error.message)
    }
    .task {
      if case .edit(let engine) = mode {
        title = engine.shortName
        query = engine.url.replacingOccurrences(
          of: Self.searchTermsTemplate,
          with: Self.searchTermsPlaceholder
        )
      }
    }
  }

  private var trimmedTitle: String {
    title.trimmingCharacters(in: .whitespacesAndNewlines)
  }

  private var trimmedQuery: String {
    query.trimmingCharacters(in: .whitespacesAndNewlines)
  }

  /// Converts the user entered query (which uses `%s`) into a search URL template
  /// (which uses `{searchTerms}`), or nil if the query is not valid.
  private func searchURLTemplate(from query: String) -> String? {
    guard query.contains(Self.searchTermsPlaceholder) else {
      return nil
    }
    // Encode each component around the placeholders separately so that the
    // placeholder itself is not encoded. Existing percent-escapes are kept as-is.
    var allowedCharacters = CharacterSet.urlFragmentAllowed
    allowedCharacters.insert("%")
    let encodedComponents = query.components(separatedBy: Self.searchTermsPlaceholder).map {
      $0.addingPercentEncoding(withAllowedCharacters: allowedCharacters)
    }
    guard !encodedComponents.contains(nil) else {
      return nil
    }
    let template = encodedComponents.compactMap { $0 }.joined(separator: Self.searchTermsTemplate)
    guard
      let url = URL(
        string: template.replacingOccurrences(of: Self.searchTermsTemplate, with: "")
      ),
      url.isWebPage()
    else {
      return nil
    }
    return template
  }

  private func save() {
    let title = trimmedTitle
    let query = trimmedQuery
    guard !title.isEmpty, !query.isEmpty else {
      error = .missingInformation
      return
    }
    guard let template = searchURLTemplate(from: query) else {
      error = .invalidQuery
      return
    }
    switch mode {
    case .add:
      if viewModel.containsDuplicate(of: title, url: template) {
        error = .duplicate
        return
      }
      if !viewModel.addSearchEngine(shortName: title, url: template) {
        error = .failedToSave
        return
      }
    case .edit(let engine):
      if viewModel.containsDuplicate(of: title, url: template, excluding: engine) {
        error = .duplicate
        return
      }
      viewModel.editSearchEngine(engine, shortName: title, url: template)
    }
    dismiss()
  }
}

private struct CustomEngineURLFormatStyle: ParseableFormatStyle {
  func format(_ value: String) -> String {
    return value.withSecureUrlScheme
  }

  var parseStrategy: Parse {
    Parse()
  }

  struct Parse: ParseStrategy {
    func parse(_ value: String) throws -> String {
      return value.withSecureUrlScheme
    }
  }
}
