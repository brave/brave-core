// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveShared
import BraveUI
import Strings
import SwiftUI
import WebKit

struct AdBlockDebugView: View {
  var body: some View {
    Form {
      CompileContentBlockersSectionView()
      CorruptCacheSectionView()
      NavigationLink(
        destination: AdblockRuleExclusionView(),
        label: {
          Text("Adblock Rule Exclusions")
        }
      )
    }
  }
}

#Preview {
  AdBlockDebugView()
}

private struct CompileContentBlockersSectionView: View {
  @AppStorage("numberOfRuns") private var numberOfRuns: Int = 10
  @State private var selection: AdBlockEngineManager.FileInfo?
  @State private var currentRun = 0
  @State private var currentAverage = 0
  @State private var result: Result<Duration, Error>?
  @State private var availableSelections: [AdBlockEngineManager.FileInfo] = []
  @State private var task: Task<Void, Never>?
  @State private var stopping = false

  var body: some View {
    Section {
      HStack {
        Text("Number of Runs")
          .frame(maxWidth: .infinity, alignment: .leading)
        TextField(
          "Enter a number greater than 0",
          value: $numberOfRuns,
          formatter: NumberFormatter()
        )
        .keyboardType(.numberPad)
        .multilineTextAlignment(.trailing)
      }

      FormPicker(selection: $selection) {
        ForEach(availableSelections) { selection in
          Group {
            Text(getTitle(for: selection)) + Text(verbatim: " v\(selection.filterListInfo.version)")
          }.tag(selection)
        }
      } label: {
        Text("Filter List")
      }

      if let result {
        switch result {
        case .success(let averageTime):
          Text("Average compile time: \(averageTime.formatted())")
            .foregroundStyle(.green)
        case .failure(let failure):
          Text("Failed to compile: \(String(describing: failure))")
            .foregroundStyle(.red)
        }
      }

      if task != nil {
        VStack(alignment: .leading) {
          HStack {
            ProgressView(
              "Run \(currentRun + 1) of \(numberOfRuns)",
              value: Double(currentRun),
              total: Double(numberOfRuns)
            )
            Button(stopping ? "Stopping" : "Stop", action: stopCompiling)
              .disabled(stopping)
          }
          if let selection = selection {
            Text("Compiling \(selection.filterListInfo.debugDescription)")
              .foregroundStyle(.tertiary)
              .font(.caption)
          }
        }
      } else {
        Button("Start", action: startCompiling)
          .disabled(selection == nil)
      }
    } header: {
      Text("Compile Content Blockers")
    } footer: {
      Text(
        """
        This will compile content blockers from the selected filter list \(numberOfRuns) times and calculate the average compile time.
        This process may take a long time depending on the number of runs set and the filter list selected.
        """
      )
    }
    .task {
      self.availableSelections = GroupedAdBlockEngine.EngineType.allCases.flatMap({
        engineType -> [AdBlockEngineManager.FileInfo] in
        var sources = AdBlockGroupsManager.shared.sourceProvider.sources(for: engineType)
        for source in sources {
          guard source.contentBlockerSource != source else { continue }
          sources.append(source.contentBlockerSource)
        }

        return AdBlockGroupsManager.shared.getManager(for: engineType).compilableFiles(
          for: sources
        )
      }).uniqued(on: \.id)

      if selection == nil {
        self.selection = availableSelections.first
      }
    }
  }

  @MainActor private func stopCompiling() {
    stopping = true
    task?.cancel()
  }

  @MainActor private func startCompiling() {
    guard let selection = selection else { return }
    result = nil
    currentRun = 0
    stopping = false

    self.task = Task {
      let clock = ContinuousClock()
      var durations: [Duration] = []

      do {
        for _ in 0..<numberOfRuns {
          try Task.checkCancellation()
          let start = clock.now
          let source = selection.filterListInfo.source

          try await AdBlockGroupsManager.shared.contentBlockerManager.compileRuleList(
            at: selection.localFileURL,
            for: .engineSource(source, engineType: .standard),
            version: selection.filterListInfo.version,
            modes: [.standard, .aggressive]
          )

          durations.append(clock.now - start)
          currentRun += 1
        }

        result = .success(
          durations.reduce(Duration.zero, { $0 + $1 }) / durations.count
        )
      } catch is CancellationError {
        result = .success(
          durations.reduce(Duration.zero, { $0 + $1 }) / durations.count
        )
      } catch {
        result = .failure(error)
      }

      self.stopping = false
      self.task = nil
    }
  }

  @MainActor private func getTitle(for fileInfo: AdBlockEngineManager.FileInfo) -> String {
    switch fileInfo.filterListInfo.source {
    case .slimList:
      return "Slim List"
    case .filterList(let componentId):
      return FilterListStorage.shared.filterLists.first(
        where: { $0.entry.componentId == componentId }
      )?.entry.title ?? componentId
    case .filterListText:
      return "filterListText"
    case .filterListURL(let uuid):
      return CustomFilterListStorage.shared.filterListsURLs.first(
        where: { $0.setting.uuid == uuid }
      )?.setting.externalURL.lastPathComponent ?? uuid
    }
  }
}

private struct CorruptCacheSectionView: View {
  enum CacheResult: Error, Identifiable {
    case success
    case noCache
    case failedStandard
    case failedAggressive

    var id: String {
      return message
    }

    var title: String {
      switch self {
      case .success:
        return "Success"
      case .noCache, .failedStandard, .failedAggressive:
        return "Failure"
      }
    }

    var message: String {
      switch self {
      case .success:
        return "Successfully corrupted adblock cache."
      case .noCache:
        return "Cache unavailable."
      case .failedStandard:
        return "Failed to corrupt standard cache."
      case .failedAggressive:
        return "Failed to corrupt aggressive cache."
      }
    }
  }
  @State private var corruptCacheResult: CacheResult?

  var body: some View {
    Section {
      Button(action: corruptAdblockDATCache) {
        Text("Corrupt Adblock Engine DAT Caches")
      }
    }
    .alert(item: $corruptCacheResult) { corruptCacheResult in
      Alert(
        title: Text(corruptCacheResult.title),
        message: Text(corruptCacheResult.message),
        dismissButton: .default(Text(Strings.OKString))
      )
    }
  }

  private func corruptAdblockDATCache() {
    Task {
      guard
        let folderURL = try? AsyncFileManager.default.url(
          for: .cachesDirectory,
          in: .userDomainMask
        )
      else {
        self.corruptCacheResult = .noCache
        return
      }

      let standardCacheFolderURL =
        folderURL
        .appendingPathComponent("engines", conformingTo: .folder)
        .appendingPathComponent("standard", conformingTo: .folder)
      if await !corruptListDATFile(in: standardCacheFolderURL) {
        self.corruptCacheResult = .failedStandard
        return
      }

      let aggressiveCacheFolderURL =
        folderURL
        .appendingPathComponent("engines", conformingTo: .folder)
        .appendingPathComponent("aggressive", conformingTo: .folder)
      if await !corruptListDATFile(in: aggressiveCacheFolderURL) {
        self.corruptCacheResult = .failedAggressive
        return
      }
      return self.corruptCacheResult = .success
    }
  }

  private func corruptListDATFile(in directory: URL) async -> Bool {
    guard await AsyncFileManager.default.fileExists(atPath: directory.path) else {
      return false
    }
    let cachedDATFile = directory.appendingPathComponent("list.dat", conformingTo: .data)
    guard await AsyncFileManager.default.fileExists(atPath: cachedDATFile.path) else {
      // if file doesn't exist, we can't corrupt it.
      return false
    }
    guard let content = await AsyncFileManager.default.contents(atPath: cachedDATFile.path),
      var corruptedData = UUID().uuidString.data(using: .utf8)
    else {
      return false
    }
    // prefix UUID string to existing data to corrupt it
    corruptedData.append(content)
    // remove the cached DAT file
    try? await AsyncFileManager.default.removeItem(atPath: cachedDATFile.path)
    // 'corrupt' by replacing with corrupted data format
    await AsyncFileManager.default.createFile(atPath: cachedDATFile.path, contents: corruptedData)

    return true
  }
}

struct AdblockRuleExclusionView: View {

  /// The rules to exclude from AdBlockEngine & Content Blockers
  @State private var rulesToExclude = ""
  /// Any errors that are seen during saving or editing which we can display
  @State private var rulesError: Error?
  /// Indicates if we are currently loading the custom exclusion rules
  @State private var isLoading = false
  /// Indicates if we are currently saving the custom exclusion rules
  @State private var isSaving = false
  @Environment(\.dismiss) private var dismiss: DismissAction
  private var customFilterListStorage = CustomFilterListStorage.shared

  private func loadExclusionRules() async {
    isLoading = true
    defer { isLoading = false }

    do {
      self.rulesToExclude = try await customFilterListStorage.loadCustomExclusionRules() ?? ""
    } catch {
      rulesError = error
    }
  }

  private func saveExlusionRules() {
    guard !isLoading && !isSaving else { return }
    Task {
      isSaving = true
      defer { isSaving = false }
      do {
        // Force reset the Content Blocker rule list caches.
        try? await AdBlockGroupsManager.shared.contentBlockerManager.removeAllRuleLists()
        if !rulesToExclude.isEmpty {
          try await customFilterListStorage.save(customExclusionRules: rulesToExclude)
        } else {
          try await customFilterListStorage.deleteCustomExclusionRules()
        }

        dismiss()
      } catch {
        // Could not load the rules
        self.rulesError = error
      }
    }
  }

  private var saveToolbarItem: ToolbarItem<(), some View> {
    ToolbarItem(placement: .confirmationAction) {
      if isSaving {
        ProgressView()
      } else {
        Button(
          action: saveExlusionRules,
          label: {
            Label(Strings.saveButtonTitle, braveSystemImage: "leo.check.normal")
              .labelStyle(.titleOnly)
          }
        )
      }
    }
  }

  var body: some View {
    Form {
      Section(
        content: {
          TextEditor(text: $rulesToExclude)
            .font(.system(size: 14, weight: .regular).monospaced())
            .foregroundStyle(Color(braveSystemName: .textPrimary))
            .frame(height: 400)
            .overlay(
              alignment: isLoading ? .center : .topLeading,
              content: {
                if isLoading {
                  ProgressView()
                } else {
                  Text("Enter rules to exclude them from AdBlock Engine and Content Blockers")
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
                    .foregroundColor(Color(braveSystemName: .textDisabled))
                    .opacity(rulesToExclude.isEmpty ? 1 : 0)
                    .accessibilityHidden(!rulesToExclude.isEmpty)
                }
              }
            )
            .background(
              Color(.secondaryBraveGroupedBackground),
              in: RoundedRectangle(cornerRadius: 12, style: .continuous)
            )
        },
        header: {
          Text("Rules to Exclude")
        },
        footer: {
          Text("Rules should be separated by new lines and match the exact rule you are excluding.")
        }
      )
    }
    .scrollContentBackground(.hidden)
    .scrollDismissesKeyboard(.interactively)
    .background(
      Color(.secondaryBraveBackground)
        .edgesIgnoringSafeArea(.all)
    )
    .navigationTitle(Text("AdBlock Rule Exclusions"))
    .navigationBarTitleDisplayMode(.inline)
    .toolbar {
      saveToolbarItem
    }
    .task {
      await loadExclusionRules()
    }
  }
}
