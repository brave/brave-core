// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShields
import BraveUI
import Data
import DesignSystem
import Strings
import SwiftUI

/// A view showing enabled and disabled community filter lists
struct FilterListsView: View {
  private static let dateFormatter = RelativeDateTimeFormatter()
  enum FilterListUpdateStatus: Equatable {
    case unknown
    case updating
    case updated

    var label: String {
      switch self {
      case .unknown: Strings.Shields.updateLists
      case .updated: Strings.Shields.listsUpdated
      case .updating: Strings.Shields.updatingLists
      }
    }

    var braveImageName: String {
      switch self {
      case .unknown, .updating: "leo.refresh"
      case .updated: "leo.check.circle-outline"
      }
    }

    var forgroundColor: Color {
      switch self {
      case .unknown: Color(.braveBlurpleTint)
      case .updated: Color(.braveSuccessLabel)
      case .updating: Color(.braveDisabled)
      }
    }
  }

  @ObservedObject private var filterListStorage = FilterListStorage.shared
  @ObservedObject private var customFilterListStorage = CustomFilterListStorage.shared
  @Environment(\.editMode) private var editMode
  @State private var showingAddSheet = false
  @State private var showingCustomFiltersSheet = false
  @State private var customRules: String?
  @State private var rulesError: Error?
  @State private var filterListsUpdateStatus = FilterListUpdateStatus.unknown
  @State private var customFilterListsUpdateStatus = FilterListUpdateStatus.unknown
  @State private var customFilterListsUpdateError: Error? = nil
  @State private var searchText = ""

  var body: some View {
    List {
      Section {
        if !customFilterListStorage.filterListsURLs.isEmpty && searchText.isEmpty {
          updateFilterListsButton(
            status: customFilterListsUpdateStatus,
            error: customFilterListsUpdateError
          ) {
            customFilterListsUpdateStatus = .updating
            customFilterListsUpdateError = nil
            Task {
              do {
                try await FilterListCustomURLDownloader.shared.updateFilterLists()
                customFilterListsUpdateStatus = .updated
              } catch {
                customFilterListsUpdateError = error
                customFilterListsUpdateStatus = .unknown
              }
            }
          }
        }
        externalFilterListRows
      } header: {
        VStack(alignment: .leading, spacing: 4) {
          Text(Strings.Shields.externalFilterLists)
            .textCase(.uppercase)
          Text(Strings.Shields.addCustomFilterListDescription)
            .textCase(.none)
        }
      }
      .listRowBackground(Color(.secondaryBraveGroupedBackground))
      .toggleStyle(SwitchToggleStyle(tint: .accentColor))

      if searchText.isEmpty {
        Section {
          customFiltersRows
        } header: {
          VStack(alignment: .leading, spacing: 4) {
            Text(Strings.Shields.customFilters)
              .textCase(.uppercase)
            Text(Strings.Shields.customFiltersDescription)
              .textCase(.none)
          }
        }
        .listRowBackground(Color(.secondaryBraveGroupedBackground))
      }

      Section {
        if searchText.isEmpty {
          updateFilterListsButton(status: filterListsUpdateStatus, error: nil) {
            filterListsUpdateStatus = .updating
            Task {
              await updateFilterLists()
              filterListsUpdateStatus = .updated
            }
          }
        }
        defaultFilterListRows
      } header: {
        VStack(alignment: .leading, spacing: 4) {
          Text(Strings.Shields.defaultFilterLists)
            .textCase(.uppercase)
          Text(Strings.Shields.filterListsDescription)
            .textCase(.none)
        }
      }.listRowBackground(Color(.secondaryBraveGroupedBackground))
    }
    .fullScreenCover(
      isPresented: $showingCustomFiltersSheet,
      content: {
        NavigationView {
          CustomFilterListView(customRules: $customRules)
        }
      }
    )
    .searchable(text: $searchText)
    .toggleStyle(SwitchToggleStyle(tint: .accentColor))
    .animation(.default, value: customFilterListStorage.filterListsURLs)
    .scrollContentBackground(.hidden)
    .background(Color(UIColor.braveGroupedBackground))
    .listStyle(.insetGrouped)
    .navigationTitle(Strings.Shields.contentFiltering)
    .toolbar {
      EditButton().disabled(
        customFilterListStorage.filterListsURLs.isEmpty && editMode?.wrappedValue.isEditing == false
      )
    }
    .task {
      await loadCustomRules()
    }
  }

  private func updateFilterListsButton(
    status: FilterListUpdateStatus,
    error: Error?,
    action: @escaping () -> Void
  ) -> some View {
    VStack(alignment: .leading) {
      Button(
        action: action,
        label: {
          Label(
            status.label,
            braveSystemImage: status.braveImageName
          )
        }
      )
      .labelStyle(.titleAndIcon)
      .foregroundStyle(status.forgroundColor)
      .disabled(status == .updating)
      if let error {
        Text(error.localizedDescription)
          .foregroundStyle(Color(.braveErrorLabel))
          .font(.caption)
      }
    }
  }

  private var customFiltersAccessibilityLabel: Text {
    if let customRules = customRules {
      Text(customRules)
    } else if let error = rulesError {
      Text(error.localizedDescription)
    } else {
      Text(Strings.Shields.customFiltersPlaceholder)
    }
  }

  @ViewBuilder private var customFiltersRows: some View {
    Button {
      showingCustomFiltersSheet = true
    } label: {
      HStack(alignment: .center) {
        if let customRules = customRules {
          Text(customRules)
            .lineLimit(2)
            .multilineTextAlignment(.leading)
            .foregroundStyle(Color(.braveLabel))
            .font(.system(size: 14, weight: .regular, design: .monospaced))
            .frame(maxWidth: .infinity, alignment: .leading)
        } else if let error = rulesError {
          Text(error.localizedDescription)
            .foregroundStyle(Color(.braveErrorLabel))
            .font(.subheadline)
            .frame(maxWidth: .infinity, alignment: .leading)
        } else {
          Text(Strings.Shields.customFiltersPlaceholder)
            .foregroundStyle(Color(.secondaryBraveLabel))
            .font(.subheadline)
            .frame(maxWidth: .infinity, alignment: .leading)
        }
        Image(systemName: "chevron.right")
          .font(.body.weight(.semibold))
          .foregroundColor(Color(.separator))
      }
      .accessibilityElement()
      .accessibilityLabel(customFiltersAccessibilityLabel)
    }
  }

  @ViewBuilder private var defaultFilterListRows: some View {
    let searchText = searchText.lowercased()
    #if DEBUG
    let allEnabled = Binding {
      filterListStorage.filterLists.allSatisfy({
        $0.isEnabled || !$0.satisfies(searchText: searchText)
      })
    } set: { isEnabled in
      filterListStorage.filterLists.enumerated().forEach { index, filterList in
        guard filterList.satisfies(searchText: searchText) else { return }
        let isEnabled = filterList.entry.hidden ? filterList.entry.defaultEnabled : isEnabled
        filterListStorage.filterLists[index].isEnabled = isEnabled
      }
    }

    Toggle(isOn: allEnabled) {
      VStack(alignment: .leading) {
        Text("All").foregroundColor(Color(.bravePrimary))
      }
    }
    #endif

    ForEach($filterListStorage.filterLists) { $filterList in
      if !filterList.isHidden && filterList.satisfies(searchText: searchText) {
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

  @ViewBuilder private var externalFilterListRows: some View {
    let searchText = searchText.lowercased()
    ForEach($customFilterListStorage.filterListsURLs) { $filterListURL in
      if filterListURL.satisfies(searchText: searchText) {
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
                    Strings.Shields.filterListsLastUpdated,
                    Self.dateFormatter.localizedString(for: downloadDate, relativeTo: Date())
                  )
                )
                .font(.caption)
                .foregroundColor(Color(.braveLabel))
              case .failure:
                Text(Strings.Shields.filterListsDownloadFailed)
                  .font(.caption)
                  .foregroundColor(Color(.braveErrorLabel))
              case .pending:
                Text(Strings.Shields.filterListsDownloadPending)
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
    }
    .onDelete(perform: onDeleteHandling)

    if searchText.isEmpty {
      Button {
        showingAddSheet = true
      } label: {
        Text(Strings.Shields.addFilterByURL)
          .foregroundColor(Color(.braveBlurpleTint))
      }
      .disabled(editMode?.wrappedValue.isEditing == true)
      .popover(
        isPresented: $showingAddSheet,
        content: {
          FilterListAddURLView()
        }
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
        // 1. Remove content blocker
        // We cannot do this for some reason as
        // it will not allow us to remove it from the tab if we do.
        // But on next launch it will be removed
        // during the `cleaupInvalidRuleLists` step on `LaunchHelper`

        // 2. Stop downloading the file
        FilterListCustomURLDownloader.shared.stopFetching(
          filterListCustomURL: removedURL
        )

        // 3. Remove the files
        do {
          try await removedURL.setting.resource.removeCacheFolder()
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

  private func loadCustomRules() async {
    do {
      self.customRules = try await customFilterListStorage.loadCustomRules()
    } catch {
      rulesError = error
      customRules = nil
    }
  }

  private func updateFilterLists() async {
    _ = await FilterListStorage.shared.updateFilterLists()
    await AdblockResourceDownloader.shared.updateResources()
    await AdBlockGroupsManager.shared.compileEngines()
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

extension FilterList {
  fileprivate func satisfies(searchText: String) -> Bool {
    guard !searchText.isEmpty else { return true }
    return entry.title.contains(searchText) || entry.desc.contains(searchText)
  }
}

extension FilterListCustomURL {
  @MainActor fileprivate func satisfies(searchText: String) -> Bool {
    guard !searchText.isEmpty else { return true }
    return setting.externalURL.absoluteString.contains(searchText)
  }
}
