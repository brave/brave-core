// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import SwiftUI
import WebKit
import BraveUI
import Strings

struct ManageWebsiteDataView: View {
  @State private var isLoading: Bool = false
  @State private var dataRecords: [WKWebsiteDataRecord] = []
  @State private var filter: String = ""
  @State private var selectedRecordsIds: Set<String> = []
  @State private var editMode: EditMode = .inactive
  @Environment(\.presentationMode) @Binding private var presentationMode

  private var filteredRecords: [WKWebsiteDataRecord] {
    if filter.isEmpty {
      return dataRecords
    }
    return dataRecords.filter {
      $0.displayName.lowercased().contains(filter.lowercased())
    }
  }

  private func removeRecords(_ records: [WKWebsiteDataRecord]) {
    let recordIds = records.map(\.id)
    WKWebsiteDataStore.default()
      .removeData(
        ofTypes: WKWebsiteDataStore.allWebsiteDataTypes(),
        for: records
      ) {
        withAnimation {
          if dataRecords.count == records.count {
            dataRecords.removeAll()
          } else {
            dataRecords.removeAll(where: { recordIds.contains($0.displayName) })
          }
        }
      }
  }

  private func removeRecordsWithIds(_ recordIds: [String]) {
    let records = dataRecords.filter { recordIds.contains($0.id) }
    removeRecords(records)
  }

  private var isEditMode: Bool {
    !selectedRecordsIds.isEmpty && editMode == .active
  }

  private func visibleSelectedRecordIds(
    _ visibleRecords: [WKWebsiteDataRecord]
  ) -> [WKWebsiteDataRecord.ID] {
    selectedRecordsIds.filter { id in
      visibleRecords.contains(where: { $0.id == id })
    }
  }

  private func removeButtonTitle(visibleRecords: [WKWebsiteDataRecord]) -> String {
    let visibleSelectedRecordsIds = visibleSelectedRecordIds(visibleRecords)
    if isEditMode {
      if visibleSelectedRecordsIds.count == 1 {
        return Strings.removeDataRecord
      }
      return String.localizedStringWithFormat(Strings.removeSelectedDataRecord, visibleSelectedRecordsIds.count)
    }
    return Strings.removeAllDataRecords
  }

  private func loadRecords() {
    isLoading = true
    WKWebsiteDataStore.default()
      .fetchDataRecords(
        ofTypes: WKWebsiteDataStore.allWebsiteDataTypes(),
        completionHandler: { records in
          withAnimation {
            self.dataRecords =
              records
              .filter { $0.displayName != "localhost" }
              .sorted(by: { $0.displayName < $1.displayName })
            self.isLoading = false
          }
        })
  }

  var body: some View {
    NavigationView {
      let visibleRecords = filteredRecords
      List(selection: $selectedRecordsIds) {
        Section {
          if isLoading {
            HStack {
              ProgressView()
              Text(Strings.loadingWebsiteData)
            }
          } else {
            ForEach(visibleRecords) { record in
              VStack(alignment: .leading, spacing: 2) {
                let types = Set(
                  record.dataTypes.compactMap(localizedStringForDataRecordType)
                ).sorted()
                Text(record.displayName)
                  .foregroundColor(Color(.bravePrimary))
                if !types.isEmpty {
                  Text(ListFormatter.localizedString(byJoining: types))
                    .foregroundColor(Color(.braveLabel))
                    .font(.footnote)
                }
              }
              .frame(maxWidth: .infinity, alignment: .leading)
              .swipeActions(edge: .trailing) {
                Button(role: .destructive, action: {
                  removeRecords([record])
                }) {
                  Label(Strings.removeDataRecord, systemImage: "trash")
                }
              }
            }
            .onDelete { indexSet in
              let recordsToDelete = indexSet.map { visibleRecords[$0] }
              removeRecords(recordsToDelete)
            }
          }
        }
        .listRowBackground(Color(.braveBackground))
      }
      .listStyle(.plain)
      .listBackgroundColor(Color(UIColor.braveGroupedBackground))
      .environment(\.editMode, $editMode)
      .overlay(
        Group {
          if !isLoading && visibleRecords.isEmpty {
            Text(Strings.noSavedWebsiteData)
              .font(.headline)
              .foregroundColor(Color(.secondaryBraveLabel))
          }
        }
      )
      .toolbar {
        ToolbarItemGroup(placement: .confirmationAction) {
          Button(action: {
            presentationMode.dismiss()
          }) {
            Text(Strings.done)
              .foregroundColor(Color(.braveBlurpleTint))
          }
        }
        ToolbarItemGroup(placement: .bottomBar) {
          Button(action: {
            withAnimation {
              editMode = editMode.isEditing ? .inactive : .active
              if editMode == .inactive {
                selectedRecordsIds = []
              }
            }
          }) {
            Text(editMode.isEditing ? Strings.done : Strings.edit)
              .foregroundColor(visibleRecords.isEmpty ? Color(.braveDisabled) : Color(.braveBlurpleTint))
          }
          .disabled(visibleRecords.isEmpty)
          Spacer()
          Button(action: {
            if isEditMode {
              let idsToRemove = visibleSelectedRecordIds(visibleRecords)
              removeRecordsWithIds(Array(idsToRemove))
              selectedRecordsIds = []
            } else {
              removeRecords(visibleRecords)
            }
          }) {
            Text(removeButtonTitle(visibleRecords: visibleRecords))
              .foregroundColor(visibleRecords.isEmpty ? Color(.braveDisabled) : .red)
              .animation(nil, value: isEditMode)
          }
          .disabled(visibleRecords.isEmpty)
        }
      }
      .searchable(
        text: $filter,
        placement: .navigationBarDrawer(displayMode: .always)
      )
      .navigationTitle(Strings.manageWebsiteDataTitle)
      .navigationBarTitleDisplayMode(.inline)
    }
    .navigationViewStyle(.stack)
    .onAppear(perform: loadRecords)
  }
}

private func localizedStringForDataRecordType(_ type: String) -> String? {
  switch type {
  case WKWebsiteDataTypeCookies:
    return Strings.dataRecordCookies
  case WKWebsiteDataTypeDiskCache, WKWebsiteDataTypeMemoryCache, WKWebsiteDataTypeFetchCache,
    WKWebsiteDataTypeOfflineWebApplicationCache, WKWebsiteDataTypeServiceWorkerRegistrations:
    return Strings.dataRecordCache
  case WKWebsiteDataTypeLocalStorage, WKWebsiteDataTypeSessionStorage:
    return Strings.dataRecordLocalStorage
  case WKWebsiteDataTypeWebSQLDatabases, WKWebsiteDataTypeIndexedDBDatabases:
    return Strings.dataRecordDatabases
  default:
    return nil
  }
}

extension WKWebsiteDataRecord: Identifiable {
  public var id: String {
    displayName
  }
}
