// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import SwiftUI
import DesignSystem
import WebKit

@available(iOS 16.0, *)
struct FilterListToolsView: View {
  enum Selection: Hashable {
    case source(CachedAdBlockEngine.Source)
    case resources(CachedAdBlockEngine.ResourcesInfo)
  }
  
  @Environment(\.dismiss) var dismiss
  @State private var selection: Selection?
  @State private var details: FilterListDetailsToolsView.FilterListDetails?
  @State private var resourcesInfo: CachedAdBlockEngine.ResourcesInfo?
  
  var body: some View {
    NavigationSplitView {
      listView
    } detail: {
      switch selection {
      case .source:
        if let details = self.details {
          let bindingDetails = Binding {
            return self.details ?? details
          } set: { newDetails in
            self.details = newDetails
          }

          FilterListDetailsToolsView(
            details: bindingDetails
          )
        }
      case .resources(let resourcesInfo):
        ResourcesToolsView(resourcesInfo: resourcesInfo)
      case nil:
        Text("Select item")
      }
    }
  }
  
  @ViewBuilder private func makeRow(
    source: CachedAdBlockEngine.Source, title: String, subtitle: String?
  ) -> some View {
    NavigationLink(value: Selection.source(source)) {
      VStack(alignment: .leading) {
        Text(title)
        if let subtitle = subtitle {
          Text(subtitle)
            .font(.caption)
            .foregroundStyle(.secondary)
        }
      }
    }
  }
  
  @ViewBuilder @MainActor private var listView: some View {
    List(selection: $selection) {
      Section("Default") {
        makeRow(source: .adBlock, title: "Ad-block", subtitle: nil)
        
        if let resourcesInfo = resourcesInfo {
          NavigationLink(value: Selection.resources(resourcesInfo)) {
            HStack {
              Text("Resources")
                .frame(maxWidth: .infinity, alignment: .leading)
              Text(resourcesInfo.version)
                .font(.caption)
                .foregroundStyle(.secondary)
            }
          }
        }
      }.listRowBackground(Color(.secondaryBraveGroupedBackground))
      
      if !CustomFilterListStorage.shared.filterListsURLs.isEmpty {
        Section("Custom filter lists") {
          ForEach(CustomFilterListStorage.shared.filterListsURLs) { filterList in
            makeRow(
              source: filterList.setting.engineSource,
              title: filterList.title,
              subtitle: filterList.setting.externalURL.path(percentEncoded: false)
            )
          }
        }.listRowBackground(Color(.secondaryBraveGroupedBackground))
      }
      
      Section("Filter lists") {
        ForEach(FilterListStorage.shared.filterLists) { filterList in
          makeRow(
            source: filterList.engineSource,
            title: filterList.entry.title,
            subtitle: filterList.entry.componentId
          )
        }
      }.listRowBackground(Color(.secondaryBraveGroupedBackground))
    }.onChange(of: selection, perform: { value in
      guard let selection = selection else {
        details = nil
        return
      }
      
      switch selection {
      case .source(let source):        
        Task {
          await onChange(of: source)
        }
      case .resources:
        details = nil
      }
    })
    .listBackgroundColor(Color(UIColor.braveGroupedBackground))
    .toolbar {
      ToolbarItem(placement: .automatic) {
        Button("Close") {
          dismiss()
        }
      }
    }
    .navigationTitle("Filter lists")
    .onAppear {
      Task {
        resourcesInfo = await AdBlockStats.shared.resourcesInfo
      }
    }
  }
  
  @MainActor private func onChange(of source: CachedAdBlockEngine.Source) async {
    switch source {
    case .adBlock:
      await setDetails(
        source: source, title: "Ad-block (default)", 
        externalURL: nil
      )
      
    case .filterList(let componentId):
      guard let filterList = FilterListStorage.shared.filterLists.first(
        where: { $0.entry.componentId == componentId }
      ) else {
        details = nil
        return
      }
      
      await setDetails(
        source: source, title: filterList.entry.title, 
        externalURL: nil
      )
    case .filterListURL(let uuid):
      guard let filterList = CustomFilterListStorage.shared.filterListsURLs.first(
        where: { $0.setting.uuid == uuid }
      ) else {
        details = nil
        return
      }
      
      await setDetails(
        source: source, title: filterList.title,
        externalURL: filterList.setting.externalURL
      )
    }
  }
  
  private func setDetails(source: CachedAdBlockEngine.Source, title: String, externalURL: URL?) async {
    let lazyInfo = await AdBlockStats.shared.availableFilterLists[source]
    let compiledInfo = await AdBlockStats.shared.cachedEngines[source]?.filterListInfo
    let compiledResource = await AdBlockStats.shared.cachedEngines[source]?.resourcesInfo
    var contentBlockers: [(ContentBlockerManager.BlockingMode, Result<WKContentRuleList, Error>)] = []
    
    if let blocklistType = lazyInfo?.blocklistType {
      for allowedMode in blocklistType.allowedModes {
        do {
          if let ruleList = try await ContentBlockerManager.shared.ruleList(for: blocklistType, mode: allowedMode) {
            contentBlockers.append((allowedMode, .success(ruleList)))
          }
        } catch {
          contentBlockers.append(
            (allowedMode, .failure(error))
          )
        }
      }
    }
    let customRules = (try? source.loadAdditionalRules()) ?? ""
    
    details = FilterListDetailsToolsView.FilterListDetails(
      source: source,
      title: title,
      externalURL: externalURL,
      lazyInfo: lazyInfo,
      compiledInfo: compiledInfo,
      compiledResource: compiledResource,
      contentBlockers: contentBlockers,
      customRules: customRules
    )
  }
  
  @MainActor private func getLazyInfo(for source: CachedAdBlockEngine.Source) async -> AdBlockStats.LazyFilterListInfo? {
    return await AdBlockStats.shared.availableFilterLists[source]
  }
}

#if swift(>=5.9)
@available(iOS 16.0, *)
#Preview {
  FilterListToolsView()
}
#endif
