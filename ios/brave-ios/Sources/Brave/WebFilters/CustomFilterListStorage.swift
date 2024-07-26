// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveShared
import BraveShields
import BraveStrings
import Data
import Foundation
import Preferences
import WebKit

@MainActor class CustomFilterListStorage: ObservableObject {
  enum CustomRulesError: Error, LocalizedError {
    /// We limit the number of lines because UITextView cannot handle large text
    case tooManyLines(max: Int)
    /// A rule that failed, the line the rule is found on and the underlying error
    /// - Note: The error is usually not that descriptive so generally not too useful.
    case failedRule(rule: String, line: Int, error: Error)

    var errorDescription: String? {
      switch self {
      case .tooManyLines(let max):
        return String.localizedStringWithFormat(
          Strings.Shields.customFiltersTooManyLinesError,
          max
        )
      case .failedRule(let rule, let line, _):
        return String.localizedStringWithFormat(
          Strings.Shields.customFiltersInvalidRuleError,
          rule,
          line
        )
      }
    }
  }

  static let shared = CustomFilterListStorage(persistChanges: true)
  /// Max number of lines our custom filter list can be
  ///
  /// We have a max number of lines for adblock rules because too many lines will kill the editor.
  static let maxNumberOfCustomRulesLines = 10_000

  /// Store the current version of the custom rules so we know if we should upgrade it
  private static var customRuleListVersion = Preferences.Option<Int>(
    key: "shields.custom-rule-list-version",
    default: 0
  )

  /// Wether or not to store the data into disk or into memory
  let persistChanges: Bool
  /// A list of filter list URLs and their enabled statuses
  @Published var filterListsURLs: [FilterListCustomURL]

  init(persistChanges: Bool) {
    self.persistChanges = persistChanges
    self.filterListsURLs = []
  }

  func loadCachedFilterLists() async {
    let settings = CustomFilterListSetting.loadAllSettings(fromMemory: !persistChanges)

    self.filterListsURLs = await settings.asyncMap { setting in
      let resource = setting.resource
      let date = try? await resource.creationDate()

      if let date = date {
        return FilterListCustomURL(setting: setting, downloadStatus: .downloaded(date))
      } else {
        return FilterListCustomURL(setting: setting, downloadStatus: .pending)
      }
    }

    do {
      // Load the custom text filter list
      if let fileInfo = try await savedCustomRulesFileInfo() {
        AdBlockGroupsManager.shared.update(fileInfo: fileInfo)
      }
    } catch {
      ContentBlockerManager.log.error(
        "Failed to load custom filter list: \(String(describing: error))"
      )
    }
  }

  /// Ensures that the settings for a filter list are stored
  /// - Parameters:
  ///   - uuid: The uuid of the filter list to update
  ///   - isEnabled: A boolean indicating if the filter list is enabled or not
  public func ensureFilterList(for uuid: String, isEnabled: Bool) {
    // Enable the setting
    if let index = filterListsURLs.firstIndex(where: { $0.setting.uuid == uuid }) {
      filterListsURLs[index].setting.isEnabled = isEnabled
    }
  }

  /// Get the file URL to the custom filter list rules regardless if it is saved or not
  private func customRulesFileURL() throws -> URL {
    let folderURL = try AsyncFileManager.default.url(
      for: .applicationSupportDirectory,
      in: .userDomainMask
    ).appending(path: "custom_rules")
    return folderURL.appending(path: "list.txt")
  }

  /// Get the file URL to the custom filter list rules if it exists
  public func savedCustomRulesFileURL() throws -> URL? {
    let fileURL = try customRulesFileURL()

    if FileManager.default.fileExists(atPath: fileURL.path) {
      return fileURL
    } else {
      return nil
    }
  }

  public func savedCustomRulesFileInfo() async throws -> AdBlockEngineManager.FileInfo? {
    guard let fileURL = try savedCustomRulesFileURL() else { return nil }
    let versionNumber = Self.customRuleListVersion.value

    return AdBlockEngineManager.FileInfo(
      filterListInfo: GroupedAdBlockEngine.FilterListInfo(
        source: .filterListText,
        version: "\(versionNumber)"
      ),
      localFileURL: fileURL
    )
  }

  /// Load the custom filter list rules if they are saved
  public func loadCustomRules() async throws -> String? {
    guard let url = try savedCustomRulesFileURL() else { return nil }
    return try await Task.detached {
      try String(contentsOf: url)
    }.value
  }

  /// Save the custom filter list rules
  public func save(customRules: String) async throws {
    let lines = customRules.components(separatedBy: .newlines)
    if lines.count > Self.maxNumberOfCustomRulesLines {
      throw CustomRulesError.tooManyLines(max: Self.maxNumberOfCustomRulesLines)
    }

    if let failure = await AdBlockGroupsManager.shared.contentBlockerManager.testRules(
      forFilterSet: customRules
    ) {
      throw CustomRulesError.failedRule(
        rule: failure.rule,
        line: failure.line,
        error: failure.error
      )
    }

    let fileURL = try customRulesFileURL()
    if await AsyncFileManager.default.fileExists(atPath: fileURL.path) {
      try await AsyncFileManager.default.removeItem(at: fileURL)
    }

    _ = try await getOrCreateCustomRulesFolder()
    try customRules.write(to: fileURL, atomically: true, encoding: .utf8)
    Self.customRuleListVersion.value += 1
    guard let fileInfo = try await savedCustomRulesFileInfo() else { return }
    await AdBlockGroupsManager.shared.updateImmediately(fileInfo: fileInfo)
  }

  /// Delete the saved custom filter list rules
  func deleteCustomRules() async throws {
    guard let fileURL = try savedCustomRulesFileURL() else { return }
    try await AsyncFileManager.default.removeItem(at: fileURL)
    await AdBlockGroupsManager.shared.removeFileInfoImmediately(for: .filterListText)
  }

  /// Get or create a cache folder for the given `Resource`
  ///
  /// - Note: This technically can't really return nil as the location and folder are hard coded
  private func getOrCreateCustomRulesFolder() async throws -> URL {
    try await AsyncFileManager.default.url(
      for: .applicationSupportDirectory,
      appending: "custom_rules",
      create: true
    )
  }

  func update(filterListId id: ObjectIdentifier, with result: Result<Date, Error>) {
    guard let index = filterListsURLs.firstIndex(where: { $0.id == id }) else {
      return
    }

    switch result {
    case .failure(let error):
      #if DEBUG
      let externalURL = filterListsURLs[index].setting.externalURL.absoluteString
      ContentBlockerManager.log.error(
        "Failed to download resource \(externalURL): \(String(describing: error))"
      )
      #endif

      filterListsURLs[index].downloadStatus = .failure
    case .success(let date):
      filterListsURLs[index].downloadStatus = .downloaded(date)
    }
  }
}

extension CustomFilterListStorage {
  /// Gives us source representations of all the enabled custom filter lists
  @MainActor var enabledSources: [GroupedAdBlockEngine.Source] {
    var sources =
      filterListsURLs
      .filter(\.setting.isEnabled)
      .map(\.setting.engineSource)
    if (try? self.savedCustomRulesFileURL()) == nil { return sources }
    sources.append(.filterListText)
    return sources
  }

  /// Return all the sources available for the given engine type based on the available filter lists
  @MainActor func sources(
    for engineType: GroupedAdBlockEngine.EngineType
  ) -> [GroupedAdBlockEngine.Source] {
    var sources: [GroupedAdBlockEngine.Source] = []
    if engineType == .aggressive {
      sources.append(contentsOf: filterListsURLs.map(\.setting.engineSource))
    }
    if (try? self.savedCustomRulesFileURL()) == nil { return sources }
    sources.append(.filterListText)
    return sources
  }
}
