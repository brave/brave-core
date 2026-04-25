// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveUI
import DesignSystem
import Preferences
import Strings
import SwiftUI

struct CustomizeMenuView: View {
  @ObservedObject var model: BrowserMenuModel

  @Environment(\.dismiss) private var dismiss

  @ObservedObject private var numberOfQuickActions = Preferences.BrowserMenu.numberOfQuickActions
  @State private var isResetDialogPresented = false

  private enum VisibleRow: Identifiable {
    case action(Action)
    case quickActionCountDivider

    var id: String {
      switch self {
      case .action(let action): return action.id.id
      case .quickActionCountDivider: return "count-divider"
      }
    }
  }

  private var visibleRows: [VisibleRow] {
    var rows = model.visibleActions.map(VisibleRow.action)
    if rows.count > numberOfQuickActions.value {
      rows.insert(.quickActionCountDivider, at: numberOfQuickActions.value)
    } else {
      rows.append(.quickActionCountDivider)
    }
    return rows
  }

  private func moveRow(from source: IndexSet, to destinationOffset: Int) {
    // Let SwiftUI do the heavy lifting for actually reordering
    var newVisibleRows = visibleRows
    newVisibleRows.move(fromOffsets: source, toOffset: destinationOffset)

    // Update the number of quick actions based on the new location of the divider
    let previousNumberOfQuickActions = numberOfQuickActions.value
    if let quickActionCountDividerIndex = newVisibleRows.firstIndex(where: { element in
      if case .quickActionCountDivider = element {
        return true
      }
      return false
    }) {
      numberOfQuickActions.value = quickActionCountDividerIndex
      // Remove the divider from the data source so that we can calculate a destination index
      // without it
      newVisibleRows.remove(at: quickActionCountDividerIndex)
    }

    guard let sourceIndex = source.first,
      // Adjust the source index if it was previously passed the divider
      case let adjustedSourceIndex = sourceIndex > previousNumberOfQuickActions
        ? sourceIndex - 1 : sourceIndex,
      let action = model.visibleActions[safe: adjustedSourceIndex],
      let destinationIndex = newVisibleRows.firstIndex(where: {
        $0.id == action.id.id
      }),
      adjustedSourceIndex != destinationIndex
    else { return }

    model.reorderVisibleAction(action, to: destinationIndex)
  }

  var body: some View {
    NavigationStack {
      List {
        Section {
          ForEach(visibleRows) { visibleRow in
            switch visibleRow {
            case .action(let action):
              let id = action.id
              HStack {
                Button {
                  withAnimation {
                    model.updateActionVisibility(action, visibility: .hidden)
                  }
                } label: {
                  Image(systemName: "minus.circle.fill")
                    .imageScale(.large)
                }
                .buttonStyle(.plain)
                .foregroundStyle(.red)
                Label {
                  Text(id.defaultTitle)
                } icon: {
                  Image(braveSystemName: id.defaultImage)
                    .foregroundStyle(Color(braveSystemName: .iconDefault))
                }
              }
              .id("Visible-\(id.id)")
            case .quickActionCountDivider:
              Text(Strings.BrowserMenu.quickActionDividerTitle)
                .font(.caption)
                .textCase(.uppercase)
                .foregroundStyle(Color(braveSystemName: .textTertiary))
            }
          }
          .onMove { source, destination in
            moveRow(from: source, to: destination)
          }
          .listRowBackground(Color(.secondaryBraveGroupedBackground))
        } header: {
          Text(Strings.BrowserMenu.visibleActionsTitle)
        }

        Section {
          ForEach(model.hiddenActions) { action in
            let id = action.id
            HStack {
              Button {
                withAnimation {
                  model.updateActionVisibility(action, visibility: .visible)
                }
              } label: {
                Image(systemName: "plus.circle.fill")
                  .imageScale(.large)
              }
              .foregroundStyle(.green)
              Label {
                Text(id.defaultTitle)
              } icon: {
                Image(braveSystemName: id.defaultImage)
                  .foregroundStyle(Color(braveSystemName: .iconDefault))
              }
            }
            .id("Hidden-\(id.id)")
          }
          .listRowBackground(Color(.secondaryBraveGroupedBackground))
        } header: {
          Text(Strings.BrowserMenu.hiddenActionsTitle)
        }

        Section {
          Button(role: .destructive) {
            isResetDialogPresented = true
          } label: {
            Text(Strings.BrowserMenu.resetToDefault)
              .frame(maxWidth: .infinity)
          }
          .listRowBackground(Color(.secondaryBraveGroupedBackground))
        }
      }
      .scrollContentBackground(.hidden)
      .background(Color(.braveGroupedBackground))
      .environment(\.editMode, .constant(EditMode.active))
      .toolbar {
        ToolbarItemGroup(placement: .confirmationAction) {
          Button {
            dismiss()
          } label: {
            Text(Strings.BrowserMenu.doneButtonTitle)
          }
        }
      }
      .navigationBarTitleDisplayMode(.inline)
      .navigationTitle(Strings.BrowserMenu.customizeTitle)
      .confirmationDialog(
        Strings.BrowserMenu.resetToDefault,
        isPresented: $isResetDialogPresented,
        titleVisibility: .visible
      ) {
        Button(role: .destructive) {
          withAnimation {
            numberOfQuickActions.reset()
            model.resetToDefault()
          }
        } label: {
          Text(Strings.BrowserMenu.resetButtonTitle)
        }
      } message: {
        Text(Strings.BrowserMenu.resetToDefaultDialogMessage)
      }
    }
  }
}

#Preview {
  CustomizeMenuView(model: .mock)
}
