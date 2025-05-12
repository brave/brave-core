// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShared
import BraveStrings
import BraveUI
import Foundation
import LocalAuthentication
import Lottie
import Preferences
import SnapKit
import Strings
import SwiftUI
import UIKit
import Web

class TabGridHostingController: UIHostingController<TabGridView> {
  let tabManager: TabManager
  let containerView: TabGridContainerView

  init(
    tabManager: TabManager,
    historyModel: HistoryModel?,
    openTabsModel: OpenTabsModel?,
    toolbarUrlActionsDelegate: ToolbarUrlActionsDelegate?,
    profileController: BraveProfileController?,
    windowProtection: WindowProtection?
  ) {
    self.tabManager = tabManager  // Basically only held for TabTrayTransition
    let viewModel = TabGridViewModel(
      tabManager: tabManager,
      historyModel: historyModel,
      openTabsModel: openTabsModel,
      toolbarUrlActionsDelegate: toolbarUrlActionsDelegate,
      profileController: profileController,
      windowProtection: windowProtection
    )
    containerView = .init(isPrivateBrowsing: viewModel.isPrivateBrowsing)
    super.init(rootView: TabGridView(viewModel: viewModel, containerView: containerView))
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
}

struct TabGridView: View {
  @Bindable var viewModel: TabGridViewModel
  var containerView: TabGridContainerView

  @Environment(\.dismiss) private var dismiss
  @Environment(\.colorScheme) private var colorScheme
  @Environment(\.horizontalSizeClass) private var horizontalSizeClass

  private enum DestinationSheet: Int, Identifiable, Hashable {
    case syncedTabs
    case history

    var id: Int {
      rawValue
    }
  }

  @State private var insets: EdgeInsets = .zero
  @State private var destinationSheet: DestinationSheet?
  @State private var isSyncSettingsPresented: Bool = false
  @State private var isShredAlertPresented: Bool = false
  @State private var isShredAnimationVisible: Bool = false

  var browserColors: BrowserColors {
    viewModel.isPrivateBrowsing ? .privateMode : .standard
  }

  private var syncSettingsController: UIViewController {
    guard let profileController = viewModel.profileController else { return .init() }
    let controller: UIViewController
    if Preferences.Chromium.syncEnabled.value {
      controller = SyncSettingsTableViewController(
        isModallyPresented: true,
        braveCoreMain: profileController,
        windowProtection: viewModel.windowProtection
      )
    } else {
      controller = SyncWelcomeViewController(
        braveCore: profileController,
        windowProtection: viewModel.windowProtection,
        isModallyPresented: true
      )
    }
    return UINavigationController(rootViewController: controller)
  }

  var body: some View {
    VStack {
      if viewModel.isPrivateBrowsing, viewModel.tabs.isEmpty, !viewModel.isSearching {
        if !viewModel.isPrivateTabsLocked {
          GeometryReader { proxy in
            ScrollView {
              PrivateModeInfoView()
                .padding(40)
                .frame(maxWidth: .infinity, minHeight: proxy.size.height)
            }
            .scrollBounceBehavior(.basedOnSize)
          }
          .safeAreaPadding(insets)
          .transition(.opacity)
        }
      } else {
        TabGridContainerViewRepresentable(
          viewModel: viewModel,
          containerView: containerView,
          tabs: viewModel.tabs,
          isPrivateBrowsing: viewModel.isPrivateBrowsing,
          insets: insets
        )
      }
    }
    .frame(maxWidth: .infinity, maxHeight: .infinity)
    .mask {
      // TODO: Test Performance of this mask w/ and w/o drawingGroup
      let radius = viewModel.isSearching ? 5 : 10.0
      Color.black
        .blur(radius: radius)
        .padding(.horizontal, -(radius * 2))
        .padding(insets)
        .padding(.top, viewModel.isSearching ? 44 + 8 : 0)
    }
    .background(alignment: .top) {
      if viewModel.isPrivateBrowsing, horizontalSizeClass == .compact {
        // Decorative blur
        Color(braveSystemName: .primitivePrivateWindow20)
          .aspectRatio(1.5, contentMode: .fit)
          .ignoresSafeArea()
          .blur(radius: 100)
      }
    }
    .background(Color(uiColor: browserColors.tabSwitcherBackground))
    .overlay(alignment: .top) {
      VStack {
        headerBar
          .onGeometryChange(for: CGFloat.self, of: \.size.height) {
            insets.top = $0
          }
          .animation(
            .default,
            body: { content in
              content
                .opacity(viewModel.isSearching ? 0.5 : 1)
                .disabled(viewModel.isSearching)
            }
          )
          .contentShape(.rect)  // Dont trigger background tap from tapping empty area
        let isSearchBarHidden =
          viewModel.isPrivateBrowsing && viewModel.tabs.isEmpty && !viewModel.isSearching
        if !isSearchBarHidden {
          TabGridSearchBar(
            text: $viewModel.searchQuery,
            isFocused: $viewModel.isSearching,
            scrollView: containerView.collectionView
          )
          .padding(.horizontal, 16)
        }
      }
    }
    .overlay(alignment: .bottom) {
      if !viewModel.isSearching {
        footerBar
          .onGeometryChange(for: CGFloat.self, of: \.size.height) {
            insets.bottom = $0
          }
          .contentShape(.rect)  // Dont trigger background tap from tapping empty area
      }
    }
    .onChange(of: viewModel.isSearching) { oldValue, newValue in
      // Reset the bottom inset since we hide the footer when searching
      if newValue {
        insets.bottom = 0
      }
    }
    .background {
      orphanKeyboardShortcuts
        .disabled(viewModel.isSearching)
    }
    .sheet(item: $destinationSheet) { item in
      switch item {
      case .syncedTabs:
        if let openTabsModel = viewModel.openTabsModel {
          SyncedTabsView(
            openTabs: openTabsModel,
            urlActionHandler: viewModel.toolbarUrlActionsDelegate
          ) {
            isSyncSettingsPresented = true
          }
          .sheet(isPresented: $isSyncSettingsPresented) {
            UIKitController(syncSettingsController)
          }
        }
      case .history:
        if let historyModel = viewModel.historyModel {
          HistoryView(model: historyModel)
        }
      }
    }
    .alert(
      Strings.Shields.shredSiteDataConfirmationTitle,
      isPresented: $isShredAlertPresented,
      actions: {
        Button(role: .destructive) {
          isShredAnimationVisible = true
        } label: {
          Text(Strings.Shields.shredDataButtonTitle)
        }
        .keyboardShortcut(.defaultAction)
      },
      message: {
        Text(Strings.Shields.shredSiteDataConfirmationMessage)
      }
    )
    .overlay {
      if isShredAnimationVisible {
        LottieView(animation: .named("shred", bundle: .module))
          .configure { view in
            // resizable modifier below doesn't actually adjust the fill mode so we need to
            // configure this separately
            view.contentMode = .scaleAspectFill
          }
          .resizable()
          .playing(loopMode: .playOnce)
          .animationDidFinish { _ in
            let dismissAfterShred = viewModel.tabs.count == 1
            viewModel.shredSelectedTab()
            withAnimation {
              isShredAnimationVisible = false
            }
            if dismissAfterShred {
              dismiss()
            }
          }
          .frame(maxWidth: .infinity, maxHeight: .infinity)
          .ignoresSafeArea()
          .transition(.opacity)
      }
    }
    .colorScheme(viewModel.isPrivateBrowsing ? .dark : colorScheme)
    .preferredColorScheme(viewModel.isPrivateBrowsing ? .dark : colorScheme)
  }

  /// Keyboard shortcuts that handle actions not associated with a particular user-facing button
  var orphanKeyboardShortcuts: some View {
    ZStack {
      Button {
        viewModel.closeSelectedTab()
        dismiss()
      } label: {
        Text(Strings.Hotkey.closeTabFromTabTrayKeyCodeTitle)
      }
      .keyboardShortcut("w", modifiers: .command)

      Button {
        viewModel.closeAllTabs()
        dismiss()
      } label: {
        Text(Strings.Hotkey.closeAllTabsFromTabTrayKeyCodeTitle)
      }
      .keyboardShortcut("w", modifiers: [.command, .shift])

      Button {
        viewModel.isPrivateBrowsing.toggle()
      } label: {
        Text(
          viewModel.isPrivateBrowsing
            ? Strings.Hotkey.switchToNonPBMKeyCodeTitle : Strings.Hotkey.switchToPBMKeyCodeTitle
        )
      }
      .keyboardShortcut("`", modifiers: .command)

      Button {
        viewModel.addAndSelectRecentlyClosedTab()
        dismiss()
      } label: {
        Text(Strings.Hotkey.recentlyClosedTabTitle)
      }
      .disabled(!viewModel.hasRecentlyClosedTabs)
      .keyboardShortcut("t", modifiers: [.command, .shift])

      Button {
        withAnimation {
          viewModel.isSearching = true
        }
      } label: {
        Text(Strings.tabTraySearchBarTitle)
      }
      .keyboardShortcut("f", modifiers: [.command])
    }
    .hidden()
    .accessibilityHidden(true)
  }

  var headerBar: some View {
    HStack {
      PrivateBrowsingPicker(isPrivateBrowsing: $viewModel.isPrivateBrowsing)
      Spacer()
      HStack(spacing: 20) {
        Button {
          isShredAlertPresented = true
        } label: {
          Label(Strings.TabGrid.shredTabsAccessibilityLabel, braveSystemImage: "leo.shred.data")
        }
        .disabled(viewModel.tabs.isEmpty)
        Button {
          destinationSheet = .history
        } label: {
          Label(Strings.TabGrid.viewHistoryAccessibilityLabel, braveSystemImage: "leo.history")
        }
        Button {
          destinationSheet = .syncedTabs
        } label: {
          Label(
            Strings.TabGrid.viewSyncedTabsAccessibilityLabel,
            braveSystemImage: "leo.smartphone.laptop"
          )
        }
      }
      .font(.callout)
      .imageScale(.large)
      .labelStyle(.iconOnly)
    }
    .foregroundStyle(Color(braveSystemName: .textSecondary))
    .padding(.horizontal, 16)
    .padding(.vertical, 8)
    .dynamicTypeSize(.xSmall..<DynamicTypeSize.accessibility2)
  }

  var footerBar: some View {
    HStack {
      Menu {
        Button(role: .destructive) {
          viewModel.closeAllTabs()
        } label: {
          Label(Strings.TabGrid.closeAllTabsButtonTitle, braveSystemImage: "leo.close")
        }
      } label: {
        Text(Strings.TabGrid.moreMenuButtonTitle)
          .padding(4)
      }
      Spacer()
      Button {
        viewModel.addTab()
        // Let the tab show up in the collection view first
        DispatchQueue.main.async { [self] in
          dismiss()
        }
      } label: {
        Label(Strings.TabGrid.newTabAccessibilityLabel, braveSystemImage: "leo.plus.add")
          .labelStyle(.iconOnly)
          .font(.callout)
          .imageScale(.large)
          .foregroundStyle(Color(braveSystemName: .iconDefault))
          .padding(.horizontal, 22)
          .padding(.vertical, 8)
          .background(
            Color(uiColor: browserColors.tabSwitcherForeground),
            in: .rect(cornerRadius: 12, style: .continuous)
          )
      }
      .keyboardShortcut("t", modifiers: [.command])
      .buttonStyle(.plain)
      Spacer()
      Button {
        if viewModel.tabs.isEmpty {
          viewModel.addTab()
        }
        dismiss()
      } label: {
        Text(Strings.done)
          .padding(4)
      }
      .keyboardShortcut(.defaultAction)
    }
    .foregroundStyle(Color(braveSystemName: .textSecondary))
    .fontWeight(.medium)
    .padding(.horizontal, 16)
    .padding(.vertical, 8)
    .dynamicTypeSize(.xSmall..<DynamicTypeSize.accessibility2)
  }
}

// We must use a UISegmentedControl directly as you cannot change the height of a PickerView using
// a segmented picker style
private struct PrivateBrowsingPicker: UIViewRepresentable {
  @Binding var isPrivateBrowsing: Bool

  func makeUIView(context: Context) -> UISegmentedControl {
    let uiView = UISegmentedControl(
      frame: .zero,
      actions: [
        .init(
          title: Strings.TabGrid.regularBrowsingModeAccessibilityLabel,
          image: UIImage(braveSystemNamed: "leo.browser.mobile-tabs"),
          handler: { _ in }
        ),
        .init(
          title: Strings.TabGrid.privateBrowsingModeAccessibilityLabel,
          image: UIImage(braveSystemNamed: "leo.product.private-window"),
          handler: { _ in }
        ),
      ]
    )
    uiView.addAction(
      .init(handler: { [unowned uiView] _ in
        let isPrivate = uiView.selectedSegmentIndex == 1
        self.isPrivateBrowsing = isPrivate
      }),
      for: .valueChanged
    )
    return uiView
  }
  func updateUIView(_ uiView: UIViewType, context: Context) {
    uiView.selectedSegmentIndex = isPrivateBrowsing ? 1 : 0
    uiView.backgroundColor = UIColor(
      braveSystemName: isPrivateBrowsing ? .privateWindow10 : .neutral10
    )
    uiView.selectedSegmentTintColor = UIColor(
      braveSystemName: isPrivateBrowsing ? .privateWindow30 : .containerBackground
    )
    uiView.setTitleTextAttributes(
      [
        .foregroundColor: UIColor(
          braveSystemName: isPrivateBrowsing ? .iconInteractive : .iconDefault
        )
      ],
      for: .selected
    )
    uiView.setTitleTextAttributes(
      [.foregroundColor: UIColor(braveSystemName: .iconDefault)],
      for: .normal
    )
  }
  func sizeThatFits(
    _ proposal: ProposedViewSize,
    uiView: UISegmentedControl,
    context: Context
  ) -> CGSize? {
    return .init(width: uiView.intrinsicContentSize.width + 44, height: 40)
  }
}

private struct PrivateModeInfoView: View {
  var body: some View {
    VStack(spacing: 48) {
      Label {
        Text(Strings.TabGrid.privateBrowsingInfoTitle)
          .foregroundStyle(.white)
      } icon: {
        Image(braveSystemName: "leo.product.private-window")
          .foregroundStyle(Color(braveSystemName: .iconInteractive))
      }
      .font(.title2.weight(.semibold))
      VStack(spacing: 24) {
        BulletPointView(
          icon: "leo.product.private-window",
          title: Strings.TabGrid.privateBrowsingInfoBulletOneTitle,
          description: Strings.TabGrid.privateBrowsingInfoBulletOneBody
        )
        BulletPointView(
          icon: "leo.antenna",
          title: Strings.TabGrid.privateBrowsingInfoBulletTwoTitle,
          description: Strings.TabGrid.privateBrowsingInfoBulletTwoBody
        )
        BulletPointView(
          icon: "leo.product.vpn",
          title: Strings.TabGrid.privateBrowsingInfoBulletThreeTitle,
          description: Strings.TabGrid.privateBrowsingInfoBulletThreeBody
        )
      }
    }
  }

  private struct BulletPointView: View {
    var icon: String
    var title: String
    var description: String

    @ScaledMetric private var iconSize = 32

    var body: some View {
      HStack(alignment: .top, spacing: 16) {
        Image(braveSystemName: icon)
          .foregroundStyle(Color(braveSystemName: .primitivePrivateWindow60))
          .frame(width: iconSize, height: iconSize)
          .background(Color(braveSystemName: .primitivePrivateWindow20), in: .circle)
        VStack(alignment: .leading, spacing: 2) {
          Text(title)
            .foregroundStyle(Color(braveSystemName: .primitivePrivateWindow90))
            .font(.callout.weight(.semibold))
          Text(description)
            .foregroundStyle(Color(braveSystemName: .primitivePrivateWindow80))
            .font(.footnote)
        }
        .frame(maxWidth: .infinity, alignment: .leading)
      }
    }
  }
}

#if DEBUG
#Preview {
  let viewModel = TabGridViewModel(
    tabManager: .init(
      windowId: .init(),
      rewards: nil,
      braveCore: nil,
      privateBrowsingManager: .init()
    ),
    historyModel: nil,
    openTabsModel: MockOpenTabsModel(),
    toolbarUrlActionsDelegate: nil,
    profileController: nil,
    windowProtection: nil
  )
  TabGridView(
    viewModel: viewModel,
    containerView: .init(isPrivateBrowsing: viewModel.isPrivateBrowsing)
  )
}
#endif
