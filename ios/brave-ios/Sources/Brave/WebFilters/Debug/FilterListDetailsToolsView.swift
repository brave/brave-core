// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import SwiftUI
import BraveCore
import DesignSystem

@available(iOS 16.0, *)
struct FilterListDetailsToolsView: View {
  struct FilterListDetails {
    let source: CachedAdBlockEngine.Source
    let title: String
    let externalURL: URL?
    let lazyInfo: AdBlockStats.LazyFilterListInfo?
    var compiledInfo: CachedAdBlockEngine.FilterListInfo?
    var compiledResource: CachedAdBlockEngine.ResourcesInfo?
    var contentBlockers: [(ContentBlockerManager.BlockingMode, Result<WKContentRuleList, Error>)]
    var customRules: String
  }
  
  @Binding var details: FilterListDetails
  @State var rulesError: Error?
  @State var isRecompiled = false
  @State var isCopied = false
  @State var isResourceCopied = false
  @State var customRules: String
  
  var isSaved: Bool {
    return details.customRules == customRules
  }
  
  init(details: Binding<FilterListDetails>) {
    _details = details
    _customRules = State(initialValue: details.wrappedValue.customRules)
  }
  
  var body: some View {
    List {
      if let lazyInfo = details.lazyInfo {
        Section("File") {
          VStack(alignment: .leading) {
            HStack {
              Label("Rules", systemImage: "doc.plaintext")
              HStack {
                Text("v") + Text(lazyInfo.filterListInfo.version)
              }.foregroundStyle(.secondary)
              
              Spacer()
              ShareOrCopyURLView(url: lazyInfo.filterListInfo.localFileURL)
            }
            
            Text(lazyInfo.filterListInfo.localFileURL.path(percentEncoded: false))
              .foregroundStyle(.secondary)
              .font(.caption)
          }
        }.listRowBackground(Color(.secondaryBraveGroupedBackground))
      }
      
      if let externalURL = details.externalURL {
        Section("Links") {
          VStack {
            HStack {
              Label("External URL", systemImage: "link")
              ShareLink(item: externalURL)
            }
            
            Text(externalURL.absoluteString)
              .foregroundStyle(.secondary)
              .font(.caption)
          }
        }.listRowBackground(Color(.secondaryBraveGroupedBackground))
      }
      
      if let compiledInfo = details.compiledInfo {
        Section("Engine") {
          HStack {
            Label("Version", systemImage: "hammer")
            HStack {
              Text("v") + Text(compiledInfo.version)
            }.foregroundStyle(.secondary)
          }
          
          if let version = details.compiledResource?.version {
            HStack {
              Label("Resources", systemImage: "doc.plaintext")
              HStack {
                Text("v") + Text(version)
              }.foregroundStyle(.secondary)
            }
          }
        }.listRowBackground(Color(.secondaryBraveGroupedBackground))
      }
      
      if let blocklistType = details.lazyInfo?.blocklistType, !details.contentBlockers.isEmpty {
        Section("Content blockers") {
          ForEach(details.contentBlockers, id: \.0) { tuple in
            VStack {
              Label(blocklistType.makeIdentifier(for: tuple.0), systemImage: "shield")
              
              switch tuple.1 {
              case .success(let ruleList):
                Text(ruleList.description).font(.caption).foregroundStyle(.secondary)
              case .failure(let error):
                Text(String(describing: error)).font(.caption).foregroundStyle(.red)
              }
            }
          }
        }.listRowBackground(Color(.secondaryBraveGroupedBackground))
      }
      
      if details.lazyInfo?.filterListInfo.fileType == .text {
        Section {
          VStack {
            TextEditor(text: $customRules)
              .frame(height: 300)
            if let error = rulesError {
              Text(String(describing: error))
                .foregroundStyle(.red)
            }
          }
        } header: {
          HStack {
            Text("Additional rules")
              .frame(maxWidth: .infinity, alignment: .leading)
            if isSaved {
              Image(systemName: "checkmark")
                .foregroundStyle(.green)
            } else {
              Button("Save", systemImage: "arrow.down.doc") {
                saveCustomRules(for: details.source)
              }
              .buttonStyle(.borderedProminent)
              .foregroundStyle(.white)
              .font(.caption)
            }
          }
        }.listRowBackground(Color(.secondaryBraveGroupedBackground))
      }
    }
    .listBackgroundColor(Color(UIColor.braveGroupedBackground))
    .toolbar {
      ToolbarItem(placement: .automatic) {
        Button("Load", systemImage: "hammer") {
          guard let lazyInfo = details.lazyInfo else { return }
          saveCustomRules(for: details.source)
          isRecompiled = true
          
          Task {
            guard let resourcesInfo = await AdBlockStats.shared.resourcesInfo else { return }
            
            await AdBlockStats.shared.compile(
              lazyInfo: lazyInfo,
              resourcesInfo: resourcesInfo,
              compileContentBlockers: false, force: true
            )
            
            let blocklistType = lazyInfo.testBlocklistType
            do {
              try await ContentBlockerManager.shared.compileRuleList(
                at: lazyInfo.filterListInfo.localFileURL,
                for: blocklistType,
                modes: blocklistType.allowedModes
              )
            } catch {
              // Handle error
            }
            
            details.compiledInfo = await AdBlockStats.shared.cachedEngines[details.source]?.filterListInfo
            details.compiledResource = await AdBlockStats.shared.cachedEngines[details.source]?.resourcesInfo
            
            try await Task.sleep(for: .seconds(2))
            isRecompiled = false
          }
        }
        .buttonStyle(.borderedProminent)
        .foregroundStyle(.white)
        .font(.caption)
      }
    }
    .onChange(of: details.source, perform: { value in
      self.isCopied = false
      self.isRecompiled = false
    })
    .navigationTitle(details.title)
    .onDisappear(perform: {
      saveCustomRules(for: details.source)
    })
  }
  
  private func saveCustomRules(for source: CachedAdBlockEngine.Source) {
    do {
      if !customRules.isEmpty {
        _ = try AdblockEngine(rules: customRules)
        try source.save(additionalRules: customRules)
      } else {
        try source.deleteAdditionalRules()
      }
      
      details.customRules = customRules
    } catch {
      self.rulesError = error
    }
  }
    
}

#if swift(>=5.9)
@available(iOS 16.0, *)
#Preview {
  FilterListDetailsToolsView(
    details: .constant(FilterListDetailsToolsView.FilterListDetails(
      source: .adBlock, title: "AdBlock", externalURL: nil, lazyInfo: nil,
      compiledInfo: nil, compiledResource: nil, contentBlockers: [], customRules: ""
    ))
  )
}
#endif

extension CachedAdBlockEngine.Source {
  /// The directory to which we should store our debug files into
  private static var debugFolderDirectory: FileManager.SearchPathDirectory {
    return FileManager.SearchPathDirectory.applicationSupportDirectory
  }
  
  /// The file to save additional rules to
  private var debugFolderName: String {
    switch self {
    case .adBlock: return "debug/ad-block"
    case .filterListURL(let uuid): return "debug/\(uuid)"
    case .filterList(let componentId): return "debug/\(componentId)"
    }
  }
  
  private var debugFolderURL: URL? {
    guard let folderURL = Self.debugFolderDirectory.url else { return nil }
    return folderURL.appendingPathComponent(debugFolderName)
  }
  
  /// Get the cache folder for this resource
  ///
  /// - Note: Returns nil if the cache folder does not exist
  private var createdDebugFolderURL: URL? {
    guard let folderURL = debugFolderURL else { return nil }
    
    if FileManager.default.fileExists(atPath: folderURL.path) {
      return folderURL
    } else {
      return nil
    }
  }
  
  func loadAdditionalRules() throws -> String? {
    guard let folderURL = createdDebugFolderURL else { return nil }
    let fileURL = folderURL.appendingPathComponent("additional-rules", conformingTo: .text)
    guard FileManager.default.fileExists(atPath: fileURL.path) else { return nil }
    return try String(contentsOf: fileURL, encoding: .utf8)
  }
  
  func save(additionalRules: String) throws {
    let folderURL = try getOrCreateDebugFolder()
    let fileURL = folderURL.appendingPathComponent("additional-rules", conformingTo: .text)
    try additionalRules.write(to: fileURL, atomically: true, encoding: .utf8)
  }
  
  func deleteAdditionalRules() throws {
    guard let folderURL = createdDebugFolderURL else { return }
    let fileURL = folderURL.appendingPathComponent("additional-rules", conformingTo: .text)
    guard FileManager.default.fileExists(atPath: fileURL.path) else { return }
    try FileManager.default.removeItem(at: fileURL)
  }

  /// Get or create a debug folder for this filter list
  private func getOrCreateDebugFolder() throws -> URL {
    guard let folderURL = FileManager.default.getOrCreateFolder(
      name: debugFolderName,
      location: Self.debugFolderDirectory
    ) else {
      throw ResourceFileError.failedToCreateCacheFolder
    }
    
    return folderURL
  }
}

private extension AdBlockStats.LazyFilterListInfo {
  var testBlocklistType: ContentBlockerManager.BlocklistType {
    switch filterListInfo.source {
    case .adBlock:
      return .generic(.blockAds)
    case .filterList(let componentId):
      return .filterList(componentId: componentId, isAlwaysAggressive: isAlwaysAggressive)
    case .filterListURL(let uuid):
      return .customFilterList(uuid: uuid)
    }
  }
}
