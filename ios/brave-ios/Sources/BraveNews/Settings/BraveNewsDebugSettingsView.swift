// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveStrings
import BraveUI
import Preferences
import SwiftUI
import UIKit

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
    self._localeOverride = .init(wrappedValue: dataSource.selectedLocale)
  }

  @State private var environment: FeedDataSource.Environment = .production
  @State private var localeOverride: String = "en_US"
  @State private var fileList: [NewsFile]?

  private struct NewsFile: Identifiable, Equatable {
    var url: URL
    var name: String
    var size: Int
    var modifiedDate: Date

    var id: String {
      url.absoluteString
    }
  }

  public var body: some View {
    Form {
      Section {
        Picker("Environment", selection: $environment) {
          ForEach(FeedDataSource.Environment.allCases, id: \.self) { env in
            Text(env.name)
          }
        }
        Picker("Selected Locale", selection: $localeOverride) {
          ForEach(Array(feedDataSource.availableLocales).sorted(), id: \.self) { locale in
            Text(locale).tag(locale)
          }
        }
      } footer: {
        Text("Changing the environment will purge all cached resources immediately.")
      }
      .listRowBackground(Color(.secondaryBraveGroupedBackground))
      Section {
        HStack {
          Text("State")
          Spacer()
          Text(feedDataSource.description(of: feedDataSource.state))
            .foregroundColor(.secondary)
        }
        HStack {
          Text("Following Locales")
          Spacer()
          Text(feedDataSource.followedLocales.joined(separator: ", "))
            .foregroundColor(.secondary)
            .multilineTextAlignment(.trailing)
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
        case .failure(let error as FeedDataSource.BraveNewsError):
          // Needed to get actual localized description defined in BraveNewsError
          // otherwise will show the generic error string
          Text(error.localizedDescription)
            .foregroundColor(.secondary)
        case .failure(let error):
          Text(error.localizedDescription)
            .foregroundColor(.secondary)
        }
        if !feedDataSource.decodingErrors.isEmpty {
          NavigationLink {
            List(feedDataSource.decodingErrors) { error in
              VStack(alignment: .leading) {
                Text(error.resourceName)
                  .font(.headline)
                Text(error.error)
                  .font(.subheadline)
                  .multilineTextAlignment(.leading)
              }
              .frame(maxWidth: .infinity)
            }
          } label: {
            HStack {
              Text("Decoding Errors")
              Spacer()
              Text(feedDataSource.decodingErrors.count, format: .number)
                .foregroundColor(.secondary)
            }
          }
        }
      }
      .listRowBackground(Color(.secondaryBraveGroupedBackground))
      Section {
        if let fileList {
          if fileList.isEmpty {
            Text("No Files Cached")
              .font(.footnote)
              .frame(maxWidth: .infinity)
              .foregroundColor(.secondary)
          } else {
            ForEach(fileList) { file in
              HStack {
                VStack(alignment: .leading) {
                  Text(file.name)
                  Text(
                    "\(Text(file.modifiedDate, format: .dateTime)) · \(Text(file.size.formatted(.byteCount(style: .file))).monospacedDigit())"
                  )
                  .frame(maxWidth: .infinity, alignment: .leading)
                  .foregroundColor(.secondary)
                  .font(.footnote)
                }
              }
              .padding(.vertical, 2)
            }
            .listRowBackground(Color(.secondaryBraveGroupedBackground))
            Button {
              Task {
                if await feedDataSource.clearCachedFiles() {
                  self.fileList = []
                }
              }
            } label: {
              Text("Clear News Cache")
                .foregroundColor(.red)
                .frame(maxWidth: .infinity)
            }
            .listRowBackground(Color(.secondaryBraveGroupedBackground))
          }
        } else {
          ProgressView()
            .progressViewStyle(.circular)
            .frame(maxWidth: .infinity)
        }
      } header: {
        Text("Cached Files")
      }
    }
    .listStyle(.insetGrouped)
    .listBackgroundColor(Color(UIColor.braveGroupedBackground))
    .navigationTitle("Brave News QA Settings")
    .navigationBarTitleDisplayMode(.inline)
    .animation(.default, value: fileList)
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
    .onChange(of: localeOverride) { locale in
      Preferences.BraveNews.selectedLocale.value = locale
      feedDataSource.selectedLocale = locale
    }
    .task {
      fileList = await fetchFileList()
    }
  }

  private func fetchFileList() async -> [NewsFile] {
    let resourceKeys: [URLResourceKey] = [.nameKey, .contentModificationDateKey, .fileSizeKey]
    let urls = await feedDataSource.fetchCachedFiles(
      resourceKeys: resourceKeys
    )
    return urls.compactMap { url in
      do {
        let values = try url.resourceValues(forKeys: Set(resourceKeys))
        guard let name = values.name,
          let size = values.fileSize,
          let modifiedDate = values.contentModificationDate
        else {
          return nil
        }
        return .init(url: url, name: name, size: size, modifiedDate: modifiedDate)
      } catch {
        return nil
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
