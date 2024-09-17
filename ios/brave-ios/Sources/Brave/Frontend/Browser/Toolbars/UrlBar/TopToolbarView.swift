// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveUI
import BraveWidgetsModels
import Combine
import Data
import DesignSystem
import Preferences
import Shared
import SnapKit
import UIKit

protocol TopToolbarDelegate: AnyObject {
  func topToolbarDidPressTabs(_ topToolbar: TopToolbarView)
  func topToolbarDidPressReaderMode(_ topToolbar: TopToolbarView)
  func topToolbarDidPressPlaylistButton(_ urlBar: TopToolbarView)
  func topToolbarDidPressPlaylistMenuAction(
    _ urlBar: TopToolbarView,
    action: PlaylistURLBarButton.MenuAction
  )
  func topToolbarDidEnterOverlayMode(_ topToolbar: TopToolbarView)
  func topToolbarDidLeaveOverlayMode(_ topToolbar: TopToolbarView)
  func topToolbarDidPressScrollToTop(_ topToolbar: TopToolbarView)
  func topToolbar(_ topToolbar: TopToolbarView, didEnterText text: String)
  func topToolbar(_ topToolbar: TopToolbarView, didSubmitText text: String)
  // Returns either (search query, true) or (url, false).
  func topToolbarDisplayTextForURL(_ url: URL?) -> (String?, Bool)
  func topToolbarDidBeginDragInteraction(_ topToolbar: TopToolbarView)
  func topToolbarDidTapShortcutButton(_ topToolbar: TopToolbarView)
  func topToolbarDidTapBraveShieldsButton(_ topToolbar: TopToolbarView)
  func topToolbarDidTapBraveRewardsButton(_ topToolbar: TopToolbarView)
  func topToolbarDidTapMenuButton(_ topToolbar: TopToolbarView)
  func topToolbarDidPressVoiceSearchButton(_ urlBar: TopToolbarView)
  func topToolbarDidPressStop(_ urlBar: TopToolbarView)
  func topToolbarDidPressReload(_ urlBar: TopToolbarView)
  func topToolbarDidPressQrCodeButton(_ urlBar: TopToolbarView)
  func topToolbarDidTapWalletButton(_ urlBar: TopToolbarView)
  func topToolbarDidTapSecureContentState(_ urlBar: TopToolbarView)
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
  }

  // MARK: Internal

  var helper: ToolbarHelper?

  weak var delegate: TopToolbarDelegate?
  weak var tabToolbarDelegate: ToolbarDelegate?

  private var cancellables: Set<AnyCancellable> = []
  private var privateModeCancellable: AnyCancellable?
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

  /// Overlay mode is the state where the lock/reader icons are hidden, the home panels are shown,
  /// and the Cancel button is visible (allowing the user to leave overlay mode). Overlay mode
  /// is *not* tied to the location text field's editing state; for instance, when selecting
  /// a panel, the first responder will be resigned, yet the overlay mode UI is still active.
  var inOverlayMode = false

  var currentURL: URL? {
    get { return locationView.url as URL? }

    set(newURL) {
      locationView.url = newURL
      refreshShieldsStatus()
    }
  }

  var secureContentState: TabSecureContentState {
    get { return locationView.secureContentState }
    set { locationView.secureContentState = newValue }
  }

  var isURLBarEnabled = true {
    didSet {
      if oldValue == isURLBarEnabled { return }

      locationTextField?.isUserInteractionEnabled = isURLBarEnabled
    }
  }

  var locationLastReplacement: String {
    locationTextField?.lastReplacement ?? ""
  }

  // MARK: Views

  private var locationTextField: AutocompleteTextField?

  lazy var locationView = TabLocationView(
    voiceSearchSupported: isVoiceSearchAvailable,
    privateBrowsingManager: privateBrowsingManager
  ).then {
    $0.translatesAutoresizingMaskIntoConstraints = false
    $0.readerModeState = ReaderModeState.unavailable
    $0.delegate = self
    $0.layer.cornerRadius = UX.textFieldCornerRadius
    $0.layer.cornerCurve = .continuous
    $0.clipsToBounds = true
    $0.setContentCompressionResistancePriority(.required, for: .vertical)
  }

  let tabsButton = TabsButton()

  private lazy var cancelButton = InsetButton().then {
    $0.setTitle(Strings.cancelButtonTitle, for: .normal)
    $0.setTitleColor(UIColor.secondaryBraveLabel, for: .normal)
    $0.accessibilityIdentifier = "topToolbarView-cancel"
    $0.addTarget(self, action: #selector(didClickCancel), for: .touchUpInside)
    $0.setContentCompressionResistancePriority(.required, for: .horizontal)
    $0.setContentHuggingPriority(.defaultHigh, for: .horizontal)
  }

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
      UIMenu(
        options: [.singleSelection, .displayInline],
        children: WidgetShortcut.eligibleButtonShortcuts.map { shortcut in
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
      ),
      UIMenu(
        options: .displayInline,
        children: [
          UIAction(
            title: "Hide Shortcut Button",
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

  private var locationTextContentView: UIStackView?

  private lazy var qrCodeButton = ToolbarButton().then {
    $0.accessibilityIdentifier = "TabToolbar.qrCodeButton"
    $0.isAccessibilityElement = true
    $0.accessibilityLabel = Strings.quickActionScanQRCode
    $0.setImage(UIImage(braveSystemNamed: "leo.qr.code", compatibleWith: nil), for: .normal)
    $0.tintColor = .braveLabel
    $0.contentEdgeInsets = UIEdgeInsets(top: 0, left: 5, bottom: 0, right: 5)
    $0.setContentCompressionResistancePriority(.required, for: .horizontal)
    $0.setContentCompressionResistancePriority(.defaultHigh, for: .vertical)
    $0.setContentHuggingPriority(.defaultHigh, for: .horizontal)
    $0.setContentHuggingPriority(.defaultHigh, for: .vertical)
  }

  private lazy var voiceSearchButton = ToolbarButton().then {
    $0.accessibilityIdentifier = "TabToolbar.voiceSearchButton"
    $0.isAccessibilityElement = true
    $0.accessibilityLabel = Strings.tabToolbarVoiceSearchButtonAccessibilityLabel
    $0.setImage(UIImage(braveSystemNamed: "leo.microphone", compatibleWith: nil), for: .normal)
    $0.tintColor = .braveLabel
    $0.isHidden = !isVoiceSearchAvailable
    $0.contentEdgeInsets = UIEdgeInsets(top: 0, left: 5, bottom: 0, right: 5)
    $0.setContentCompressionResistancePriority(.required, for: .horizontal)
    $0.setContentCompressionResistancePriority(.defaultHigh, for: .vertical)
    $0.setContentHuggingPriority(.defaultHigh, for: .horizontal)
    $0.setContentHuggingPriority(.defaultHigh, for: .vertical)
  }

  private(set) lazy var shieldsButton: ToolbarButton = {
    let button = ToolbarButton()
    button.setImage(UIImage(sharedNamed: "brave.logo"), for: .normal)
    button.addTarget(self, action: #selector(didTapBraveShieldsButton), for: .touchUpInside)
    button.imageView?.contentMode = .scaleAspectFit
    button.accessibilityLabel = Strings.bravePanel
    button.imageView?.adjustsImageSizeForAccessibilityContentSizeCategory = true
    button.accessibilityIdentifier = "urlBar-shieldsButton"
    button.contentEdgeInsets = .init(top: 4, left: 5, bottom: 4, right: 5)
    return button
  }()

  private(set) lazy var rewardsButton: RewardsButton = {
    let button = RewardsButton()
    button.addTarget(self, action: #selector(didTapBraveRewardsButton), for: .touchUpInside)
    // Visual centering
    button.contentEdgeInsets = .init(top: 4, left: 4, bottom: 6, right: 4)
    return button
  }()

  private lazy var locationBarOptionsStackView = UIStackView().then {
    $0.alignment = .center
    $0.isHidden = true
    $0.layoutMargins = UIEdgeInsets(top: 0, left: 0, bottom: 0, right: 3)
    $0.isLayoutMarginsRelativeArrangement = true
    $0.insetsLayoutMarginsFromSafeArea = false
  }

  lazy var locationContainer = UIView().then {
    $0.translatesAutoresizingMaskIntoConstraints = false
    $0.backgroundColor = .clear
    $0.layer.cornerRadius = UX.textFieldCornerRadius
    $0.layer.cornerCurve = .continuous
    $0.layer.shadowOffset = .init(width: 0, height: 1)
    $0.layer.shadowRadius = 2
    $0.layer.shadowColor = UIColor.black.cgColor
    $0.layer.shadowOpacity = 0.1
  }

  // The location container has a second shadow but we can't apply 2 shadows in UIKit, so adding a second view
  private let secondLocationShadowView = UIView().then {
    $0.backgroundColor = .clear
    $0.layer.shadowOffset = .init(width: 0, height: 4)
    $0.layer.shadowRadius = 16
    $0.layer.shadowOpacity = 0.08
    $0.layer.shadowColor = UIColor.black.cgColor
  }

  private var isVoiceSearchAvailable: Bool

  // MARK: Lifecycle

  init(voiceSearchSupported: Bool, privateBrowsingManager: PrivateBrowsingManager) {
    isVoiceSearchAvailable = voiceSearchSupported
    self.privateBrowsingManager = privateBrowsingManager

    super.init(frame: .zero)

    addSubview(secondLocationShadowView)
    locationContainer.addSubview(locationView)

    [scrollToTopButton, tabsButton, cancelButton].forEach(addSubview(_:))
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
      cancelButton,
    ].forEach {
      mainStackView.addArrangedSubview($0)
    }

    setupConstraints()

    Preferences.General.toolbarShortcutButton.observe(from: self)

    // Make sure we hide any views that shouldn't be showing in non-overlay mode.
    updateViewsForOverlayModeAndToolbarChanges()

    privateModeCancellable = privateBrowsingManager
      .$isPrivateBrowsing
      .removeDuplicates()
      .receive(on: RunLoop.main)
      .sink(receiveValue: { [weak self] _ in
        guard let self = self else { return }
        self.updateColors()
        self.helper?.updateForTraitCollection(
          self.traitCollection,
          browserColors: privateBrowsingManager.browserColors
        )
      })

    updateURLBarButtonsVisibility()
    helper?.updateForTraitCollection(
      traitCollection,
      browserColors: privateBrowsingManager.browserColors,
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

    qrCodeButton.addTarget(
      self,
      action: #selector(topToolbarDidPressQrCodeButton),
      for: .touchUpInside
    )
    voiceSearchButton.addTarget(
      self,
      action: #selector(topToolbarDidPressVoiceSearchButton),
      for: .touchUpInside
    )

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
      additionalButtons: [shortcutButton]
    )
    updateForTraitCollection()
  }

  private func updateForTraitCollection() {
    let toolbarSizeCategory = traitCollection.toolbarButtonContentSizeCategory
    let pointSize = UIFontMetrics(forTextStyle: .body).scaledValue(
      for: 28,
      compatibleWith: .init(preferredContentSizeCategory: toolbarSizeCategory)
    )
    shieldsButton.snp.remakeConstraints {
      $0.size.equalTo(pointSize)
    }
    rewardsButton.snp.remakeConstraints {
      $0.size.equalTo(pointSize)
    }
    let clampedTraitCollection = traitCollection.clampingSizeCategory(maximum: .accessibilityLarge)
    locationTextField?.font = .preferredFont(
      forTextStyle: .body,
      compatibleWith: clampedTraitCollection
    )
  }

  private func setupConstraints() {
    locationContainer.snp.remakeConstraints {
      $0.top.bottom.equalToSuperview().inset(UX.locationPadding)
      $0.height.greaterThanOrEqualTo(UX.locationHeight)
    }

    mainStackView.snp.remakeConstraints { make in
      make.top.bottom.equalTo(self)
      make.leading.trailing.equalTo(self.safeAreaLayoutGuide)
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

    locationContainer.layoutIfNeeded()
    locationContainer.layer.shadowPath =
      UIBezierPath(
        roundedRect: locationContainer.bounds.insetBy(dx: 1, dy: 1),  // -1 spread in Figma
        cornerRadius: locationContainer.layer.cornerRadius
      ).cgPath

    secondLocationShadowView.frame = locationContainer.frame
    secondLocationShadowView.layer.shadowPath =
      UIBezierPath(
        roundedRect: secondLocationShadowView.bounds.insetBy(dx: 2, dy: 2),  // -2 spread in Figma
        cornerRadius: locationContainer.layer.cornerRadius
      ).cgPath
  }

  override func becomeFirstResponder() -> Bool {
    return self.locationTextField?.becomeFirstResponder() ?? false
  }

  private func makePlaceholder(colors: some BrowserColors) -> NSAttributedString {
    NSAttributedString(
      string: Strings.tabToolbarSearchAddressPlaceholderText,
      attributes: [.foregroundColor: colors.textTertiary]
    )
  }

  private func updateColors() {
    let browserColors = privateBrowsingManager.browserColors
    backgroundColor = browserColors.chromeBackground
    locationTextField?.backgroundColor = browserColors.containerBackground
    locationTextField?.textColor = browserColors.textPrimary
    locationTextField?.attributedPlaceholder = makePlaceholder(colors: browserColors)
    for button in [qrCodeButton, voiceSearchButton] {
      button.primaryTintColor = browserColors.iconDefault
      button.disabledTintColor = browserColors.iconDisabled
      button.selectedTintColor = browserColors.iconActive
    }
  }

  /// Created whenever the location bar on top is selected
  /// it is "converted" from static to actual TextField
  private func createLocationTextFieldContainer() {
    guard locationTextField == nil else { return }

    locationTextField = AutocompleteTextField()

    guard let locationTextField = locationTextField else { return }

    locationTextField.do {
      $0.backgroundColor = privateBrowsingManager.browserColors.containerBackground
      $0.textColor = privateBrowsingManager.browserColors.textPrimary
      $0.translatesAutoresizingMaskIntoConstraints = false
      $0.autocompleteDelegate = self
      $0.keyboardType = .webSearch
      $0.autocorrectionType = .no
      $0.autocapitalizationType = .none
      $0.smartDashesType = .no
      $0.returnKeyType = .go
      $0.clearButtonMode = .whileEditing
      $0.textAlignment = .left
      $0.font = .preferredFont(forTextStyle: .body)
      $0.accessibilityIdentifier = "address"
      $0.accessibilityLabel = Strings.URLBarViewLocationTextViewAccessibilityLabel
      $0.attributedPlaceholder = self.makePlaceholder(colors: .standard)
      $0.clearButtonMode = .whileEditing
      $0.rightViewMode = .never
      if let dropInteraction = $0.textDropInteraction {
        $0.removeInteraction(dropInteraction)
      }
    }

    if RecentSearchQRCodeScannerController.hasCameraSupport {
      locationBarOptionsStackView.addArrangedSubview(qrCodeButton)
    }
    if isVoiceSearchAvailable {
      locationBarOptionsStackView.addArrangedSubview(voiceSearchButton)
    }

    let subviews = [locationTextField, locationBarOptionsStackView]
    locationTextContentView = UIStackView(arrangedSubviews: subviews).then {
      $0.layoutMargins = UIEdgeInsets(top: 2, left: 8, bottom: 2, right: 0)
      $0.isLayoutMarginsRelativeArrangement = true
      $0.insetsLayoutMarginsFromSafeArea = false
      $0.spacing = 8
      $0.setCustomSpacing(4, after: locationTextField)
    }

    guard let locationTextContentView = locationTextContentView else { return }

    locationContainer.addSubview(locationTextContentView)

    locationTextContentView.snp.remakeConstraints {
      let insets = UIEdgeInsets(
        top: 0,
        left: UX.locationPadding,
        bottom: 0,
        right: UX.locationPadding
      )
      $0.edges.equalTo(locationContainer).inset(insets)
    }
  }

  private func removeLocationTextField() {
    locationTextContentView?.removeFromSuperview()
    locationTextContentView = nil

    locationTextField?.removeFromSuperview()
    locationTextField = nil
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
    updateViewsForOverlayModeAndToolbarChanges()
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

  func setAutocompleteSuggestion(_ suggestion: String?) {
    locationTextField?.setAutocompleteSuggestion(suggestion)
  }

  func setLocation(_ location: String?, search: Bool) {
    guard let text = location, !text.isEmpty else {
      locationTextField?.text = location
      updateLocationBarRightView(showToolbarActions: true)
      return
    }

    updateLocationBarRightView(showToolbarActions: false)

    if search {
      locationTextField?.text = text
      // Not notifying when empty agrees with AutocompleteTextField.textDidChange.
      delegate?.topToolbar(self, didEnterText: text)
    } else {
      locationTextField?.setTextWithoutSearching(text)
    }
  }

  func submitLocation(_ location: String?) {
    locationTextField?.text = location
    guard let text = location, !text.isEmpty else {
      return
    }
    // Not notifying when empty agrees with AutocompleteTextField.textDidChange.
    delegate?.topToolbar(self, didSubmitText: text)
  }

  func enterOverlayMode(_ locationText: String?, pasted: Bool, search: Bool) {
    createLocationTextFieldContainer()
    updateForTraitCollection()

    // Show the overlay mode UI, which includes hiding the locationView and replacing it
    // with the editable locationTextField.
    animateToOverlayState(overlayMode: true)

    delegate?.topToolbarDidEnterOverlayMode(self)

    // Bug 1193755 Workaround - Calling becomeFirstResponder before the animation happens
    // won't take the initial frame of the label into consideration, which makes the label
    // look squished at the start of the animation and expand to be correct. As a workaround,
    // we becomeFirstResponder as the next event on UI thread, so the animation starts before we
    // set a first responder.
    guard let urlTextField = locationTextField else {
      return
    }

    if pasted {
      // Clear any existing text, focus the field, then set the actual pasted text.
      // This avoids highlighting all of the text.
      urlTextField.text = ""
    }

    DispatchQueue.main.async {
      urlTextField.becomeFirstResponder()
      self.setLocation(locationText, search: search)
    }

    if !pasted {
      DispatchQueue.main.async {
        // When Not-pasted selecting text shiuld be from beggining to end
        let textRange = urlTextField.textRange(
          from: urlTextField.beginningOfDocument,
          to: urlTextField.endOfDocument
        )
        urlTextField.selectedTextRange = textRange
      }
    }

    setNeedsUpdateConstraints()
  }

  func leaveOverlayMode(didCancel cancel: Bool = false) {
    if !inOverlayMode { return }
    locationTextField?.resignFirstResponder()
    animateToOverlayState(overlayMode: false, didCancel: cancel)
    delegate?.topToolbarDidLeaveOverlayMode(self)
  }

  private func updateViewsForOverlayModeAndToolbarChanges() {
    // UIStackView bug:
    // Don't set `isHidden` to the same value on a view that adjusts layout of a UIStackView
    // inside of a UIView.animate() block, otherwise on occasion the view will render but
    // `isHidden` will still be true
    if cancelButton.isHidden == inOverlayMode {
      cancelButton.isHidden = !inOverlayMode
    }
    backButton.isHidden = !toolbarIsShowing || inOverlayMode
    forwardButton.isHidden = !toolbarIsShowing || inOverlayMode
    shareButton.isHidden = !toolbarIsShowing || inOverlayMode
    trailingItemsStackView.isHidden = !toolbarIsShowing || inOverlayMode
    locationView.contentView.isHidden = inOverlayMode
    shieldsRewardsStack.isHidden = inOverlayMode

    let selectedShortcut: WidgetShortcut? = Preferences.General.toolbarShortcutButton.value.flatMap(
      WidgetShortcut.init
    )
    shortcutButton.isHidden = selectedShortcut != nil ? inOverlayMode : true
    if let selectedShortcut {
      shortcutButton.setImage(selectedShortcut.image, for: .normal)
      shortcutButton.accessibilityLabel = selectedShortcut.displayString
    }
    leadingItemsStackView.isHidden = leadingItemsStackView.arrangedSubviews.allSatisfy(\.isHidden)
  }

  private func animateToOverlayState(overlayMode overlay: Bool, didCancel cancel: Bool = false) {
    inOverlayMode = overlay

    if !overlay {
      removeLocationTextField()
    }

    if inOverlayMode {
      [
        leadingItemsStackView, shortcutButton, shieldsRewardsStack, trailingItemsStackView,
        locationView.contentView,
      ].forEach {
        $0?.isHidden = true
      }

      cancelButton.isHidden = false
    } else {
      updateViewsForOverlayModeAndToolbarChanges()
    }

    layoutIfNeeded()
  }

  private func updateLocationBarRightView(showToolbarActions: Bool) {
    locationBarOptionsStackView.isHidden = !showToolbarActions

    if RecentSearchQRCodeScannerController.hasCameraSupport {
      qrCodeButton.isHidden = !showToolbarActions
    } else {
      qrCodeButton.isHidden = true
    }

    if isVoiceSearchAvailable {
      voiceSearchButton.isHidden = !showToolbarActions
    } else {
      voiceSearchButton.isHidden = true
    }
  }

  /// Update the shields icon based on whether or not shields are enabled for this site
  func refreshShieldsStatus() {
    // Default on
    var shieldIcon = "brave.logo"
    let shieldsOffIcon = "brave.logo.greyscale"
    if let currentURL = currentURL {
      let isPrivateBrowsing = privateBrowsingManager.isPrivateBrowsing
      let domain = Domain.getOrCreate(forUrl: currentURL, persistent: !isPrivateBrowsing)
      if domain.areAllShieldsOff {
        shieldIcon = shieldsOffIcon
      }
      if currentURL.isLocal || currentURL.isLocalUtility {
        shieldIcon = shieldsOffIcon
      }
    } else {
      shieldIcon = shieldsOffIcon
    }

    shieldsButton.setImage(UIImage(sharedNamed: shieldIcon), for: .normal)
  }

  // MARK: Actions

  @objc func didClickCancel() {
    leaveOverlayMode(didCancel: true)
  }

  @objc func tappedScrollToTopArea() {
    delegate?.topToolbarDidPressScrollToTop(self)
  }

  @objc func didClickShortcutButton() {
    delegate?.topToolbarDidTapShortcutButton(self)
  }

  @objc func didClickMenu() {
    delegate?.topToolbarDidTapMenuButton(self)
  }

  @objc func topToolbarDidPressQrCodeButton() {
    delegate?.topToolbarDidPressQrCodeButton(self)
    leaveOverlayMode(didCancel: true)
  }

  @objc func topToolbarDidPressVoiceSearchButton() {
    leaveOverlayMode(didCancel: true)
    delegate?.topToolbarDidPressVoiceSearchButton(self)
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
    updateViewsForOverlayModeAndToolbarChanges()
  }
}

// MARK:  TabLocationViewDelegate

extension TopToolbarView: TabLocationViewDelegate {
  func tabLocationViewDidTapLocation(_ tabLocationView: TabLocationView) {
    guard
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
    enterOverlayMode(overlayText, pasted: false, search: isSearchQuery)
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

// MARK:  AutocompleteTextFieldDelegate

extension TopToolbarView: AutocompleteTextFieldDelegate {
  func autocompleteTextFieldShouldReturn(_ autocompleteTextField: AutocompleteTextField) -> Bool {
    guard let text = locationTextField?.text else { return true }
    if !text.trimmingCharacters(in: .whitespaces).isEmpty {
      delegate?.topToolbar(self, didSubmitText: text)
      return true
    } else {
      return false
    }
  }

  func autocompleteTextField(
    _ autocompleteTextField: AutocompleteTextField,
    didEnterText text: String
  ) {
    delegate?.topToolbar(self, didEnterText: text)
    updateLocationBarRightView(showToolbarActions: text.isEmpty)
  }

  func autocompleteTextField(
    _ autocompleteTextField: AutocompleteTextField,
    didDeleteAutoSelectedText text: String
  ) {
    updateLocationBarRightView(showToolbarActions: text.isEmpty)
  }

  func autocompleteTextFieldDidBeginEditing(_ autocompleteTextField: AutocompleteTextField) {
    autocompleteTextField.highlightAll()
    updateLocationBarRightView(
      showToolbarActions: locationView.urlDisplayLabel.text?.isEmpty == true
    )
  }

  func autocompleteTextFieldShouldClear(_ autocompleteTextField: AutocompleteTextField) -> Bool {
    delegate?.topToolbar(self, didEnterText: "")
    updateLocationBarRightView(showToolbarActions: true)
    return true
  }

  func autocompleteTextFieldDidCancel(_ autocompleteTextField: AutocompleteTextField) {
    leaveOverlayMode(didCancel: true)
    updateLocationBarRightView(showToolbarActions: false)
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
      !locationView.reloadButton.isHighlighted, !inOverlayMode
    else {
      return []
    }

    let dragItem = UIDragItem(itemProvider: itemProvider)
    dragItem.localObject = locationTextField
    return [dragItem]
  }

  func dragInteraction(_ interaction: UIDragInteraction, sessionWillBegin session: UIDragSession) {
    delegate?.topToolbarDidBeginDragInteraction(self)
  }
}
