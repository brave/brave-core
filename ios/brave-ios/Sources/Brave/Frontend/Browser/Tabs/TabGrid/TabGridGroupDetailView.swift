// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveStrings
import DesignSystem
import SwiftUI
import Web

/// Shows the tabs of a single group on top of the tab grid
struct TabGroupDetailView: View {
  var viewModel: TabGridViewModel
  var group: TabGroup

  @Environment(\.dismiss) private var dismiss
  @State private var insets: EdgeInsets = .zero
  @State private var title = ""
  @State private var isEditingTitle = false
  @FocusState private var isTitleFocused: Bool

  private var browserColors: BrowserColors {
    viewModel.isPrivateBrowsing ? .privateMode : .standard
  }

  var body: some View {
    TabGridContainerViewRepresentable(
      viewModel: viewModel,
      containerView: nil,
      items: viewModel.openGroupItems,
      isPrivateBrowsing: viewModel.isPrivateBrowsing,
      insets: insets,
      contextMenuForTabs: { _ in nil },
      contextMenuForGroup: { _ in nil },
      scrollsOnModeSwitch: false,
      selectedTabList: .constant([])
    )
    .edgesIgnoringSafeArea(.vertical)
    .background(Color(uiColor: browserColors.tabSwitcherBackground).ignoresSafeArea())
    .overlay(alignment: .top) {
      header
        .padding(.horizontal, 16)
        .padding(.top, 8)
        .padding(.bottom, 12)
        .onGeometryChange(for: CGFloat.self, of: \.size.height) {
          insets.top = $0
        }
        .contentShape(.rect)
    }
    .onAppear {
      title = group.title
      if viewModel.consumeNamingRequest() {
        title = ""
        isEditingTitle = true
      }
    }
    .onChange(of: group.title) {
      if !isTitleFocused {
        title = group.title
      }
    }
    .onChange(of: isTitleFocused) {
      if !isTitleFocused {
        commitTitle()
        isEditingTitle = false
      }
    }
  }

  private func commitTitle() {
    let newTitle = title.trimmingCharacters(in: .whitespacesAndNewlines)
    if newTitle.isEmpty {
      title = group.title
    } else {
      viewModel.renameGroup(group, to: newTitle)
      title = newTitle
    }
  }

  private var optionsMenu: some View {
    Menu {
      Button {
        title = group.title
        isEditingTitle = true
      } label: {
        Label(Strings.TabGrid.editGroupNameMenuTitle, braveSystemImage: "leo.edit.pencil")
      }
      Button {
        viewModel.ungroup(group)
      } label: {
        Label(Strings.TabGrid.ungroupMenuTitle, braveSystemImage: "leo.group.ungroup")
      }
      Button(role: .destructive) {
        // Closing every remaining tab leaves nothing to go back to
        let dismissAfterClose = viewModel.tabCount(in: group) == viewModel.tabs.count
        viewModel.closeTabs(in: group)
        if dismissAfterClose {
          dismiss()
        }
      } label: {
        Label(Strings.TabGrid.closeGroupTabsMenuTitle, braveSystemImage: "leo.close")
      }
    } label: {
      Label(Strings.TabGrid.groupOptionsAccessibilityLabel, braveSystemImage: "leo.more.vertical")
        .labelStyle(.buttonIconOnly)
    }
    .plainHeaderIconButtonStyle()
    .menuOrder(.fixed)
  }

  private var header: some View {
    VStack(alignment: .leading, spacing: 12) {
      HStack(spacing: 12) {
        Button {
          commitTitle()
          withAnimation {
            viewModel.closeOpenGroup()
          }
        } label: {
          Label(Strings.TabGrid.backToTabsAccessibilityLabel, braveSystemImage: "leo.browser.back")
            .labelStyle(.buttonIconOnly)
        }
        .plainHeaderIconButtonStyle()
        Spacer()
        Button {
          viewModel.addTab(in: group)
          dismiss()
        } label: {
          Label(Strings.TabGrid.newTabAccessibilityLabel, braveSystemImage: "leo.plus.add")
            .labelStyle(.buttonIconOnly)
        }
        .plainHeaderIconButtonStyle()
        optionsMenu
      }
      .foregroundStyle(Color(braveSystemName: .textSecondary))
      Group {
        if isEditingTitle {
          TextField(Strings.TabGrid.groupNamePlaceholder, text: $title)
            .focused($isTitleFocused)
            .submitLabel(.done)
            .onSubmit {
              isTitleFocused = false
            }
            .onAppear {
              DispatchQueue.main.async {
                isTitleFocused = true
              }
            }
        } else {
          Text(group.title)
            .lineLimit(1)
            .accessibilityAddTraits(.isHeader)
        }
      }
      .font(.title3.weight(.semibold))
      .foregroundStyle(Color(braveSystemName: .textPrimary))
      .padding(.vertical, 4)
    }
    .containerCornerOffset(.leading, sizeToFit: true)
    .dynamicTypeSize(.xSmall..<DynamicTypeSize.accessibility2)
  }
}
