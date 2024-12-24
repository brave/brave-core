// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShared
import BraveShields
import BraveUI
import Combine
import Data
import LocalAuthentication
import Lottie
import Preferences
import Shared
import SnapKit
import SwiftUI
import UIKit

protocol TabTrayDelegate: AnyObject {
  /// Notifies the delegate that order of tabs on tab tray has changed.
  /// This info can be used to update UI in other place, for example update order of tabs in tabs bar.
  func tabOrderChanged()
  /// Notifies the delegate that the user tapped the add button to create a tab and the tab tray
  /// used the tab manager to create one
  func didCreateTab()
}

class TabTrayController: AuthenticationController {

  typealias DataSource = UICollectionViewDiffableDataSource<TabTraySection, Tab>
  typealias Snapshot = NSDiffableDataSourceSnapshot<TabTraySection, Tab>

  // MARK: Internal

  enum TabTraySection {
    case main
  }

  enum TabTrayMode {
    case local
    case sync
  }

  // MARK: SyncStatusState

  enum SyncStatusState {
    case noSyncChain
    case openTabsDisabled
    case noSyncedSessions
    case activeSessions
  }

  let tabManager: TabManager
  let braveCore: BraveCoreMain

  private var openTabsSessionServiceListener: OpenTabsSessionStateListener?
  private var syncServicStateListener: AnyObject?

  weak var delegate: TabTrayDelegate?
  weak var toolbarUrlActionsDelegate: ToolbarUrlActionsDelegate?

  private var emptyPanelState: SyncStatusState {
    if Preferences.Chromium.syncEnabled.value {
      if !Preferences.Chromium.syncOpenTabsEnabled.value {
        return .openTabsDisabled
      }
    } else {
      return .noSyncChain
    }

    if sessionList.count > 0 {
      return .activeSessions
    }

    return .noSyncedSessions
  }

  private(set) lazy var dataSource =
    DataSource(
      collectionView: tabTrayView.collectionView,
      cellProvider: { [weak self] collectionView, indexPath, tab -> UICollectionViewCell? in
        self?.cellProvider(collectionView: collectionView, indexPath: indexPath, tab: tab)
      }
    )

  private(set) var sessionList = [OpenDistantSession]()
  var hiddenSections = Set<Int>()

  private(set) var privateMode: Bool = false {
    didSet {
      // Should be set immediately before other logic executes
      tabManager.privateBrowsingManager.isPrivateBrowsing = privateMode
      applySnapshot()

      privateModeButton.isSelected = privateMode
      tabTypeSelector.isHidden = privateMode
      tabTypeSelectorContainerView.isHidden =
        privateMode && !BraveCore.FeatureList.kBraveShredFeature.enabled
    }
  }

  var searchTabTrayTimer: Timer?
  var isTabTrayBeingSearched = false
  var tabTraySearchQuery: String?
  var tabTrayMode: TabTrayMode = .local
  // The tab tray is presented by an action outside the application like shortcuts
  private var isExternallyPresented: Bool
  private var privateModeCancellable: AnyCancellable?
  private var initialScrollCompleted = false
  private var localAuthObservers = Set<AnyCancellable>()

  // MARK: User Interface Elements

  private struct UX {
    static let horizontalInset = 5.0
    static let buttonEdgeInset = 10.0
    static let segmentedControlTopInset = 16.0
  }

  private let containerView = UIView()

  private let tabContentView = UIView()

  private let tabTypeSelectorContainerView = UIView()
  private var tabTypeSelectorItems = [UIImage]()
  private lazy var tabTypeSelector: UISegmentedControl = {
    let segmentedControl = UISegmentedControl(items: tabTypeSelectorItems).then {
      $0.selectedSegmentIndex = 0
      $0.addTarget(self, action: #selector(typeSelectionDidChange(_:)), for: .valueChanged)
    }
    return segmentedControl
  }()

  var tabTrayView = TabTrayContainerView().then {
    $0.isHidden = false
  }

  var tabSyncView = TabSyncContainerView().then {
    $0.isHidden = true
  }

  let newTabButton = UIButton(type: .system).then {
    $0.setImage(UIImage(braveSystemNamed: "leo.plus.add"), for: .normal)
    $0.accessibilityLabel = Strings.tabTrayAddTabAccessibilityLabel
    $0.accessibilityIdentifier = "TabTrayController.addTabButton"
    $0.contentEdgeInsets = .init(
      top: 0,
      left: UX.buttonEdgeInset,
      bottom: 0,
      right: UX.buttonEdgeInset
    )
    $0.setContentCompressionResistancePriority(.required, for: .horizontal)
  }

  let doneButton = UIButton(type: .system).then {
    $0.setTitle(Strings.done, for: .normal)
    $0.titleLabel?.font = .preferredFont(forTextStyle: .body)
    $0.titleLabel?.adjustsFontForContentSizeCategory = true
    $0.contentHorizontalAlignment = .right
    $0.titleLabel?.adjustsFontSizeToFitWidth = true
    $0.accessibilityLabel = Strings.done
    $0.accessibilityIdentifier = "TabTrayController.doneButton"
  }

  let privateModeButton = SelectedInsetButton().then {
    $0.setTitle(Strings.private, for: .normal)
    $0.titleLabel?.font = .preferredFont(forTextStyle: .body)
    $0.titleLabel?.adjustsFontForContentSizeCategory = true
    $0.contentHorizontalAlignment = .left
    $0.titleLabel?.adjustsFontSizeToFitWidth = true
    $0.accessibilityLabel = Strings.done
    $0.accessibilityIdentifier = "TabTrayController.privateButton"
    $0.tintColor = .braveLabel
    $0.selectedBackgroundColor = UIColor(braveSystemName: .primitivePrivateWindow25)

    if Preferences.Privacy.privateBrowsingOnly.value {
      $0.alpha = 0
    }
  }

  lazy var shredButton: UIButton = {
    var button = UIButton(frame: .zero)
    button.setImage(UIImage(braveSystemNamed: "leo.shred.data"), for: .normal)
    button.tintColor = .bravePrimary
    button.titleLabel?.font = .preferredFont(forTextStyle: .body)
    button.titleLabel?.adjustsFontForContentSizeCategory = true
    button.contentHorizontalAlignment = .right
    button.titleLabel?.adjustsFontSizeToFitWidth = true
    button.accessibilityLabel = Strings.Shields.shredSiteData
    button.accessibilityIdentifier = "TabTrayController.shredButton"
    return button
  }()

  private var searchBarView: TabTraySearchBar?
  let tabTraySearchController = UISearchController(searchResultsController: nil)

  private lazy var emptyStateOverlayView: UIView = EmptyStateOverlayView(
    overlayDetails: EmptyOverlayStateDetails(title: Strings.noSearchResultsfound)
  )

  override var preferredStatusBarStyle: UIStatusBarStyle {
    if tabManager.privateBrowsingManager.isPrivateBrowsing {
      return .lightContent
    }

    return view.overrideUserInterfaceStyle == .light ? .darkContent : .lightContent
  }

  // MARK: Lifecycle

  init(
    isExternallyPresented: Bool = false,
    tabManager: TabManager,
    braveCore: BraveCoreMain,
    windowProtection: WindowProtection?
  ) {
    self.isExternallyPresented = isExternallyPresented
    self.tabManager = tabManager
    self.braveCore = braveCore

    super.init(
      windowProtection: windowProtection,
      isCancellable: true,
      unlockScreentitle: "Private Browsing is Locked"
    )

    if !UIAccessibility.isReduceMotionEnabled {
      transitioningDelegate = self
      modalPresentationStyle = .fullScreen
    }

    // Adding the Open Tabs session observer in constructor to watch synced sessions updating
    openTabsSessionServiceListener = braveCore.openTabsAPI.add(
      OpenTabsStateObserver { [weak self] stateChange in
        guard let self = self else { return }

        if case .openTabsSyncCycleCompleted = stateChange {
          if !self.isTabTrayBeingSearched {
            self.reloadOpenTabsSession()
          }
        }
      }
    )

    // Adding State Sync Observer to watch the states change in sync chain
    syncServicStateListener = braveCore.syncAPI.addServiceStateObserver { [weak self] in
      guard let self = self else { return }

      if self.braveCore.syncAPI.shouldLeaveSyncGroup {
        self.tabSyncView.do {
          $0.updateSyncStatusPanel(for: self.emptyPanelState)
        }
      }
    }
  }

  @available(*, unavailable)
  required init?(coder: NSCoder) { fatalError() }

  deinit {
    tabManager.removeDelegate(self)

    // Remove the open tabs service observer
    if let observer = openTabsSessionServiceListener {
      braveCore.openTabsAPI.removeObserver(observer)
    }
  }

  override func viewDidLoad() {
    super.viewDidLoad()

    definesPresentationContext = true

    searchBarView = TabTraySearchBar(searchBar: tabTraySearchController.searchBar).then {
      $0.searchBar.autocapitalizationType = .none
      $0.searchBar.autocorrectionType = .no
      $0.searchBar.placeholder = Strings.tabTraySearchBarTitle
    }

    tabTraySearchController.do {
      $0.searchResultsUpdater = self
      $0.obscuresBackgroundDuringPresentation = false
      $0.delegate = self
      // Don't hide the navigation bar because the search bar is in it.
      $0.hidesNavigationBarDuringPresentation = false
    }

    navigationItem.do {
      // Place the search bar in the navigation item's title view.
      $0.titleView = searchBarView
      $0.hidesSearchBarWhenScrolling = true
    }

    tabManager.addDelegate(self)

    tabTrayView.collectionView.do {
      $0.dataSource = dataSource
      $0.delegate = self
      $0.dragDelegate = self
      $0.dropDelegate = self
      $0.dragInteractionEnabled = true
      $0.backgroundView = UIView().then {
        $0.backgroundColor = .clear
        $0.addGestureRecognizer(
          UITapGestureRecognizer(target: self, action: #selector(tappedCollectionViewBackground))
        )
      }
    }

    tabSyncView.tableView.do {
      $0.dataSource = self
      $0.delegate = self
    }

    tabSyncView.actionHandler = { [weak self] status in
      self?.presentSyncSettings(status: status)
    }

    privateMode = tabManager.selectedTab?.isPrivate == true

    doneButton.addTarget(self, action: #selector(doneAction), for: .touchUpInside)
    newTabButton.addTarget(self, action: #selector(newTabAction), for: .touchUpInside)
    newTabButton.addGestureRecognizer(
      UILongPressGestureRecognizer(target: self, action: #selector(tappedButton(_:)))
    )
    privateModeButton.addTarget(
      self,
      action: #selector(togglePrivateModeAction),
      for: .touchUpInside
    )

    navigationController?.isToolbarHidden = false

    toolbarItems = [
      UIBarButtonItem(customView: privateModeButton),
      UIBarButtonItem(barButtonSystemItem: .flexibleSpace, target: self, action: nil),
      UIBarButtonItem(customView: newTabButton),
      UIBarButtonItem(barButtonSystemItem: .flexibleSpace, target: self, action: nil),
      UIBarButtonItem(customView: doneButton),
    ]

    privateModeCancellable = tabManager.privateBrowsingManager
      .$isPrivateBrowsing
      .removeDuplicates()
      .receive(on: RunLoop.main)
      .sink(receiveValue: { [weak self] _ in
        self?.updateColors()
      })

    windowProtection?.cancelPressed
      .sink { [weak self] _ in
        self?.navigationController?.popViewController(animated: true)
      }.store(in: &localAuthObservers)

    windowProtection?.finalizedAuthentication
      .sink { [weak self] success, viewType in
        if success, viewType == .tabTray {
          self?.toggleModeChanger()
        }
        self?.navigationController?.popViewController(animated: true)
      }.store(in: &localAuthObservers)

    reloadOpenTabsSession()
    updateColors()

    shredButton.addTarget(self, action: #selector(shredButtonPressed), for: .touchUpInside)
    becomeFirstResponder()
    
    NotificationCenter.default.do {
      $0.addObserver(
        self,
        selector: #selector(sceneWillResignActiveNotification(_:)),
        name: UIScene.willDeactivateNotification,
        object: nil
      )
      $0.addObserver(
        self,
        selector: #selector(sceneDidBecomeActiveNotification(_:)),
        name: UIScene.didActivateNotification,
        object: nil
      )
    }
  }

  override func viewDidAppear(_ animated: Bool) {
    super.viewDidAppear(animated)

    // Navigate tabs from other devices
    if isExternallyPresented {
      tabTypeSelector.selectedSegmentIndex = 1
      tabTypeSelector.sendActions(for: UIControl.Event.valueChanged)
    }
  }

  override func loadView() {
    createTypeSelectorItems()
    layoutTabTray()
  }

  override var canBecomeFirstResponder: Bool {
    return true
  }

  private func layoutTabTray() {
    let contentStackView = UIStackView().then {
      $0.axis = .vertical
      $0.alignment = .center
      $0.layoutMargins = UIEdgeInsets(
        top: UX.segmentedControlTopInset,
        left: 0,
        bottom: 0,
        right: 0
      )
      $0.isLayoutMarginsRelativeArrangement = true
    }

    tabTypeSelectorContainerView.addSubview(tabTypeSelector)
    contentStackView.addArrangedSubview(tabTypeSelectorContainerView)
    contentStackView.setCustomSpacing(
      UX.segmentedControlTopInset,
      after: tabTypeSelectorContainerView
    )
    contentStackView.addArrangedSubview(tabContentView)

    containerView.addSubview(contentStackView)

    if FeatureList.kBraveShredFeature.enabled {
      tabTypeSelectorContainerView.addSubview(shredButton)

      shredButton.snp.makeConstraints {
        $0.leading.greaterThanOrEqualTo(tabTypeSelector.snp.trailing)
        $0.trailing.equalTo(tabTypeSelectorContainerView.snp.trailing)
        $0.centerY.equalTo(tabTypeSelectorContainerView)
        $0.top.greaterThanOrEqualTo(tabTypeSelectorContainerView.snp.top)
        $0.bottom.lessThanOrEqualTo(tabTypeSelectorContainerView.snp.bottom)
      }
    }

    contentStackView.snp.makeConstraints {
      $0.edges.equalTo(containerView.safeAreaLayoutGuide)
    }

    tabTypeSelectorContainerView.snp.makeConstraints {
      $0.width.equalTo(containerView.layoutMarginsGuide.snp.width)
    }

    tabTypeSelector.snp.makeConstraints {
      $0.width.equalTo(contentStackView.snp.width).dividedBy(2)
      $0.center.top.bottom.equalTo(tabTypeSelectorContainerView)
    }

    tabContentView.addSubview(tabTrayView)
    tabContentView.addSubview(tabSyncView)

    tabTrayView.snp.makeConstraints {
      $0.edges.equalToSuperview()
    }

    tabSyncView.snp.makeConstraints {
      $0.edges.equalToSuperview()
    }

    view = containerView
  }

  private func createTypeSelectorItems() {
    tabTypeSelectorItems = [
      UIImage(braveSystemNamed: "leo.browser.mobile-tabs")!.template,
      UIImage(braveSystemNamed: "leo.smartphone.laptop")!.template,
    ]
  }

  @objc func typeSelectionDidChange(_ sender: UISegmentedControl) {
    tabTrayMode = sender.selectedSegmentIndex == 0 ? .local : .sync

    tabTrayView.isHidden = tabTrayMode == .sync
    tabSyncView.isHidden = tabTrayMode == .local

    updateShredButtonVisibility()

    searchBarView?.searchBar.placeholder =
      tabTrayMode == .local
      ? Strings.tabTraySearchBarTitle : Strings.OpenTabs.tabTrayOpenTabSearchBarTitle

    refreshDataSource()
  }

  func refreshDataSource() {
    switch tabTrayMode {
    case .local:
      applySnapshot(for: tabTraySearchQuery)
    case .sync:
      reloadOpenTabsSession(for: tabTraySearchQuery)
    }
  }

  private func updateColors() {
    let privateBrowsingManager = tabManager.privateBrowsingManager
    let browserColors = privateBrowsingManager.browserColors
    containerView.backgroundColor = browserColors.chromeBackground
    tabTypeSelector.backgroundColor = browserColors.chromeBackground
    newTabButton.tintColor = browserColors.iconDefault
    privateModeButton.setTitleColor(browserColors.textSecondary, for: .normal)
    privateModeButton.setTitleColor(browserColors.textPrimary, for: .selected)

    let toolbarAppearance = UIToolbarAppearance()
    toolbarAppearance.configureWithOpaqueBackground()
    toolbarAppearance.backgroundColor = browserColors.chromeBackground
    toolbarAppearance.backgroundEffect = nil
    navigationController?.toolbar.do {
      $0.tintColor = browserColors.textSecondary
      $0.standardAppearance = toolbarAppearance
      $0.compactAppearance = toolbarAppearance
      $0.scrollEdgeAppearance = toolbarAppearance
    }

    let navBarAppearance = UINavigationBarAppearance()
    navBarAppearance.configureWithOpaqueBackground()
    navBarAppearance.backgroundColor = browserColors.chromeBackground
    navBarAppearance.backgroundEffect = nil
    navigationController?.navigationBar.do {
      $0.tintColor = browserColors.textSecondary
      $0.standardAppearance = navBarAppearance
      $0.compactAppearance = navBarAppearance
      $0.scrollEdgeAppearance = navBarAppearance
    }

    shredButton.tintColor = browserColors.textPrimary

    // Need to force a relayout for the nav controller for the appearance to take affect
    navigationController?.view.setNeedsLayout()
    navigationController?.view.layoutIfNeeded()

    setNeedsStatusBarAppearanceUpdate()
  }

  override func viewDidLayoutSubviews() {
    super.viewDidLayoutSubviews()

    // When user opens the tray for the first time, we scroll the collection view to selected tab.
    if initialScrollCompleted { return }

    if let selectedTab = tabManager.selectedTab,
      let selectedIndexPath = dataSource.indexPath(for: selectedTab)
    {
      DispatchQueue.main.async {
        self.tabTrayView.collectionView.scrollToItem(
          at: selectedIndexPath,
          at: .centeredVertically,
          animated: false
        )
      }

      initialScrollCompleted = true
    }
  }

  override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
    super.traitCollectionDidChange(previousTraitCollection)

    searchBarView?.frame = navigationController?.navigationBar.frame ?? .zero
    navigationItem.titleView = searchBarView

    searchBarView?.setNeedsLayout()
    searchBarView?.layoutIfNeeded()
  }

  override func motionEnded(_ motion: UIEvent.EventSubtype, with event: UIEvent?) {
    guard !tabManager.privateBrowsingManager.isPrivateBrowsing,
      let recentlyClosedTab = RecentlyClosed.all().first
    else {
      return
    }

    if motion == .motionShake {
      let alert = UIAlertController(
        title: Strings.Hotkey.recentlyClosedTabTitle,
        message: Strings.RecentlyClosed.recentlyClosedShakeActionDescription,
        preferredStyle: .alert
      )

      alert.addAction(
        UIAlertAction(
          title: Strings.RecentlyClosed.recentlyClosedOpenActionTitle,
          style: .default,
          handler: { [weak self] _ in
            self?.tabManager.addAndSelectRecentlyClosed(recentlyClosedTab)
            RecentlyClosed.remove(with: recentlyClosedTab.url)
          }
        )
      )
      alert.addAction(UIAlertAction(title: Strings.cancelButtonTitle, style: .cancel, handler: nil))

      self.present(alert, animated: true, completion: nil)
    }
  }

  // MARK: Snapshot handling

  func applySnapshot(for query: String? = nil) {
    var snapshot = Snapshot()
    snapshot.appendSections([.main])
    snapshot.appendItems(tabManager.tabsForCurrentMode(for: query))
    dataSource.apply(snapshot, animatingDifferences: true) { [weak self] in
      guard let self = self else { return }

      let isSearchResultEmpty =
        self.dataSource.snapshot().numberOfItems == 0 && self.isTabTrayBeingSearched
      self.updateEmptyPanelState(isHidden: !isSearchResultEmpty)
    }
  }

  /// Reload the data source even if no changes are present. Use with caution.
  func forceReload() {
    var snapshot = dataSource.snapshot()
    snapshot.reloadSections([.main])
    dataSource.apply(snapshot, animatingDifferences: false)
  }

  private func cellProvider(
    collectionView: UICollectionView,
    indexPath: IndexPath,
    tab: Tab
  ) -> UICollectionViewCell? {
    guard
      let cell =
        collectionView
        .dequeueReusableCell(
          withReuseIdentifier: TabCell.identifier,
          for: indexPath
        ) as? TabCell
    else { return UICollectionViewCell() }

    cell.configure(with: tab)

    if tab == tabManager.selectedTab {
      cell.setTabSelected(tab)
    }

    cell.closedTab = { [weak self] tab in
      self?.remove(tab: tab)
      UIAccessibility.post(
        notification: .announcement,
        argument: Strings.tabTrayClosingTabAccessibilityNotificationText
      )
    }

    return cell
  }

  func reloadOpenTabsSession(for query: String? = nil) {
    // Fetch all synced session from devices with open tabs information
    // Query is nil when search bar is inactive
    sessionList = fetchSyncedSessions(for: query)

    tabSyncView.do {
      $0.tableView.reloadData()
      $0.updateSyncStatusPanel(for: emptyPanelState)
    }

    updateEmptyPanelState(
      isHidden: !(isTabTrayBeingSearched && sessionList.isEmpty && !(query?.isEmpty ?? true))
    )
  }

  private func fetchSyncedSessions(for query: String? = nil) -> [OpenDistantSession] {
    let allSessions = braveCore.openTabsAPI.getSyncedSessions()
    var queriedSessions = [OpenDistantSession]()

    if let query = query, !query.isEmpty {
      for session in allSessions {
        let queriedSyncedTabList = session.tabs.filter {
          ($0.title?.lowercased().contains(query) ?? false)
            || ($0.url.baseDomain?.contains(query) ?? false)
        }

        if !queriedSyncedTabList.isEmpty {
          session.tabs = queriedSyncedTabList
          queriedSessions.append(session)
        }
      }

      return queriedSessions
    }

    return allSessions
  }

  private func updateEmptyPanelState(isHidden: Bool) {
    if isHidden {
      emptyStateOverlayView.removeFromSuperview()
    } else {
      showEmptyPanelState()
    }
  }

  private func showEmptyPanelState() {
    if emptyStateOverlayView.superview == nil {
      view.addSubview(emptyStateOverlayView)
      view.bringSubviewToFront(emptyStateOverlayView)
      emptyStateOverlayView.snp.makeConstraints {
        $0.edges.equalTo(tabTrayView.collectionView)
      }
    }
  }

  private func updateShredButtonVisibility() {
    let totalItems = dataSource.snapshot().numberOfItems
    shredButton.isHidden = tabTrayMode == .sync || (privateMode && totalItems == 0)
  }

  // MARK: - Actions

  @objc func doneAction() {
    tabTraySearchController.isActive = false

    dismiss(animated: true)
  }

  @objc func newTabAction() {
    tabTraySearchController.isActive = false

    let wasPrivateModeInfoShowing = tabTrayView.isPrivateModeInfoShowing
    if privateMode {
      tabTrayView.hidePrivateModeInfo()
      navigationController?.setNavigationBarHidden(false, animated: false)
    }

    // If private mode info is showing it means we already added one tab.
    // So when user taps on the 'new tab' button we do nothing, only dismiss the view.
    if wasPrivateModeInfoShowing {
      dismiss(animated: true)
    } else {
      tabManager.addTabAndSelect(isPrivate: privateMode)
    }

    delegate?.didCreateTab()
  }

  @objc func shredButtonPressed() {
    guard let tab = self.tabManager.selectedTab, let url = tab.url else { return }

    let alert = UIAlertController.shredDataAlert(url: url) { _ in
      LottieAnimationView.showShredAnimation(
        on: self.navigationController?.view ?? self.view
      ) { [weak self] in
        self?.tabManager.shredData(for: url, in: tab)
        self?.refreshDataSource()
        self?.tabTrayView.collectionView.reloadData()
      }
    }
    present(alert, animated: true)
  }

  @objc private func tappedButton(_ gestureRecognizer: UIGestureRecognizer) {
    if tabManager.privateBrowsingManager.isPrivateBrowsing {
      return
    }

    var recentlyClosedTabsView = RecentlyClosedTabsView(tabManager: tabManager)
    recentlyClosedTabsView.onRecentlyClosedSelected = { [weak self] recentlyClosed in
      guard let self else { return }

      self.tabManager.addAndSelectRecentlyClosed(recentlyClosed)
      // After opening the Recently Closed in a new tab delete it from list
      RecentlyClosed.remove(with: recentlyClosed.url)

      self.dismiss(animated: false)
    }

    present(UIHostingController(rootView: recentlyClosedTabsView), animated: true)
  }

  @objc func togglePrivateModeAction() {
    if !privateMode, Preferences.Privacy.privateBrowsingLock.value {
      askForAuthentication(viewType: .tabTray) { [weak self] success, error in
        if !success, error == LAError.passcodeNotSet {
          // If Pin code is not set in this device, private mode is enabled default
          self?.toggleModeChanger()
        }
      }
    } else {
      toggleModeChanger()
    }
  }

  @objc func sceneWillResignActiveNotification(_ notification: NSNotification) {
    guard let scene = notification.object as? UIScene, scene == currentScene else {
      return
    }

    // If we are in the private mode info showing, hide any elements in the tab that we wouldn't want shown
    // when the app is in the home switcher
    if privateMode {
      tabContentView.alpha = 0
    }
  }

  @objc func sceneDidBecomeActiveNotification(_ notification: NSNotification) {
    guard let scene = notification.object as? UIScene, scene == currentScene else {
      return
    }

    // Re-show any components that might have been hidden
    tabContentView.alpha = 1
  }

  func toggleModeChanger() {
    tabTraySearchController.isActive = false

    // Mode Change action disabled while drap-drop is active
    // Added to prevent Diffable Data source crash
    if tabTrayView.collectionView.hasActiveDrag || tabTrayView.collectionView.hasActiveDrop {
      return
    }

    // Record the selected index before mode navigation
    if privateMode {
      tabManager.privateTabSelectedIndex =
        Preferences.Privacy.persistentPrivateBrowsing.value ? tabManager.selectedIndex : 0
    } else {
      tabManager.normalTabSelectedIndex = tabManager.selectedIndex
    }

    tabManager.willSwitchTabMode(leavingPBM: privateMode)
    privateMode.toggle()

    updateShredButtonVisibility()

    // When we switch from Private => Regular make sure we reset _selectedIndex, fix for bug #888
    tabManager.resetSelectedIndex()
    if privateMode {
      tabTypeSelector.selectedSegmentIndex = 0
      tabTypeSelector.sendActions(for: UIControl.Event.valueChanged)

      if !Preferences.Privacy.persistentPrivateBrowsing.value
        || tabManager.tabsForCurrentMode.isEmpty
      {
        tabTrayView.showPrivateModeInfo()
        updateShredButtonVisibility()
        // New private tab is created immediately to reflect changes on NTP.
        // If user drags the modal down or dismisses it, a new private tab will be ready.
        tabManager.addTabAndSelect(isPrivate: true)
        navigationController?.setNavigationBarHidden(true, animated: false)
      } else {
        if tabManager.tabsForCurrentMode.isEmpty {
          tabManager.addTabAndSelect(isPrivate: true)
        }

        let privateModeTabSelected =
          tabManager.tabsForCurrentMode[safe: tabManager.privateTabSelectedIndex]
          ?? tabManager.tabsForCurrentMode.last

        if Preferences.Privacy.persistentPrivateBrowsing.value {
          tabManager.selectTab(privateModeTabSelected)
        }

        tabTrayView.hidePrivateModeInfo()
        updateShredButtonVisibility()
        tabTrayView.collectionView.reloadData()

        // When you go back from normal mode, current tab should be selected
        // in case private tabs are persistent
        if Preferences.Privacy.persistentPrivateBrowsing.value {
          scrollToSelectedTab(privateModeTabSelected)
        }
        navigationController?.setNavigationBarHidden(false, animated: false)
      }
    } else {
      tabTrayView.hidePrivateModeInfo()

      // When you go back from private mode, a previous current tab is selected
      // Reloding the collection view in order to mark the selecte the tab
      let normalModeTabSelected =
        tabManager.tabsForCurrentMode[safe: tabManager.normalTabSelectedIndex]
        ?? tabManager.tabsForCurrentMode.last

      tabManager.selectTab(normalModeTabSelected)
      tabTrayView.collectionView.reloadData()

      scrollToSelectedTab(normalModeTabSelected)
      navigationController?.setNavigationBarHidden(false, animated: false)
    }

    tabTypeSelector.isHidden = privateMode
    tabTypeSelectorContainerView.isHidden =
      privateMode && !BraveCore.FeatureList.kBraveShredFeature.enabled
  }

  func remove(tab: Tab) {
    // Initially add the tab to recently closed and remove it from Tab Data after
    tabManager.addTabToRecentlyClosed(tab)
    tabManager.removeTab(tab)

    let query = isTabTrayBeingSearched ? tabTraySearchQuery : nil
    applySnapshot(for: query)
  }

  func removeAllTabs() {
    tabManager.removeTabsWithUndoToast(tabManager.tabsForCurrentMode)
    applySnapshot()
  }

  private func presentSyncSettings(status: SyncStatusState) {
    switch status {
    case .noSyncChain:
      openInsideSettingsNavigation(
        with: SyncWelcomeViewController(
          syncAPI: braveCore.syncAPI,
          syncProfileServices: braveCore.syncProfileService,
          tabManager: tabManager,
          windowProtection: windowProtection,
          isModallyPresented: true
        )
      )
    case .openTabsDisabled, .noSyncedSessions:
      if !DeviceInfo.hasConnectivity() {
        present(SyncAlerts.noConnection, animated: true)
        return
      }

      let syncSettingsScreen = SyncSettingsTableViewController(
        isModallyPresented: true,
        syncAPI: braveCore.syncAPI,
        syncProfileService: braveCore.syncProfileService,
        tabManager: tabManager,
        windowProtection: windowProtection
      )

      syncSettingsScreen.syncStatusDelegate = self

      openInsideSettingsNavigation(with: syncSettingsScreen)
    default:
      return
    }
  }

  func openInsideSettingsNavigation(with viewController: UIViewController) {
    let settingsNavigationController = SettingsNavigationController(
      rootViewController: viewController
    ).then {
      $0.popoverDelegate = self
      $0.isModalInPresentation = true
      $0.modalPresentationStyle =
        UIDevice.current.userInterfaceIdiom == .phone ? .pageSheet : .formSheet
    }

    settingsNavigationController.navigationBar.topItem?.leftBarButtonItem =
      UIBarButtonItem(
        barButtonSystemItem: .done,
        target: settingsNavigationController,
        action: #selector(settingsNavigationController.done)
      )

    DeviceOrientation.shared.changeOrientationToPortraitOnPhone()

    present(settingsNavigationController, animated: true)
  }

  private func scrollToSelectedTab(_ tab: Tab?) {
    if let selectedTab = tab,
      let selectedIndexPath = dataSource.indexPath(for: selectedTab)
    {
      DispatchQueue.main.async {
        self.tabTrayView.collectionView.scrollToItem(
          at: selectedIndexPath,
          at: .centeredVertically,
          animated: false
        )
      }
    }
  }

  @objc private func tappedCollectionViewBackground() {
    if traitCollection.horizontalSizeClass == .compact {
      doneAction()
    }
  }
}

// MARK: PresentingModalViewControllerDelegate

extension TabTrayController: PresentingModalViewControllerDelegate {

  func dismissPresentedModalViewController(_ modalViewController: UIViewController, animated: Bool)
  {
    modalViewController.dismiss(animated: true) { [weak self] in
      self?.reloadOpenTabsSession()
    }
  }
}

// MARK: TabManagerDelegate

extension TabTrayController: TabManagerDelegate {
  func tabManager(_ tabManager: TabManager, didAddTab tab: Tab) {
    updateShredButtonVisibility()
    applySnapshot()

    // This check is mainly for entering private mode.
    // Then a first private tab is created, and we want to show an informational screen
    // instead of taking the user directly to new tab page.
    if tabManager.tabsForCurrentMode.count > 1 {
      DispatchQueue.main.asyncAfter(deadline: .now() + 0.2) {
        self.dismiss(animated: true)
      }
    }
  }

  func tabManager(_ tabManager: TabManager, didRemoveTab tab: Tab) {
    // When user removes their last tab, a new one is created.
    // Until then, the view is dismissed and takes the user directly to that tab.
    if tabManager.tabsForCurrentMode.count < 1 {
      dismiss(animated: true)
    }
  }

  func tabManager(_ tabManager: TabManager, didSelectedTabChange selected: Tab?, previous: Tab?) {}
  func tabManager(_ tabManager: TabManager, willAddTab tab: Tab) {}
  func tabManager(_ tabManager: TabManager, willRemoveTab tab: Tab) {}
  func tabManagerDidAddTabs(_ tabManager: TabManager) {}
  func tabManagerDidRestoreTabs(_ tabManager: TabManager) {}
  func tabManagerDidRemoveAllTabs(_ tabManager: TabManager, toast: ButtonToast?) {}
}

// MARK: UIScrollViewAccessibilityDelegate

extension TabTrayController: UIScrollViewAccessibilityDelegate {
  func accessibilityScrollStatus(for scrollView: UIScrollView) -> String? {
    let collectionView = tabTrayView.collectionView

    guard var visibleCells = collectionView.visibleCells as? [TabCell] else { return nil }
    var bounds = collectionView.bounds
    bounds = bounds.offsetBy(
      dx: collectionView.contentInset.left,
      dy: collectionView.contentInset.top
    )
    bounds.size.width -= collectionView.contentInset.left + collectionView.contentInset.right
    bounds.size.height -= collectionView.contentInset.top + collectionView.contentInset.bottom
    // visible cells do sometimes return also not visible cells when attempting to go past the last cell with VoiceOver right-flick gesture; so make sure we have only visible cells (yeah...)
    visibleCells = visibleCells.filter { !$0.frame.intersection(bounds).isEmpty }

    let cells = visibleCells.compactMap { collectionView.indexPath(for: $0) }
    let indexPaths = cells.sorted { (a: IndexPath, b: IndexPath) -> Bool in
      return a.section < b.section || (a.section == b.section && a.row < b.row)
    }

    if indexPaths.isEmpty {
      return Strings.tabTrayEmptyVoiceOverText
    }

    guard let firstTab = indexPaths.first, let lastTab = indexPaths.last else {
      return nil
    }

    let firstTabRow = firstTab.row + 1
    let lastTabRow = lastTab.row + 1
    let tabCount = collectionView.numberOfItems(inSection: 0)

    if firstTabRow == lastTabRow {
      return String(
        format: Strings.tabTraySingleTabPositionFormatVoiceOverText,
        NSNumber(value: firstTabRow),
        NSNumber(value: tabCount)
      )
    } else {
      return String(
        format: Strings.tabTrayMultiTabPositionFormatVoiceOverText,
        NSNumber(value: firstTabRow as Int),
        NSNumber(value: lastTabRow),
        NSNumber(value: tabCount)
      )
    }
  }
}

extension TabTrayController: SyncStatusDelegate {
  func syncStatusChanged() {
    tabSyncView.updateSyncStatusPanel(for: emptyPanelState)
  }
}
