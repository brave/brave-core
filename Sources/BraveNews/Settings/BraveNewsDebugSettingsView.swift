// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import SwiftUI
import BraveShared

extension FeedDataSource.Environment {
  fileprivate var name: String {
    switch self {
    case .dev: return "Dev"
    case .staging: return "Staging"
    case .production: return "Production"
    }
  }
}

extension FeedDataSource {
  fileprivate func description(of state: State) -> String {
    switch state {
    case .initial:
      return "—"
    case .loading:
      return "Loading"
    case .success:
      return "Success"
    case .failure:
      return "Error"
    }
  }
}

public struct BraveNewsDebugSettingsView: View {
  private let feedDataSource: FeedDataSource
  private let dismissAction: (() -> Void)?
  
  public init(dataSource: FeedDataSource, dismissAction: (() -> Void)? = nil) {
    self.feedDataSource = dataSource
    self.dismissAction = dismissAction
    // These values should always be overridden
    self._environment = .init(wrappedValue: dataSource.environment)
    self._languageCode = .init(wrappedValue: dataSource.languageCode)
  }
  
  @State private var environment: FeedDataSource.Environment = .production
  @State private var languageCode: String = "en"
  
  public var body: some View {
    List {
      Section {
        Picker("Environment", selection: $environment) {
          ForEach(FeedDataSource.Environment.allCases, id: \.self) { env in
            Text(env.name)
          }
        }
        NavigationLink {
          LanguagePicker(languageCode: $languageCode)
        } label: {
          HStack {
            Text("Language")
            Spacer()
            Text(Locale.current.localizedString(forLanguageCode: languageCode) ?? "")
              .foregroundColor(.secondary)
          }
        }
      } footer: {
        Text("Changing the environment or language will purge all cached resources immediately.")
      }
      .listRowBackground(Color(.secondaryBraveGroupedBackground))
      Section {
        HStack {
          Text("State")
          Spacer()
          Text(feedDataSource.description(of: feedDataSource.state))
            .foregroundColor(.secondary)
        }
        switch feedDataSource.state {
        case .initial:
          EmptyView()
        case .loading(let previousState):
          HStack {
            Text("Previous State")
            Spacer()
            Text(feedDataSource.description(of: previousState))
              .foregroundColor(.secondary)
          }
        case .success(let cards):
          HStack {
            Text("Sources")
            Spacer()
            Text("\(feedDataSource.sources.count)")
              .foregroundColor(.secondary)
          }
          HStack {
            Text("Cards Generated")
            Spacer()
            Text("\(cards.count)")
              .foregroundColor(.secondary)
          }
        case .failure(let error):
          Text(error.localizedDescription)
            .foregroundColor(.secondary)
        }
      }
      .listRowBackground(Color(.secondaryBraveGroupedBackground))
      Section {
        Button {
          let categories = Set(feedDataSource.sources.map(\.category))
          for category in categories where !category.isEmpty {
            feedDataSource.toggleCategory(category, enabled: false)
          }
        } label: {
          Text("Disable All Default Sources")
        }
      }
      .listRowBackground(Color(.secondaryBraveGroupedBackground))
    }
    .listStyle(.insetGrouped)
    .navigationTitle("Brave News QA Settings")
    .navigationBarTitleDisplayMode(.inline)
    .toolbar {
      ToolbarItemGroup(placement: .confirmationAction) {
        if let dismissAction = dismissAction {
          Button(action: dismissAction) {
            Text("Done")
          }
        }
      }
    }
    .onChange(of: environment) { _ in
      feedDataSource.environment = environment
    }
    .onChange(of: languageCode) { _ in
      feedDataSource.languageCode = languageCode
    }
  }
}

private struct LanguagePicker: View {
  @Binding var languageCode: String
  @State private var customCode: String = ""
  
  var body: some View {
    Form {
      Section {
        Picker("", selection: $languageCode) {
          ForEach(FeedDataSource.supportedLanguages, id: \.self) { code in
            Label {
              if let localizedName = Locale.current.localizedString(forLanguageCode: code) {
                Text(localizedName)
              }
            } icon: {
              Text(code)
                .foregroundColor(.secondary)
            }
          }
        }
        .labelsHidden()
        .pickerStyle(.inline)
      } header: {
        Text("Supported Langauges")
      }
      .listRowBackground(Color(.secondaryBraveGroupedBackground))
      Section {
        HStack {
          TextField("Language Code", text: $customCode) {
            languageCode = customCode
          }
          .autocapitalization(.none)
          .disableAutocorrection(true)
          if languageCode == customCode {
            if let localizedName = Locale.current.localizedString(forLanguageCode: customCode) {
              Text(localizedName)
                .foregroundColor(.secondary)
            } else {
              Text("\(Image(systemName: "exclamationmark.triangle")) Unknown Language")
                .foregroundColor(Color(.braveErrorLabel))
            }
            Image(systemName: "checkmark")
              .font(.body.weight(.semibold))
              .foregroundColor(Color(.braveOrange))
          }
        }
      } header: {
        Text("Custom")
      } footer: {
        Text("A 2 letter language code. Ensure when providing a custom language code that it is supported on backend. This is to test languages that have not been officially added to the supported languages list on the client.")
      }
      .listRowBackground(Color(.secondaryBraveGroupedBackground))
    }
    .listStyle(.insetGrouped)
    .onAppear {
      if !FeedDataSource.supportedLanguages.contains(languageCode) {
        // SwiftUI bug needs to be set a bit after
        DispatchQueue.main.asyncAfter(deadline: .now() + 0.1) {
          customCode = languageCode
        }
      }
    }
  }
}

#if DEBUG
struct BraveNewsDebugSettingsView_PreviewProvided: PreviewProvider {
  static var previews: some View {
    NavigationView {
      BraveNewsDebugSettingsView(dataSource: .init())
    }
    .previewLayout(.sizeThatFits)
  }
}
#endif
