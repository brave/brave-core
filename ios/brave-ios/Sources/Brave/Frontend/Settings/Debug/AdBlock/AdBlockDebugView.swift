// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import SwiftUI
import WebKit

struct AdBlockDebugView: View {
  var body: some View {
    Form {
      CompileContentBlockersSectionView()
    }
  }
}

#Preview {
  AdBlockDebugView()
}

private struct CompileContentBlockersSectionView: View {
  @AppStorage("numberOfRuns") private var numberOfRuns: Int = 10
  @AppStorage("selectionId") private var selectionId: String = ""
  @State private var currentRun = 0
  @State private var currentAverage = 0
  @State private var result: Result<Duration, Error>?
  @State private var availableSelections: [AdBlockEngineManager.FileInfo] = []
  @State private var task: Task<Void, Never>?
  @State private var stopping = false

  var selection: AdBlockEngineManager.FileInfo? {
    return availableSelections.first(where: {
      $0.filterListInfo.source.debugDescription == selectionId
    })
  }

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

      Picker("Filter List", selection: $selectionId) {
        ForEach(availableSelections, id: \.filterListInfo.source) { selection in
          Group {
            Text(getTitle(for: selection)) + Text(verbatim: " v\(selection.filterListInfo.version)")
          }.tag(selection.filterListInfo.source.debugDescription)
        }
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
      })

      if selectionId.isEmpty, let selection = availableSelections.first {
        self.selectionId = selection.filterListInfo.source.debugDescription
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
