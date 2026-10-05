// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveStrings
import DesignSystem
import Shared
import SwiftUI
import UIKit

/// The action buttons of the split layout
struct SplitBottomToolbarView: View {
  var toolbarState: BrowserToolbarState
  @ObservedObject var privateBrowsingManager: PrivateBrowsingManager

  init(toolbarState: BrowserToolbarState, privateBrowsingManager: PrivateBrowsingManager) {
    self.toolbarState = toolbarState
    self.privateBrowsingManager = privateBrowsingManager
  }

  @ScaledMetric(relativeTo: .body) private var iconSize = 20

  private var browserColors: any BrowserColors {
    privateBrowsingManager.browserColors
  }

  var body: some View {
    HStack(spacing: 0) {
      SplitToolbarButton(
        icon: "leo.browser.back",
        label: Strings.tabToolbarBackButtonAccessibilityLabel,
        identifier: "TabToolbar.backButton",
        isEnabled: toolbarState.canGoBack,
        browserColors: browserColors,
        iconSize: iconSize
      ) {
        // TODO: Forward to toolbar commands
      }
      // The forward button replaces the share button when available
      if toolbarState.canGoForward {
        SplitToolbarButton(
          icon: "leo.browser.forward",
          label: Strings.tabToolbarForwardButtonAccessibilityLabel,
          identifier: "TabToolbar.forwardButton",
          isEnabled: true,
          browserColors: browserColors,
          iconSize: iconSize
        ) {
          // TODO: Forward to toolbar commands
        }
      } else {
        SplitToolbarButton(
          icon: "leo.share.macos",
          label: Strings.tabToolbarShareButtonAccessibilityLabel,
          identifier: "TabToolbar.shareButton",
          isEnabled: toolbarState.isWebPage,
          browserColors: browserColors,
          iconSize: iconSize
        ) {
          // TODO: Forward to toolbar commands
        }
      }
      // The search button replaces the add tab button on the new tab page
      if toolbarState.isNewTabPage {
        SplitToolbarButton(
          icon: "leo.search",
          label: Strings.tabToolbarSearchAddressPlaceholderText,
          identifier: "TabToolbar.searchButton",
          isEnabled: true,
          browserColors: browserColors,
          iconSize: iconSize
        ) {
          // TODO: Forward to toolbar commands
        }
      } else {
        SplitToolbarButton(
          icon: "leo.plus.add",
          label: Strings.tabToolbarAddTabButtonAccessibilityLabel,
          identifier: "TabToolbar.addTabButton",
          isEnabled: true,
          browserColors: browserColors,
          iconSize: iconSize
        ) {
          // TODO: Forward to toolbar commands
        }
      }
      SplitTabsButton(count: toolbarState.tabCount, browserColors: browserColors) {
        // TODO: Forward to toolbar commands
      }
      .frame(maxWidth: .infinity, maxHeight: .infinity)
      .accessibilityIdentifier("TabToolbar.tabsButton")
      SplitToolbarButton(
        icon: "leo.more.horizontal",
        label: Strings.tabToolbarMenuButtonAccessibilityLabel,
        identifier: nil,
        isEnabled: true,
        browserColors: browserColors,
        iconSize: iconSize
      ) {
        // TODO: Forward to toolbar commands
      }
    }
    .dynamicTypeSize(...DynamicTypeSize.xxLarge)
    .frame(maxWidth: .infinity)
    .frame(height: UIConstants.toolbarHeight)
    .accessibilityElement(children: .contain)
    .accessibilityLabel(Strings.tabToolbarAccessibilityLabel)
  }
}

private struct SplitToolbarButton: View {
  var icon: String
  var label: String
  var identifier: String?
  var isEnabled: Bool
  var browserColors: any BrowserColors
  var iconSize: CGFloat
  var action: () -> Void

  var body: some View {
    Button(action: action) {
      Image(braveSystemName: icon)
        .renderingMode(.template)
        .resizable()
        .aspectRatio(contentMode: .fit)
        .frame(width: iconSize, height: iconSize)
        .frame(maxWidth: .infinity, maxHeight: .infinity)
        .contentShape(Rectangle())
    }
    .buttonStyle(SplitToolbarButtonStyle(isEnabled: isEnabled, browserColors: browserColors))
    .disabled(!isEnabled)
    .accessibilityLabel(label)
    .accessibilityIdentifier(identifier ?? "")
  }
}

private struct SplitToolbarButtonStyle: ButtonStyle {
  var isEnabled: Bool
  var browserColors: any BrowserColors

  func makeBody(configuration: Configuration) -> some View {
    configuration.label
      .foregroundStyle(
        Color(
          uiColor: !isEnabled
            ? browserColors.iconDisabled
            : (configuration.isPressed ? browserColors.iconActive : browserColors.iconDefault)
        )
      )
  }
}

/// The tabs button, which displays the number of open tabs
private struct SplitTabsButton: UIViewRepresentable {
  var count: Int
  var browserColors: any BrowserColors
  var action: () -> Void

  final class Coordinator {
    var action: () -> Void = {}
  }

  func makeCoordinator() -> Coordinator {
    Coordinator()
  }

  func makeUIView(context: Context) -> TabsButton {
    let button = TabsButton()
    let coordinator = context.coordinator
    button.addAction(UIAction { _ in coordinator.action() }, for: .touchUpInside)
    return button
  }

  func updateUIView(_ uiView: TabsButton, context: Context) {
    context.coordinator.action = action
    uiView.browserColors = browserColors
    uiView.updateTabCount(count)
  }
}
