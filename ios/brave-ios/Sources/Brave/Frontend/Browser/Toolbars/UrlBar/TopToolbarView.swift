// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveUI
import BraveWidgetsModels
import Combine
import Data
import DesignSystem
import OrderedCollections
import Preferences
import Shared
import SnapKit
import SpeechRecognition
import UIKit
import Web

protocol TopToolbarDelegate: AnyObject {
  func topToolbarDidPressTabs(_ topToolbar: TopToolbarView)
  func topToolbarDidPressReaderMode(_ topToolbar: TopToolbarView)
  func topToolbarDidPressPlaylistButton(_ urlBar: TopToolbarView)
  func topToolbarDidPressPlaylistMenuAction(
    _ urlBar: TopToolbarView,
    action: PlaylistURLBarButton.MenuAction
  )
  func topToolbarDidPressTranslateButton(_ urlBar: TopToolbarView)
  /// The user tapped the location bar to begin editing the URL. The receiver should present the
  /// search input rather than mutating the toolbar.
  func topToolbarDidRequestSearchInput(
    _ topToolbar: TopToolbarView,
    initialText: String?,
    pasted: Bool,
    search: Bool
  )
  func topToolbarDidPressScrollToTop(_ topToolbar: TopToolbarView)
  // Returns either (search query, true) or (url, false).
  func topToolbarDisplayTextForURL(_ url: URL?) -> (String?, Bool)
  func topToolbarDidBeginDragInteraction(_ topToolbar: TopToolbarView)
  func topToolbarAvailableShortcutButtons(
    _ topToolbar: TopToolbarView
  ) -> OrderedSet<WidgetShortcut>
  func topToolbarDidTapShortcutButton(_ topToolbar: TopToolbarView)
  func topToolbarDidTapBraveShieldsButton(_ topToolbar: TopToolbarView)
  func topToolbarDidTapBraveRewardsButton(_ topToolbar: TopToolbarView)
  func topToolbarDidTapMenuButton(_ topToolbar: TopToolbarView)
  func topToolbarDidPressVoiceSearchButton(_ urlBar: TopToolbarView)
  func topToolbarDidPressPasteAndGoButton(_ urlBar: TopToolbarView)
  func topToolbarDidPressStop(_ urlBar: TopToolbarView)
  func topToolbarDidPressReload(_ urlBar: TopToolbarView)
  func topToolbarDidPressQrCodeButton(_ urlBar: TopToolbarView)
  func topToolbarDidTapWalletButton(_ urlBar: TopToolbarView)
  func topToolbarDidTapSecureContentState(_ urlBar: TopToolbarView)
  func topToolbarIsShieldsEnabled(_ topToolbar: TopToolbarView, for url: URL?) -> Bool
}

class TopToolbarView: UIView, ToolbarProtocol {

  // MARK: UX

  struct UX {
    static let locationPadding: CGFloat = 8
    static let locationHeight: CGFloat = 44
    static let textFieldCornerRadius: CGFloat = 10
  }

  // MARK: URLBarButton

  enum URLBarButton {
    case wallet
    case playlist
    case translate
  }

  // MARK: Internal

  var helper: ToolbarHelper?

  weak var delegate: TopToolbarDelegate? {
    didSet {
      guard delegate !== oldValue else { return }
      updateViewsForToolbarChanges()
    }
  }
  weak var tabToolbarDelegate: ToolbarDelegate?

  private var cancellables: Set<AnyCancellable> = []
  private let privateBrowsingManager: PrivateBrowsingManager

  private(set) var displayTabTraySwipeGestureRecognizer: UISwipeGestureRecognizer?

  // MARK: State

  private var isTransitioning: Bool = false {
    didSet {
      if isTransitioning {
        // Cancel any pending/in-progress animations related to the progress bar
        locationView.progressBar.setProgress(1, animated: false)
        locationView.progressBar.alpha = 0.0
      }
    }
  }

  private var toolbarIsShowing = false

  var currentURL: URL? {
    get { return locationView.url as URL? }

    set(newURL) {
      locationView.url = newURL
      refreshShieldsStatus()
    }
  }

  var secureContentState: SecureContentState {
    get { return locationView.secureContentState }
    set { locationView.secureContentState = newValue }
  }

  var isURLBarEnabled = true {
    didSet {
      if oldValue == isURLBarEnabled { return }

      locationView.isUserInteractionEnabled = isURLBarEnabled
    }
  }

  // MARK: Views

  lazy var locationView = TabLocationView(
    speechRecognizer: speechRecognizer,
    privateBrowsingManager: privateBrowsingManager
  ).then {
    $0.translatesAutoresizingMaskIntoConstraints = false
    $0.readerModeState = .unavailable
    $0.translationState = .unavailable
    $0.delegate = self
    $0.layer.cornerRadius = UX.textFieldCornerRadius
    $0.layer.cornerCurve = .continuous
    $0.clipsToBounds = true
    $0.setContentCompressionResistancePriority(.required, for: .vertical)
  }

  let tabsButton = TabsButton()

  private lazy var scrollToTopButton = UIButton().then {
    $0.addTarget(self, action: #selector(tappedScrollToTopArea), for: .touchUpInside)
  }

  private lazy var shortcutButton = ToolbarButton().then {
    $0.addTarget(self, action: #selector(didClickShortcutButton), for: .touchUpInside)
    $0.contentEdgeInsets = .init(top: 4, left: 4, bottom: 4, right: 4)
    $0.snp.makeConstraints {
      $0.size.greaterThanOrEqualTo(32)
    }
    $0.menu = .init(children: [
      UIDeferredMenuElement.uncached { [weak self] handler in
        guard let self, let availableShortcuts = delegate?.topToolbarAvailableShortcutButtons(self)
        else {
          handler([])
          return
        }
        let options = UIMenu(
          options: [.singleSelection, .displayInline],
          children: availableShortcuts.map { shortcut in
            UIAction(
              title: shortcut.displayString,
              image: shortcut.image,
              state: shortcut.rawValue == Preferences.General.toolbarShortcutButton.value
                ? .on : .off,
              handler: { _ in
                Preferences.General.toolbarShortcutButton.value = shortcut.rawValue
              }
            )
          }
        )
        handler([options])
      },
      UIMenu(
        options: .displayInline,
        children: [
          UIAction(
            title: Strings.ShortcutButton.hideButtonTitle,
            image: UIImage(braveSystemNamed: "leo.eye.off"),
            attributes: .destructive,
            handler: { _ in
              Preferences.General.toolbarShortcutButton.value = nil
            }
          )
        ]
      ),
    ])
  }

  var forwardButton = ToolbarButton()
  var shareButton = ToolbarButton()
  var addTabButton = ToolbarButton()
  var searchButton = ToolbarButton()
  lazy var menuButton = MenuButton().then {
    $0.accessibilityIdentifier = "topToolbarView-menuButton"
  }

  var backButton: ToolbarButton = {
    let backButton = ToolbarButton()
    backButton.accessibilityIdentifier = "TopToolbarView.backButton"
    return backButton
  }()

  lazy var actionButtons: [UIButton] = [
    shareButton, tabsButton, shortcutButton,
    forwardButton, backButton, menuButton,
    shieldsButton, rewardsButton,
  ].compactMap { $0 }

  private let mainStackView = UIStackView().then {
    $0.spacing = 8
    $0.isLayoutMarginsRelativeArrangement = true
    $0.insetsLayoutMarginsFromSafeArea = false
    $0.alignment = .center
  }

  private let leadingItemsStackView = UIStackView().then {
    $0.distribution = .fillEqually
    $0.translatesAutoresizingMaskIntoConstraints = false
    $0.spacing = 8
  }

  private let trailingItemsStackView = UIStackView().then {
    $0.distribution = .fillEqually
    $0.spacing = 8
  }

  private let shieldsRewardsStack = UIStackView().then {
    $0.distribution = .fillEqually
    $0.spacing = 8
    $0.setContentHuggingPriority(.required, for: .horizontal)
  }

  /// The currently visible URL bar button beside the refresh button.
  private(set) var currentURLBarButton: URLBarButton? {
    didSet {
      locationView.walletButton.isHidden = currentURLBarButton != .wallet
      locationView.playlistButton.isHidden = currentURLBarButton != .playlist
    }
  }

  private(set) lazy var shieldsButton: ToolbarButton = {
    let button = ToolbarButton()
    button.setImage(UIImage(sharedNamed: "brave.logo"), for: .normal)
    button.addTarget(self, action: #selector(didTapBraveShieldsButton), for: .touchUpInside)
    button.imageView?.contentMode = .scaleAspectFit
    button.accessibilityLabel = Strings.bravePanel
    button.imageView?.adjustsImageSizeForAccessibilityContentSizeCategory = true
    button.accessibilityIdentifier = "urlBar-shieldsButton"
    button.contentHorizontalAlignment = .fill
    button.contentVerticalAlignment = .fill
    return button
  }()

  private(set) lazy var rewardsButton: RewardsButton = {
    let button = RewardsButton()
    button.addTarget(self, action: #selector(didTapBraveRewardsButton), for: .touchUpInside)
    return button
  }()

  lazy var locationContainer = LocationContainerView().then {
    $0.translatesAutoresizingMaskIntoConstraints = false
  }

  private var speechRecognizer: SpeechRecognizer

  // MARK: Lifecycle

  init(speechRecognizer: SpeechRecognizer, privateBrowsingManager: PrivateBrowsingManager) {
    self.speechRecognizer = speechRecognizer
    self.privateBrowsingManager = privateBrowsingManager

    super.init(frame: .zero)

    locationContainer.contentView.addSubview(locationView)

    [scrollToTopButton, tabsButton].forEach(addSubview(_:))
    addSubview(mainStackView)

    helper = ToolbarHelper(toolbar: self)

    // Buttons won't take unnecessary space and won't shrink
    actionButtons.forEach {
      $0.setContentCompressionResistancePriority(.defaultHigh, for: .horizontal)
      $0.setContentHuggingPriority(.defaultHigh, for: .horizontal)
    }

    // Url bar will expand while keeping space for other items on the address bar.
    locationContainer.setContentHuggingPriority(.defaultLow, for: .horizontal)
    locationContainer.setContentHuggingPriority(.required, for: .vertical)

    leadingItemsStackView.addArrangedSubview(backButton)
    leadingItemsStackView.addArrangedSubview(forwardButton)
    leadingItemsStackView.addArrangedSubview(shortcutButton)
    leadingItemsStackView.addArrangedSubview(shareButton)

    [backButton, forwardButton].forEach {
      $0.contentEdgeInsets = UIEdgeInsets(
        top: 0,
        left: UX.locationPadding,
        bottom: 0,
        right: UX.locationPadding
      )
    }

    if UIDevice.current.userInterfaceIdiom == .phone {
      trailingItemsStackView.addArrangedSubview(addTabButton)
    }
    trailingItemsStackView.addArrangedSubview(tabsButton)
    trailingItemsStackView.addArrangedSubview(menuButton)

    shieldsRewardsStack.addArrangedSubview(shieldsButton)
    shieldsRewardsStack.addArrangedSubview(rewardsButton)

    [
      leadingItemsStackView, locationContainer, shieldsRewardsStack, trailingItemsStackView,
    ].forEach {
      mainStackView.addArrangedSubview($0)
    }

    setupConstraints()

    Preferences.General.toolbarShortcutButton.observe(from: self)

    updateViewsForToolbarChanges()

    privateBrowsingManager
      .$isPrivateBrowsing
      .removeDuplicates()
      .receive(on: RunLoop.main)
      .sink(receiveValue: { [weak self] _ in
        guard let self = self else { return }
        self.updateColors()
        self.helper?.updateForTraitCollection(
          self.traitCollection,
          browserColors: privateBrowsingManager.browserColors,
          isBottomToolbar: false
        )
      })
      .store(in: &cancellables)

    updateURLBarButtonsVisibility()
    helper?.updateForTraitCollection(
      traitCollection,
      browserColors: privateBrowsingManager.browserColors,
      isBottomToolbar: false,
      additionalButtons: [shortcutButton]
    )
    updateForTraitCollection()

    let swipeGestureRecognizer = UISwipeGestureRecognizer(
      target: self,
      action: #selector(swipedLocationView)
    )
    swipeGestureRecognizer.direction = .up
    swipeGestureRecognizer.isEnabled = false
    locationView.addGestureRecognizer(swipeGestureRecognizer)

    let dragInteraction = UIDragInteraction(delegate: self)
    dragInteraction.allowsSimultaneousRecognitionDuringLift = true
    locationView.addInteraction(dragInteraction)

    self.displayTabTraySwipeGestureRecognizer = swipeGestureRecognizer

    updateColors()
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }

  override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
    super.traitCollectionDidChange(previousTraitCollection)
    helper?.updateForTraitCollection(
      traitCollection,
      browserColors: privateBrowsingManager.browserColors,
      isBottomToolbar: false,
      additionalButtons: [shortcutButton]
    )
    updateForTraitCollection()
  }

  private func updateForTraitCollection() {
    let toolbarSizeCategory = traitCollection.toolbarButtonContentSizeCategory
    let pointSize = UIFontMetrics(forTextStyle: .body).scaledValue(
      for: 24,
      compatibleWith: .init(preferredContentSizeCategory: toolbarSizeCategory)
    )
    shieldsButton.snp.remakeConstraints {
      $0.size.equalTo(pointSize)
    }
    rewardsButton.snp.remakeConstraints {
      $0.size.equalTo(pointSize)
    }
  }

  private func setupConstraints() {
    locationContainer.snp.remakeConstraints {
      $0.top.bottom.equalToSuperview().inset(UX.locationPadding)
      $0.height.greaterThanOrEqualTo(UX.locationHeight)
    }

    mainStackView.snp.remakeConstraints { make in
      make.top.bottom.equalTo(self)
      if #available(iOS 26.0, *) {
        make.leading.trailing.equalTo(layoutGuide(for: .safeArea(cornerAdaptation: .horizontal)))
      } else {
        make.leading.trailing.equalTo(safeAreaLayoutGuide)
      }
    }

    scrollToTopButton.snp.makeConstraints { make in
      make.top.equalTo(self)
      make.left.right.equalTo(self.locationContainer)
    }

    locationView.snp.makeConstraints { make in
      make.edges.equalTo(self.locationContainer)
      make.height.greaterThanOrEqualTo(UX.locationHeight)
    }
  }

  override func layoutSubviews() {
    super.layoutSubviews()
    // Increase the inset of the main stack view if there's no additional space from safe areas
    let horizontalInset: CGFloat = safeAreaInsets.left > 0 ? 0 : 12
    mainStackView.layoutMargins = .init(
      top: 0,
      left: horizontalInset,
      bottom: 0,
      right: horizontalInset
    )
  }

  private func updateColors() {
    overrideUserInterfaceStyle = privateBrowsingManager.isPrivateBrowsing ? .dark : .unspecified
    let browserColors = privateBrowsingManager.browserColors
    backgroundColor = browserColors.chromeBackground
    locationContainer.contentView.backgroundColor = browserColors.containerBackground
  }

  // Ideally we'd split this implementation in two, one TopToolbarView with a toolbar and one without
  // However, switching views dynamically at runtime is a difficult. For now, we just use one view
  // that can show in either mode.
  func setShowToolbar(_ shouldShow: Bool) {
    toolbarIsShowing = shouldShow
    setNeedsUpdateConstraints()
    // when we transition from portrait to landscape, calling this here causes
    // the constraints to be calculated too early and there are constraint errors
    if !toolbarIsShowing {
      updateConstraintsIfNeeded()
    }
    updateViewsForToolbarChanges()
  }

  func currentProgress() -> Float {
    locationView.progressBar.progress
  }

  func updateProgressBar(_ progress: Float) {
    locationView.progressBar.alpha = 1
    locationView.progressBar.isHidden = false
    locationView.progressBar.setProgress(progress, animated: !isTransitioning)
  }

  func hideProgressBar() {
    locationView.progressBar.isHidden = true
    locationView.progressBar.setProgress(0, animated: false)
  }

  func updateReaderModeState(_ state: ReaderModeState) {
    locationView.readerModeState = state
    updateURLBarButtonsVisibility()
  }

  func updatePlaylistButtonState(_ state: PlaylistURLBarButton.State) {
    locationView.playlistButton.buttonState = state
    updateURLBarButtonsVisibility()
  }

  func updateTranslateButtonState(_ state: TranslationState) {
    locationView.translationState = state
    updateURLBarButtonsVisibility()
  }

  func updateWalletButtonState(_ state: WalletURLBarButton.ButtonState) {
    locationView.walletButton.buttonState = state
    updateURLBarButtonsVisibility()
  }

  /// Updates the `currentURLBarButton` based on priority: 1) Wallet 2) Playlist 3) ReaderMode.
  private func updateURLBarButtonsVisibility() {
    if locationView.walletButton.buttonState != .inactive {
      currentURLBarButton = .wallet
    } else if locationView.playlistButton.buttonState != .none {
      currentURLBarButton = .playlist
    } else {
      currentURLBarButton = nil
    }
  }

  func updateViewsForToolbarChanges() {
    backButton.stackViewAnimationSafeIsHidden = toolbarIsShowing
    forwardButton.stackViewAnimationSafeIsHidden = toolbarIsShowing
    shareButton.stackViewAnimationSafeIsHidden = toolbarIsShowing
    trailingItemsStackView.stackViewAnimationSafeIsHidden = toolbarIsShowing

    let selectedShortcut: WidgetShortcut? = {
      let shortcut = Preferences.General.toolbarShortcutButton.value.flatMap(WidgetShortcut.init)
      if let delegate, let shortcut,
        !delegate.topToolbarAvailableShortcutButtons(self).contains(shortcut)
      {
        return nil
      }
      return shortcut
    }()
    shortcutButton.stackViewAnimationSafeIsHidden = selectedShortcut == nil
    if let selectedShortcut {
      shortcutButton.setImage(selectedShortcut.image, for: .normal)
      shortcutButton.accessibilityLabel = selectedShortcut.displayString
    }
    leadingItemsStackView.stackViewAnimationSafeIsHidden = leadingItemsStackView.arrangedSubviews
      .allSatisfy(\.isHidden)
  }

  /// Update the shields icon based on whether or not shields are enabled for this site
  func refreshShieldsStatus() {
    // Default on
    var shieldIcon = "brave.logo"
    let shieldsOffIcon = "brave.logo.greyscale"
    if let currentURL = currentURL, currentURL.isWebPage(includeDataURIs: false) {
      let isShieldsEnabled =
        delegate?.topToolbarIsShieldsEnabled(self, for: currentURL) ?? true
      if !isShieldsEnabled {
        shieldIcon = shieldsOffIcon
      }
      if currentURL.isLocal || currentURL.isNewTabURL {
        shieldIcon = shieldsOffIcon
      }
    } else {
      shieldIcon = shieldsOffIcon
    }

    shieldsButton.setImage(UIImage(sharedNamed: shieldIcon), for: .normal)
  }

  // MARK: Actions

  @objc func tappedScrollToTopArea() {
    delegate?.topToolbarDidPressScrollToTop(self)
  }

  @objc func didClickShortcutButton() {
    delegate?.topToolbarDidTapShortcutButton(self)
  }

  @objc func didClickMenu() {
    delegate?.topToolbarDidTapMenuButton(self)
  }

  @objc private func swipedLocationView() {
    delegate?.topToolbarDidPressTabs(self)
  }

  @objc private func didTapBraveShieldsButton() {
    delegate?.topToolbarDidTapBraveShieldsButton(self)
  }

  @objc private func didTapBraveRewardsButton() {
    delegate?.topToolbarDidTapBraveRewardsButton(self)
  }
}

// MARK:  PreferencesObserver

extension TopToolbarView: PreferencesObserver {
  func preferencesDidChange(for key: String) {
    updateViewsForToolbarChanges()
  }
}

// MARK:  TabLocationViewDelegate

extension TopToolbarView: TabLocationViewDelegate {
  func tabLocationViewDidTapLocation(_ tabLocationView: TabLocationView) {
    guard
      isURLBarEnabled,
      let (locationText, isSearchQuery) = delegate?.topToolbarDisplayTextForURL(
        locationView.url as URL?
      )
    else { return }

    var overlayText = locationText
    // Make sure to use the result from topToolbarDisplayTextForURL as it is responsible for extracting out search terms when on a search page
    if let text = locationText, let url = NSURL(idnString: text) as? URL {
      // When the user is entering text into the URL bar, we must show the entire URL, omitting NOTHING (not even the scheme, or www), and un-escaping NOTHING!
      overlayText = URLFormatter.formatURL(url.absoluteString, formatTypes: [], unescapeOptions: [])
    }
    delegate?.topToolbarDidRequestSearchInput(
      self,
      initialText: overlayText,
      pasted: false,
      search: isSearchQuery
    )
  }

  func tabLocationViewDidTapReload(_ tabLocationView: TabLocationView) {
    delegate?.topToolbarDidPressReload(self)
  }

  func tabLocationViewDidTapStop(_ tabLocationView: TabLocationView) {
    delegate?.topToolbarDidPressStop(self)
  }

  func tabLocationViewDidTapVoiceSearch(_ tabLocationView: TabLocationView) {
    delegate?.topToolbarDidPressVoiceSearchButton(self)
  }

  func tabLocationViewDidTapReaderMode(_ tabLocationView: TabLocationView) {
    delegate?.topToolbarDidPressReaderMode(self)
  }

  func tabLocationViewDidTapPlaylist(_ tabLocationView: TabLocationView) {
    delegate?.topToolbarDidPressPlaylistButton(self)
  }

  func tabLocationViewDidTapPlaylistMenuAction(
    _ tabLocationView: TabLocationView,
    action: PlaylistURLBarButton.MenuAction
  ) {
    delegate?.topToolbarDidPressPlaylistMenuAction(self, action: action)
  }

  func tabLocationViewDidTapTranslateButton(_ tabLocationView: TabLocationView) {
    delegate?.topToolbarDidPressTranslateButton(self)
  }

  func tabLocationViewDidBeginDragInteraction(_ tabLocationView: TabLocationView) {
    delegate?.topToolbarDidBeginDragInteraction(self)
  }

  func tabLocationViewDidTapWalletButton(_ urlBar: TabLocationView) {
    delegate?.topToolbarDidTapWalletButton(self)
  }

  func tabLocationViewDidTapSecureContentState(_ urlBar: TabLocationView) {
    delegate?.topToolbarDidTapSecureContentState(self)
  }
}

extension TopToolbarView: UIDragInteractionDelegate {
  func dragInteraction(
    _ interaction: UIDragInteraction,
    itemsForBeginning session: UIDragSession
  ) -> [UIDragItem] {
    // Ensure we actually have a URL in the location bar and that the URL is not local.
    guard let url = self.locationView.url, !InternalURL.isValid(url: url),
      let itemProvider = NSItemProvider(contentsOf: url),
      !locationView.reloadButton.isHighlighted
    else {
      return []
    }

    let dragItem = UIDragItem(itemProvider: itemProvider)
    return [dragItem]
  }

  func dragInteraction(_ interaction: UIDragInteraction, sessionWillBegin session: UIDragSession) {
    delegate?.topToolbarDidBeginDragInteraction(self)
  }
}

extension UIView {
  // UIStackView bug:
  // Don't set `isHidden` to the same value on a view that adjusts layout of a UIStackView
  // inside of a UIView.animate() block, otherwise on occasion the view will render but
  // `isHidden` will still be true
  fileprivate var stackViewAnimationSafeIsHidden: Bool {
    get { isHidden }
    set {
      if isHidden != newValue {
        isHidden = newValue
      }
    }
  }
}
