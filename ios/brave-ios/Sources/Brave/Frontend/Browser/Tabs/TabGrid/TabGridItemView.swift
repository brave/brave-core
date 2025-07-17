// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveStrings
import Favicon
import Foundation
import SwiftUI
import Web

@Observable
class TabGridItemViewModel: TabObserver {
  private(set) var favicon: Favicon?
  private(set) var title: String?
  private(set) var snapshot: UIImage?
  private(set) var isVisible: Bool = false
  var isPrivateBrowsing: Bool = false
  var swipeToDeleteGestureState: GestureState?

  struct GestureState {
    var initialCenter: CGPoint = .zero
  }

  @ObservationIgnored
  private weak var tab: (any TabState)?

  deinit {
    tab?.removeObserver(self)
  }

  func configure(with tab: any TabState) {
    if let tab = self.tab {
      tab.removeObserver(self)
    }
    self.tab = tab

    tab.addObserver(self)
    tab.onScreenshotUpdated = { [weak self, weak tab] in
      guard let self, let tab else { return }
      Task { @MainActor in
        self.snapshot = tab.screenshot
      }
    }
    favicon = tab.displayFavicon ?? Favicon.default
    title = tab.displayTitle
    snapshot = tab.screenshot
    isVisible = tab.isVisible
  }

  func reset() {
    tab?.removeObserver(self)
    tab?.onScreenshotUpdated = nil
    title = nil
    favicon = nil
    snapshot = nil
    isVisible = false
    swipeToDeleteGestureState = nil
  }

  func tabDidChangeTitle(_ tab: some TabState) {
    title = tab.displayTitle
  }

  func tabWasShown(_ tab: some TabState) {
    isVisible = true
  }

  func tabWasHidden(_ tab: some TabState) {
    isVisible = false
  }

  func tabWillBeDestroyed(_ tab: some TabState) {
    // Don't reset the actual model data here since we want it to animate correctly when the
    // tab is removed
    tab.removeObserver(self)
  }
}

struct TabGridItemView: View {
  enum Action {
    case closedTab
  }

  var viewModel: TabGridItemViewModel
  var actionHandler: (Action) -> Void

  var browserColors: BrowserColors {
    viewModel.isPrivateBrowsing ? .privateMode : .standard
  }

  var body: some View {
    VStack(spacing: 0) {
      HStack {
        Color.clear
          .frame(width: 16, height: 16)
          .overlay {
            if let favicon = viewModel.favicon?.image {
              Image(uiImage: favicon)
                .resizable()
                .aspectRatio(contentMode: .fit)
                .frame(width: 16, height: 16)
            } else {
              Circle()
            }
          }
        if let title = viewModel.title {
          Text(title)
            .foregroundStyle(Color(braveSystemName: .textPrimary))
            .lineLimit(1)
        }
        Spacer()
        Button {
          actionHandler(.closedTab)
        } label: {
          Label(Strings.TabGrid.closeTabAccessibilityLabel, braveSystemImage: "leo.close")
            .labelStyle(.iconOnly)
        }
        .imageScale(.large)
        .foregroundStyle(Color(braveSystemName: .iconDefault))
      }
      .padding(8)
      .font(.caption2.weight(.semibold))
      Color(braveSystemName: .containerDisabled)
        .overlay(alignment: .top) {
          if let screenshot = viewModel.snapshot {
            Image(uiImage: screenshot)
              .resizable()
              .aspectRatio(contentMode: .fill)
              .allowsHitTesting(false)
          }
        }
        .clipShape(
          .rect(
            topLeadingRadius: 4,
            bottomLeadingRadius: 12,
            bottomTrailingRadius: 12,
            topTrailingRadius: 4,
            style: .continuous
          )
        )
    }
    .padding(4)
    .overlay {
      if viewModel.isVisible {
        ContainerRelativeShape()
          .strokeBorder(Color(uiColor: browserColors.tabSwitcherSelectedCellBorder), lineWidth: 1.5)
      }
    }
    .animation(.default) { content in
      content
        .background(
          Color(uiColor: browserColors.tabSwitcherCellBackground)
            .shadow(
              .drop(
                color: viewModel.swipeToDeleteGestureState != nil
                  ? Color.black.opacity(0.25) : Color(braveSystemName: .elevationSecondary),
                radius: viewModel.swipeToDeleteGestureState != nil ? 15 : 4,
                x: 0,
                y: 1
              )
            )
            .shadow(.drop(color: Color(braveSystemName: .elevationPrimary), radius: 0, x: 0, y: 1)),
          in: .containerRelative
        )
    }
    .containerShape(.rect(cornerRadius: 16, style: .continuous))
    .dynamicTypeSize(.xSmall..<DynamicTypeSize.accessibility1)
  }
}
