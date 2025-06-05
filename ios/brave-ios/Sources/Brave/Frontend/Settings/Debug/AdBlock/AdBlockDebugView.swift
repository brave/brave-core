// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveShared
import Strings
import SwiftUI
import WebKit

struct AdBlockDebugView: View {
  var body: some View {
    Form {
      CompileContentBlockersSectionView()
      CorruptCacheSectionView()
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

      Picker(selection: $selection) {
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
