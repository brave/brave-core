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

  @State private var insets: EdgeInsets = .zero

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
    .frame(maxWidth: .infinity, maxHeight: .infinity)
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
  }

  private var header: some View {
    HStack(spacing: 12) {
      Button {
        withAnimation {
          viewModel.closeOpenGroup()
        }
      } label: {
        Label(Strings.TabGrid.backToTabsAccessibilityLabel, braveSystemImage: "leo.browser.back")
          .labelStyle(.iconOnly)
      }
      .plainHeaderIconButtonStyle()
      Text(group.title)
        .font(.title3.weight(.semibold))
        .foregroundStyle(Color(braveSystemName: .textPrimary))
        .lineLimit(1)
        .accessibilityAddTraits(.isHeader)
      Spacer()
    }
    .foregroundStyle(Color(braveSystemName: .textSecondary))
    .dynamicTypeSize(.xSmall..<DynamicTypeSize.accessibility2)
  }
}
