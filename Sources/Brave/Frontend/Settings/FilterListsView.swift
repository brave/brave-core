// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import SwiftUI
import Strings
import Data
import DesignSystem
import BraveUI
import BraveCore

/// A view showing enabled and disabled community filter lists
struct FilterListsView: View {
  @ObservedObject private var filterListStorage = FilterListStorage.shared
  @ObservedObject private var customFilterListStorage = CustomFilterListStorage.shared
  @Environment(\.editMode) private var editMode
  @State private var showingAddSheet = false
  private let dateFormatter = RelativeDateTimeFormatter()
  
  var body: some View {
    List {
      Section {
        ForEach($customFilterListStorage.filterListsURLs) { $filterListURL in
          VStack(alignment: .leading, spacing: 4) {
            Toggle(isOn: $filterListURL.setting.isEnabled) {
              VStack(alignment: .leading, spacing: 4) {
                Text(filterListURL.title)
                  .foregroundColor(Color(.bravePrimary))
                  .truncationMode(.middle)
                  .lineLimit(1)
                
                switch filterListURL.downloadStatus {
                case .downloaded(let downloadDate):
                  Text(String.localizedStringWithFormat(
                    Strings.filterListsLastUpdated,
                    dateFormatter.localizedString(for: downloadDate, relativeTo: Date())))
                    .font(.caption)
                    .foregroundColor(Color(.braveLabel))
                case .failure:
                  Text(Strings.filterListsDownloadFailed)
                    .font(.caption)
                    .foregroundColor(.red)
                case .pending:
                  Text(Strings.filterListsDownloadPending)
                    .font(.caption)
                    .foregroundColor(Color(.braveLabel))
                }
              }
            }
            .disabled(editMode?.wrappedValue.isEditing == true)
            .toggleStyle(SwitchToggleStyle(tint: .accentColor))
            .onChange(of: filterListURL.setting.isEnabled) { value in
              Task {
                CustomFilterListSetting.save(inMemory: !customFilterListStorage.persistChanges)
                await FilterListCustomURLDownloader.shared.handleUpdate(to: filterListURL, isEnabled: value)
              }
            }
            
          Text(filterListURL.setting.externalURL.absoluteDisplayString)
            .font(.caption)
            .foregroundColor(Color(.secondaryBraveLabel))
            .allowsTightening(true)
          }.listRowBackground(Color(.secondaryBraveGroupedBackground))
        }
        .onDelete(perform: onDeleteHandling)
        
        Button {
          showingAddSheet = true
        } label: {
          Text(Strings.addCustomFilterList)
            .foregroundColor(Color(.braveBlurpleTint))
        }
          .disabled(editMode?.wrappedValue.isEditing == true)
          .listRowBackground(Color(.secondaryBraveGroupedBackground))
          .popover(isPresented: $showingAddSheet, content: {
            FilterListAddURLView()
          })
      } header: {
        Text(Strings.customFilterLists)
      }
      
      Section {
        ForEach($filterListStorage.filterLists) { $filterList in
          Toggle(isOn: $filterList.isEnabled) {
            VStack(alignment: .leading) {
              Text(filterList.entry.title)
                .foregroundColor(Color(.bravePrimary))
              Text(filterList.entry.desc)
                .font(.caption)
                .foregroundColor(Color(.secondaryBraveLabel))
            }
          }.toggleStyle(SwitchToggleStyle(tint: .accentColor))
            .listRowBackground(Color(.secondaryBraveGroupedBackground))
        }
      } header: {
        VStack(alignment: .leading, spacing: 4) {
          Text(Strings.defaultFilterLists)
            .textCase(.uppercase)
          Text(Strings.filterListsDescription)
            .textCase(.none)
        }
      }
    }
    .animation(.default, value: customFilterListStorage.filterListsURLs)
    .listBackgroundColor(Color(UIColor.braveGroupedBackground))
    .listStyle(.insetGrouped)
    .navigationTitle(Strings.filterLists)
    .toolbar {
      EditButton().disabled(
        customFilterListStorage.filterListsURLs.isEmpty &&
        editMode?.wrappedValue.isEditing == false
      )
    }
  }
  
  private func onDeleteHandling(offsets: IndexSet) {
    let removedURLs = offsets.map { customFilterListStorage.filterListsURLs[$0] }
    customFilterListStorage.filterListsURLs.remove(atOffsets: offsets)
    
    if customFilterListStorage.filterListsURLs.isEmpty {
      editMode?.wrappedValue = .inactive
    }
    
    Task {
      await removedURLs.asyncConcurrentForEach { removedURL in
        // 1. Disable the filter list.
        // It would be better to delete it but for some reason if we remove the rule list,
        // it will not allow us to remove it from the tab
        // So we don't remove it, only flag it as disabled and it will be removed on the next launch
        // during the `cleaupInvalidRuleLists` step on `LaunchHelper`
        await FilterListCustomURLDownloader.shared.handleUpdate(
          to: removedURL, isEnabled: false
        )
        
        // 2. Stop downloading the file
        await FilterListCustomURLDownloader.shared.stopFetching(
          filterListCustomURL: removedURL
        )
        
        // 3. Remove the files
        do {
          try removedURL.setting.resource.removeCacheFolder()
        } catch {
          ContentBlockerManager.log.error(
            "Failed to remove file for resource \(removedURL.setting.uuid)"
          )
        }
        
        // 4. Remove the setting.
        // This should always happen in the end
        // because we need to access properties on the setting until then
        removedURL.setting.delete(inMemory: !customFilterListStorage.persistChanges)
      }
    }
  }
}

#if DEBUG
struct FilterListsView_Previews: PreviewProvider {
  static var previews: some View {
    Group {
      FilterListsView()
    }
    .previewLayout(.sizeThatFits)
  }
}
#endif
