// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveStrings
import BraveUI
import Favicon
import Foundation
import Shared
import SwiftUI
import Web

@MainActor
@Observable
class TabGridItemViewModel: TabObserver {
  private(set) var favicon: Favicon?
  private(set) var title: String?
  private(set) var snapshot: UIImage?
  private(set) var isVisible: Bool = false
  var isSelectedInEditMode: Bool = false
  var isPrivateBrowsing: Bool = false
  var swipeToDeleteGestureState: GestureState?

  struct GestureState {
    var initialCenter: CGPoint = .zero
  }

  @ObservationIgnored
  private weak var tab: (any TabState)?

  isolated deinit {
    tab?.removeObserver(self)
  }

  func configure(with tab: any TabState) {
    if let tab = self.tab {
      tab.removeObserver(self)
      (tab as? TorTabState)?.onVisualsUpdated = nil
    }
    self.tab = tab

    tab.addObserver(self)
    tab.onScreenshotUpdated = { [weak self, weak tab] in
      guard let self, let tab else { return }
      Task { @MainActor in
        self.snapshot = tab.screenshot
      }
    }
    favicon = tab.faviconTabHelper?.displayFavicon ?? Favicon.default
    title = tab.displayTitle
    snapshot = tab.screenshot
    isVisible = tab.isVisible
    if let tor = tab as? TorTabState {
      updateTorVisuals(tor)
      tor.onVisualsUpdated = { [weak self, weak tor] in
        guard let self, let tor, self.tab === tor else { return }
        updateTorVisuals(tor)
      }
      if tor.visibleURL?.isNewTabURL == true {
        _ = tor.nativeNewTabPageViewController?.captureTorTabGridPreview()
        updateTorVisuals(tor)
      } else {
        tor.refreshPreview()
      }
    }
  }

  private func updateTorVisuals(_ tab: TorTabState) {
    if tab.visibleURL?.isNewTabURL == true {
      snapshot = tab.nativeNewTabPageViewController?.torTabGridPreview
      favicon = Favicon(
        image: UIImage(sharedNamed: "brave.logo"), isMonogramImage: false, backgroundColor: .clear
      )
      return
    }
    snapshot = tab.previewImage
    favicon = tab.faviconImage.map {
      Favicon(image: $0, isMonogramImage: false, backgroundColor: .clear)
    } ?? .default
  }

  func reset() {
    (tab as? TorTabState)?.onVisualsUpdated = nil
    tab?.removeObserver(self)
    tab?.onScreenshotUpdated = nil
    title = nil
    favicon = nil
    snapshot = nil
    isVisible = false
    swipeToDeleteGestureState = nil
    isSelectedInEditMode = false
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
  @Environment(\.editMode) private var editMode
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
        Group {
          if editMode?.wrappedValue == .active {
            let isSelected = viewModel.isSelectedInEditMode
            Image(braveSystemName: isSelected ? "leo.checkbox.checked" : "leo.checkbox.unchecked")
              .foregroundStyle(Color(braveSystemName: isSelected ? .iconInteractive : .iconDefault))
          } else {
            Button {
              actionHandler(.closedTab)
            } label: {
              Label(Strings.TabGrid.closeTab, braveSystemImage: "leo.close")
                .labelStyle(.iconOnly)
            }
          }
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
      let borderShape = ContainerRelativeShape().strokeBorder(
        Color(uiColor: browserColors.tabSwitcherSelectedCellBorder),
        lineWidth: 1.5
      )
      if editMode?.wrappedValue == .active {
        if viewModel.isSelectedInEditMode {
          borderShape
        }
      } else if viewModel.isVisible {
        borderShape
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
