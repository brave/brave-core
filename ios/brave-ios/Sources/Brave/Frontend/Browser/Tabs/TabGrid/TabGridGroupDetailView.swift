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
  var didAddTab: () -> Void

  @Environment(\.dismiss) private var dismiss
  @State private var insets: EdgeInsets = .zero
  @State private var title = ""
  @State private var isEditingTitle = false
  @FocusState private var isTitleFocused: Bool
  @State private var isExpanded = false
  @State private var expandedFrame: CGRect?

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
      isReorderingEnabled: false,
      isGroupDetail: true,
      selectedTabList: .constant([])
    )
    // The tab grid's edit mode doesn't apply to a group's tabs
    .environment(\.editMode, .constant(.inactive))
    .background {
      Color(uiColor: browserColors.tabSwitcherBackground)
        .ignoresSafeArea()
        .clipShape(.rect(cornerRadius: isExpanded ? 0 : 16, style: .continuous))
        .onGeometryChange(for: CGRect.self) {
          $0.frame(in: .global)
        } action: { frame in
          guard expandedFrame == nil, !frame.isEmpty else { return }
          expandedFrame = frame
          guard !isExpanded else { return }
          DispatchQueue.main.async {
            withAnimation(.growAnimation) {
              isExpanded = true
            }
          }
        }
    }
    .edgesIgnoringSafeArea(.vertical)
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
    .scaleEffect(isExpanded ? CGSize(width: 1, height: 1) : collapsedScale)
    .offset(isExpanded ? .zero : collapsedOffset)
    .opacity(isExpanded ? 1 : 0)
    .allowsHitTesting(isExpanded)
    .onAppear {
      // A restored group is shown right away, without growing out of its tile
      isExpanded = viewModel.isOpenGroupRestored
      title = viewModel.openGroupTitle
      if viewModel.consumeNamingRequest() {
        title = ""
        isEditingTitle = true
      }
    }
    .onChange(of: viewModel.openGroupTitle) {
      if !isTitleFocused {
        title = viewModel.openGroupTitle
      }
    }
    .onChange(of: isTitleFocused) {
      if !isTitleFocused {
        commitTitle()
        isEditingTitle = false
      }
    }
  }

  /// The scale that shrinks the view down to the size of its group's item
  private var collapsedScale: CGSize {
    guard let source = viewModel.openGroupSourceFrame, let expandedFrame else {
      return CGSize(width: 0.9, height: 0.9)
    }
    return CGSize(
      width: source.width / expandedFrame.width,
      height: source.height / expandedFrame.height
    )
  }

  /// The offset that moves the shrunk view over its group's item
  private var collapsedOffset: CGSize {
    guard let source = viewModel.openGroupSourceFrame, let expandedFrame else { return .zero }
    return CGSize(
      width: source.midX - expandedFrame.midX,
      height: source.midY - expandedFrame.midY
    )
  }

  /// Shrinks the view back into its group's item, then closes it
  private func collapse() {
    commitTitle()
    withAnimation(.growAnimation) {
      isExpanded = false
    } completion: {
      viewModel.closeOpenGroup()
    }
  }

  private func commitTitle() {
    let newTitle = title.trimmingCharacters(in: .whitespacesAndNewlines)
    if newTitle.isEmpty {
      title = viewModel.openGroupTitle
    } else {
      viewModel.renameGroup(group, to: newTitle)
      title = newTitle
    }
  }

  private var optionsMenu: some View {
    Menu {
      Button {
        title = viewModel.openGroupTitle
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
        let dismissAfterClose = viewModel.closesAllTabs(in: group)
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
          collapse()
        } label: {
          Label(Strings.TabGrid.backToTabsAccessibilityLabel, braveSystemImage: "leo.close")
            .labelStyle(.buttonIconOnly)
        }
        .plainHeaderIconButtonStyle()
        Spacer()
        Button {
          viewModel.addTab(in: group)
          DispatchQueue.main.async {
            dismiss()
            didAddTab()
          }
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
          Text(viewModel.openGroupTitle)
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

extension Animation {
  /// Matches the spring of the tab tray's transition to and from a tab
  fileprivate static var growAnimation: Animation {
    .spring(duration: 0.4, bounce: 0)
  }
}
