// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShared
import BraveStrings
import BraveUI
import DesignSystem
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
    windowProtection: WindowProtection?,
    didAddTab: @escaping () -> Void
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
    super.init(
      rootView: TabGridView(
        viewModel: viewModel,
        containerView: containerView,
        didAddTab: didAddTab
      )
    )
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
}

struct TabGridView: View {
  @Bindable var viewModel: TabGridViewModel
  var containerView: TabGridContainerView
  var didAddTab: () -> Void

  @Environment(\.dismiss) private var dismiss
  @Environment(\.colorScheme) private var colorScheme
  @Environment(\.horizontalSizeClass) private var horizontalSizeClass

  @State private var editMode: EditMode = .inactive
  @State private var selectedTabs: Set<TabState.ID> = []

  @ObservedObject private var privateBrowsingOnly = Preferences.Privacy.privateBrowsingOnly

  private enum DestinationSheet: Int, Identifiable, Hashable {
    case syncedTabs
    case history
    case privateTabsSettings

    var id: Int {
      rawValue
    }
  }

  private enum ActiveShredMode: Equatable {
    case selectedTab
    case selectedTabs
    case allTabs
  }

  @State private var insets: EdgeInsets = .zero
  @State private var destinationSheet: DestinationSheet?
  @State private var isSyncSettingsPresented: Bool = false
  @State private var isShredAlertPresented: Bool = false
  @State private var activeShredMode: ActiveShredMode?
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

  private var isSearchBarHidden: Bool {
    (viewModel.isPrivateBrowsing && viewModel.tabs.isEmpty && !viewModel.isSearching)
      || editMode == .active
  }

  private var isShredButtonVisible: Bool {
    !viewModel.tabs.isEmpty && viewModel.isShredMenuVisible
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
        let isGridHidden = viewModel.isPrivateBrowsing && !viewModel.isSceneActive
        TabGridContainerViewRepresentable(
          viewModel: viewModel,
          containerView: containerView,
          tabs: viewModel.tabs,
          isPrivateBrowsing: viewModel.isPrivateBrowsing,
          insets: insets,
          contextMenuForTabs: { tabs in
            contextMenu(for: tabs)
          },
          selectedTabList: $selectedTabs
        )
        .opacity(isGridHidden ? 0 : 1)
        .accessibilityHidden(isGridHidden)
      }
    }
    .environment(\.editMode, $editMode)
    .frame(maxWidth: .infinity, maxHeight: .infinity)
    .mask {
      // TODO: Test Performance of this mask w/ and w/o drawingGroup
      let radius = 5.0
      Color.black
        .blur(radius: radius)
        .padding(.horizontal, -(radius * 2))
        .padding(insets)
        .animation(.toolbarsSizeAnimation, value: insets)
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
      VStack(spacing: 12) {
        let isHeaderDisabled = editMode == .active && !viewModel.isSearching
        headerBar
          .transition(.blurReplace().animation(.default))
          .animation(
            .default,
            body: { content in
              content
                .opacity(isHeaderDisabled ? 0.5 : 1)
                .disabled(isHeaderDisabled)
            }
          )
        if editMode == .active {
          editModeHeaderBar
            .transition(.blurReplace())
        }
      }
      .padding(.horizontal, 16)
      // Asymmetrical padding because the bottom needs to account for some distance to the grid
      .padding(.top, 8)
      .padding(.bottom, 12)
      .onGeometryChange(for: CGFloat.self, of: \.size.height) {
        insets.top = $0
      }
      .contentShape(.rect)  // Dont trigger background tap from tapping empty area
    }
    .overlay(alignment: .bottom) {
      VStack {
        Group {
          if editMode == .active {
            editModeFooterBar
          } else if !viewModel.isSearching {
            footerBar
          }
        }
        .transition(.blurReplace())
      }
      .onGeometryChange(for: CGFloat.self, of: \.size.height) {
        insets.bottom = $0
      }
      .contentShape(.rect)  // Dont trigger background tap from tapping empty area
    }
    .onChange(of: viewModel.isSearching) { oldValue, newValue in
      // Reset the bottom inset since we hide the footer when searching
      if newValue {
        insets.bottom = 0
      }
    }
    .background {
      orphanKeyboardShortcuts
        .disabled(viewModel.isSearching || editMode == .active)
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
      case .privateTabsSettings:
        TabGridPrivateTabsSettings(viewModel: viewModel)
          .presentationDetents([.medium])
          .colorScheme(.dark)
          .preferredColorScheme(.dark)
      }
    }
    .alert(
      Strings.Shields.shredSiteDataConfirmationTitle,
      isPresented: $isShredAlertPresented,
      presenting: activeShredMode,
      actions: { _ in
        Button(role: .destructive) {
          isShredAnimationVisible = true
        } label: {
          Text(Strings.Shields.shredDataButtonTitle)
        }
        .keyboardShortcut(.defaultAction)
      },
      message: { shredMode in
        let message =
          switch shredMode {
          case .allTabs:
            Strings.Shields.shredSiteAllTabsConfirmationMessage
          case .selectedTabs:
            Strings.Shields.shredSiteSelectedTabsConfirmationMessage
          case .selectedTab:
            Strings.Shields.shredSiteDataConfirmationMessage
          }
        Text(message)
      }
    )
    .overlay {
      if isShredAnimationVisible, let shredMode = activeShredMode {
        LottieView(animation: .named("shred", bundle: .module))
          .configure { view in
            // resizable modifier below doesn't actually adjust the fill mode so we need to
            // configure this separately
            view.contentMode = .scaleAspectFill
          }
          .resizable()
          .playing(loopMode: .playOnce)
          .animationDidFinish { _ in
            let originalTabIDs = Set(viewModel.tabs.map(\.id))
            var dismissAfterShred = originalTabIDs.count == 1 || shredMode == .allTabs
            switch shredMode {
            case .selectedTab:
              viewModel.shredSelectedTab()
            case .selectedTabs:
              let affectedTabsIDs = viewModel.shredSelectedTabs(Set(selectedTabs))
              if !dismissAfterShred {
                dismissAfterShred = originalTabIDs.count == affectedTabsIDs.count
              }
            case .allTabs:
              viewModel.shredAllTabs()
            }
            withAnimation {
              isShredAnimationVisible = false
              activeShredMode = nil
              editMode = .inactive
              selectedTabs = []
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
    HStack(spacing: 12) {
      if !viewModel.isSearching {
        moreMenu
      }
      if !isSearchBarHidden {
        TabGridSearchBar(
          text: $viewModel.searchQuery,
          isFocused: $viewModel.isSearching,
          scrollView: containerView.collectionView
        )
        .frame(maxWidth: .infinity)
      } else if !viewModel.isSearching {
        Spacer()
      }
      if !viewModel.isSearching, isShredButtonVisible {
        shredMenu
      }
    }
    .containerCornerOffset(.leading, sizeToFit: true)
    .foregroundStyle(Color(braveSystemName: .textSecondary))
    .dynamicTypeSize(.xSmall..<DynamicTypeSize.accessibility2)
  }

  var moreMenu: some View {
    Menu {
      ControlGroup {
        Button {
          destinationSheet = .history
        } label: {
          Label(Strings.TabGrid.viewHistoryMenuItemLabel, braveSystemImage: "leo.history")
        }
        .accessibilityLabel(Strings.TabGrid.viewHistoryAccessibilityLabel)
        Button {
          destinationSheet = .syncedTabs
        } label: {
          Label(
            Strings.TabGrid.viewSyncedTabsMenuItemLabel,
            braveSystemImage: "leo.smartphone.laptop"
          )
        }
        .accessibilityLabel(Strings.TabGrid.viewSyncedTabsAccessibilityLabel)
      }
      .controlGroupStyle(.menu)
      if viewModel.isPrivateBrowsing && !privateBrowsingOnly.value {
        Section {
          Button {
            destinationSheet = .privateTabsSettings
          } label: {
            Label(Strings.TabGrid.privateTabsSettingsTitle, braveSystemImage: "leo.settings")
          }
        }
      }
      if !viewModel.tabs.isEmpty {
        Section {
          Button {
            withAnimation {
              editMode = .active
            }
          } label: {
            Label(
              Strings.TabGrid.selectTabsButtonTitle,
              braveSystemImage: "leo.check.circle-outline"
            )
          }
          Button(role: .destructive) {
            viewModel.closeAllTabs()
            dismiss()
          } label: {
            Label(Strings.TabGrid.closeAllTabsButtonTitle, braveSystemImage: "leo.close")
          }
        }
      }
    } label: {
      Label(Strings.TabGrid.moreMenuButtonTitle, braveSystemImage: "leo.more.horizontal")
        .labelStyle(.iconOnly)
        .font(.callout)
        .imageScale(.large)
    }
    .tabGridChromeButtonStyle()
    .menuOrder(.fixed)
    .accessibilityLabel(Strings.TabGrid.moreMenuButtonTitle)
  }

  var shredMenu: some View {
    Menu {
      Button {
        isShredAlertPresented = true
        activeShredMode = .selectedTab
      } label: {
        Text(Strings.TabGrid.shredSelectedTabButtonTitle)
      }
      .disabled(!viewModel.isSelectedTabShredAvailable)
      Button(role: .destructive) {
        isShredAlertPresented = true
        activeShredMode = .allTabs
      } label: {
        Text(Strings.TabGrid.shredAllTabsButtonTitle)
      }
    } label: {
      Label(Strings.TabGrid.shredTabsAccessibilityLabel, braveSystemImage: "leo.shred.data")
        .labelStyle(.iconOnly)
        .font(.callout)
        .imageScale(.large)
    }
    .tabGridChromeButtonStyle()
  }

  var footerBar: some View {
    HStack {
      Button {
        viewModel.addTab()
        // Let the tab show up in the collection view first
        DispatchQueue.main.async { [self] in
          dismiss()
          didAddTab()
        }
      } label: {
        Label(Strings.TabGrid.newTabAccessibilityLabel, braveSystemImage: "leo.plus.add")
          .labelStyle(.iconOnly)
          .font(.callout)
          .imageScale(.large)
      }
      .tabGridChromeButtonStyle()
      .keyboardShortcut("t", modifiers: [.command])
      .frame(maxWidth: .infinity, alignment: .leading)

      if !privateBrowsingOnly.value {
        TabGridModeSwitcher(
          isPrivateBrowsing: $viewModel.isPrivateBrowsing,
          regularTabCount: viewModel.regularTabCount
        )
      }

      Button {
        if viewModel.tabs.isEmpty {
          viewModel.addTab()
        }
        dismiss()
      } label: {
        Label(Strings.done, braveSystemImage: "leo.check.normal")
          .labelStyle(.iconOnly)
          .font(.callout)
          .imageScale(.large)
      }
      .tabGridChromeButtonStyle(isDone: true)
      .keyboardShortcut(.defaultAction)
      .frame(maxWidth: .infinity, alignment: .trailing)
    }
    .foregroundStyle(Color(braveSystemName: .textSecondary))
    .fontWeight(.medium)
    .padding(.horizontal, 16)
    .padding(.vertical, 8)
    .dynamicTypeSize(.xSmall..<DynamicTypeSize.accessibility2)
  }

  var editModeHeaderBar: some View {
    HStack {
      HStack {
        Text(Strings.TabGrid.selectedTabs)
          .foregroundStyle(Color(braveSystemName: .textPrimary))
        Text(selectedTabs.count, format: .number)
          .foregroundStyle(Color(braveSystemName: .textTertiary))
      }
      .accessibilityElement()
      .font(.title3.weight(.semibold))
      Spacer()
      Button {
        withAnimation {
          selectedTabs = []
          editMode = .inactive
        }
      } label: {
        Text(Strings.CancelString)
          .fontWeight(.medium)
          .foregroundStyle(Color(braveSystemName: .textSecondary))
      }
    }
    .containerCornerOffset(.leading, sizeToFit: true)
    .padding(.vertical, 4)
  }

  var editModeFooterBar: some View {
    HStack {
      Button {
        isShredAlertPresented = true
        activeShredMode = .selectedTabs
      } label: {
        Label(Strings.TabGrid.shredSelectedTabsButtonTitle, braveSystemImage: "leo.shred.data")
          .frame(maxWidth: .infinity)
      }
      .buttonStyle(.outline)
      .disabled(selectedTabs.isEmpty || !viewModel.isShredAvailableForSelectedTabs(selectedTabs))
      Button {
        withAnimation {
          let dismissAfterClose = selectedTabs.count == viewModel.tabs.count
          editMode = .inactive
          viewModel.closeTabs(selectedTabs)
          selectedTabs = []
          if dismissAfterClose {
            dismiss()
          }
        }
      } label: {
        Label(Strings.close, braveSystemImage: "leo.close")
          .frame(maxWidth: .infinity)
      }
      .buttonStyle(.filled)
      .disabled(selectedTabs.isEmpty)
    }
    .padding(.horizontal, 16)
    .padding(.vertical, 8)
  }

  private func contextMenu(for tabs: [any TabState]) -> UIMenu? {
    let tabIDs = Set(tabs.map(\.id))
    let isShredAvailable = viewModel.isShredAvailableForSelectedTabs(tabIDs)
    let allTabsAreSelected = editMode == .active && selectedTabs == tabIDs
    let selectAndShred = UIMenu(
      options: .displayInline,
      children: [
        UIAction(
          title: allTabsAreSelected
            ? Strings.TabGrid.deselectTabsButtonTitle : Strings.TabGrid.selectTabsButtonTitle,
          image: UIImage(braveSystemNamed: "leo.check.circle-outline"),
          handler: { _ in
            withAnimation {
              if allTabsAreSelected {
                // Deselect tabs
                selectedTabs = []
              } else {
                selectedTabs.formUnion(tabIDs)
              }
              editMode = .active
            }
          }
        ),
        UIAction(
          title: Strings.Shields.shredSiteData,
          image: UIImage(braveSystemNamed: "leo.shred.data"),
          attributes: .destructive.union(!isShredAvailable ? .disabled : []),
          handler: { _ in
            selectedTabs.formUnion(tabIDs)
            activeShredMode = .selectedTabs
            isShredAlertPresented = true
          }
        ),
      ]
    )
    let closeTabs = UIMenu(
      options: .displayInline,
      children: [
        UIAction(
          title: Strings.closeAllOtherTabsTitle,
          image: UIImage(braveSystemNamed: "leo.close"),
          attributes: .destructive.union(viewModel.tabs.count == tabIDs.count ? .disabled : []),
          handler: { _ in
            withAnimation {
              viewModel.closeOtherTabs(tabIDs)
              editMode = .inactive
              selectedTabs = []
            }
          }
        ),
        UIAction(
          title: tabIDs.count == 1 ? Strings.TabGrid.closeTab : Strings.TabGrid.closeTabs,
          image: UIImage(braveSystemNamed: "leo.close"),
          attributes: .destructive,
          handler: { _ in
            withAnimation {
              let dismissAfterClose = tabIDs.count == viewModel.tabs.count
              editMode = .inactive
              viewModel.closeTabs(tabIDs)
              selectedTabs = []
              if dismissAfterClose {
                dismiss()
              }
            }
          }
        ),
      ]
    )
    return UIMenu(children: [
      selectAndShred,
      closeTabs,
    ])
  }
}

extension View {
  /// Since Tab tray chrome lives in custom overlays, not `.toolbar`,  iOS 26 does not apply liquid glass automatically.
  /// Use glass button styles on 26+ and circle-shaped filled/plain styles on earlier releases.
  @ViewBuilder
  func tabGridChromeButtonStyle(isDone: Bool = false) -> some View {
    if isDone {
      if #available(iOS 26.0, *) {
        buttonStyle(.glassFilledCircle)
      } else {
        buttonStyle(.filledCircle)
      }
    } else {
      if #available(iOS 26.0, *) {
        buttonStyle(.plainGlassCircle)
      } else {
        buttonStyle(.plainBorderedCircle)
      }
    }
  }
}

// We must use a UISegmentedControl directly as you cannot change the height of a PickerView using
// a segmented picker style
private struct TabGridModeSwitcher: UIViewRepresentable {
  @Binding var isPrivateBrowsing: Bool
  var regularTabCount: Int

  func makeUIView(context: Context) -> UISegmentedControl {
    let uiView = UISegmentedControl(
      items: [
        Strings.TabGrid.tabsCountFormat(regularTabCount),
        Strings.TabGrid.privateBrowsingModeTitle,
      ]
    )
    uiView.addAction(
      .init(handler: { [unowned uiView] _ in
        isPrivateBrowsing = uiView.selectedSegmentIndex == 1
      }),
      for: .valueChanged
    )
    configureAppearance(uiView)
    return uiView
  }

  func updateUIView(_ uiView: UISegmentedControl, context: Context) {
    uiView.setTitle(Strings.TabGrid.tabsCountFormat(regularTabCount), forSegmentAt: 0)
    uiView.setTitle(Strings.TabGrid.privateBrowsingModeTitle, forSegmentAt: 1)
    uiView.selectedSegmentIndex = isPrivateBrowsing ? 1 : 0
    configureAppearance(uiView)
  }

  func sizeThatFits(
    _ proposal: ProposedViewSize,
    uiView: UISegmentedControl,
    context: Context
  ) -> CGSize? {
    return .init(width: uiView.intrinsicContentSize.width + 44, height: 40)
  }

  private func configureAppearance(_ uiView: UISegmentedControl) {
    uiView.backgroundColor = UIColor(
      braveSystemName: isPrivateBrowsing ? .privateWindow10 : .neutral10
    )
    uiView.selectedSegmentTintColor = UIColor(braveSystemName: .containerBackground)
    let titleFont = UIFont.preferredFont(forTextStyle: .subheadline)
    uiView.setTitleTextAttributes(
      [
        .font: titleFont,
        .foregroundColor: UIColor(
          braveSystemName: isPrivateBrowsing ? .iconInteractive : .iconDefault
        ),
      ],
      for: .selected
    )
    uiView.setTitleTextAttributes(
      [
        .font: titleFont,
        .foregroundColor: UIColor(braveSystemName: .iconDefault),
      ],
      for: .normal
    )
  }
}

extension Strings.TabGrid {
  fileprivate static func tabsCountFormat(_ count: Int) -> String {
    String.localizedStringWithFormat(
      count == 1
        ? Strings.TabGrid.tabsCountSingularFormatString
        : Strings.TabGrid.tabsCountPluralFormatString,
      count
    )
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

extension Animation {
  /// A shared animation that will be used when animating the toolbar size like when searching or
  /// entering multi-select mode
  static var toolbarsSizeAnimation: Animation {
    .spring(response: 0.3)
  }
}

#if DEBUG
#Preview {
  let viewModel = TabGridViewModel(
    tabManager: .init(
      windowId: .init(),
      rewards: nil,
      braveCore: nil,
      profile: FakeProfile(),
      privateBrowsingManager: .init(),
      tabCreationFactory: { _ in
        FakeTabState()
      }
    ),
    historyModel: nil,
    openTabsModel: MockOpenTabsModel(),
    toolbarUrlActionsDelegate: nil,
    profileController: nil,
    windowProtection: nil
  )
  TabGridView(
    viewModel: viewModel,
    containerView: .init(isPrivateBrowsing: viewModel.isPrivateBrowsing),
    didAddTab: {}
  )
}
#endif
