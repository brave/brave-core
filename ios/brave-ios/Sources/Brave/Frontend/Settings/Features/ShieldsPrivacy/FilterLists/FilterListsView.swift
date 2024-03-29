// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveUI
import Data
import DesignSystem
import Strings
import SwiftUI

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
        customFilterListView

        Button {
          showingAddSheet = true
        } label: {
          Text(Strings.addCustomFilterList)
            .foregroundColor(Color(.braveBlurpleTint))
        }
        .disabled(editMode?.wrappedValue.isEditing == true)
        .popover(
          isPresented: $showingAddSheet,
          content: {
            FilterListAddURLView()
          }
        )
      } header: {
        Text(Strings.customFilterLists)
      }
      .listRowBackground(Color(.secondaryBraveGroupedBackground))
      .toggleStyle(SwitchToggleStyle(tint: .accentColor))

      Section {
        filterListView
      } header: {
        VStack(alignment: .leading, spacing: 4) {
          Text(Strings.defaultFilterLists)
            .textCase(.uppercase)
          Text(Strings.filterListsDescription)
            .textCase(.none)
        }
      }.listRowBackground(Color(.secondaryBraveGroupedBackground))
    }
    .toggleStyle(SwitchToggleStyle(tint: .accentColor))
    .animation(.default, value: customFilterListStorage.filterListsURLs)
    .listBackgroundColor(Color(UIColor.braveGroupedBackground))
    .listStyle(.insetGrouped)
    .navigationTitle(Strings.contentFiltering)
    .toolbar {
      EditButton().disabled(
        customFilterListStorage.filterListsURLs.isEmpty && editMode?.wrappedValue.isEditing == false
      )
    }
    .onDisappear {
      Task.detached {
        await AdBlockGroupsManager.shared.ensureEnabledEngines()
      }
    }
  }

  @ViewBuilder private var filterListView: some View {
    ForEach($filterListStorage.filterLists) { $filterList in
      if !filterList.isHidden {
        Toggle(isOn: $filterList.isEnabled) {
          VStack(alignment: .leading) {
            Text(filterList.entry.title)
              .foregroundColor(Color(.bravePrimary))
            Text(filterList.entry.desc)
              .font(.caption)
              .foregroundColor(Color(.secondaryBraveLabel))
          }
        }
      }
    }
  }

  @ViewBuilder private var customFilterListView: some View {
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
              Text(
                String.localizedStringWithFormat(
                  Strings.filterListsLastUpdated,
                  dateFormatter.localizedString(for: downloadDate, relativeTo: Date())
                )
              )
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
        .onChange(of: filterListURL.setting.isEnabled) { isEnabled in
          Task {
            CustomFilterListSetting.save(inMemory: !customFilterListStorage.persistChanges)
          }
        }

        Text(filterListURL.setting.externalURL.absoluteDisplayString)
          .font(.caption)
          .foregroundColor(Color(.secondaryBraveLabel))
          .allowsTightening(true)
      }
    }
    .onDelete(perform: onDeleteHandling)
  }

  private func onDeleteHandling(offsets: IndexSet) {
    let removedURLs = offsets.map { customFilterListStorage.filterListsURLs[$0] }
    customFilterListStorage.filterListsURLs.remove(atOffsets: offsets)

    if customFilterListStorage.filterListsURLs.isEmpty {
      editMode?.wrappedValue = .inactive
    }

    Task {
      await removedURLs.asyncConcurrentForEach { removedURL in
        // 1. Remove content blocker
        // We cannot do this for some reason as
        // it will not allow us to remove it from the tab if we do.
        // But on next launch it will be removed
        // during the `cleaupInvalidRuleLists` step on `LaunchHelper`

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
