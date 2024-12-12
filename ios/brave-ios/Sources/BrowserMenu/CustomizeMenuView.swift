// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveUI
import DesignSystem
import Strings
import SwiftUI

struct CustomizeMenuView: View {
  @ObservedObject var model: BrowserMenuModel

  @Environment(\.dismiss) private var dismiss

  var body: some View {
    NavigationStack {
      List {
        Section {
          ForEach(model.visibleActions) { action in
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
          }
          .onMove { indexSet, offset in
            guard let index = indexSet.first, let action = model.visibleActions[safe: index] else {
              return
            }
            // SwiftUI's move offset logic is a bit strange where the destination can be the
            // count instead of the final valid index, so its safer to use SwiftUI's Collection
            // API to handle it then get the valid destination index of that mutated array.
            var ids = model.visibleActions.map(\.id)
            ids.move(fromOffsets: indexSet, toOffset: offset)
            guard let destination = ids.firstIndex(of: action.id) else {
              return
            }
            model.reorderVisibleAction(action, to: destination)
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
          }
          .listRowBackground(Color(.secondaryBraveGroupedBackground))
        } header: {
          Text(Strings.BrowserMenu.hiddenActionsTitle)
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
    }
  }
}

#Preview {
  CustomizeMenuView(model: .mock)
}
