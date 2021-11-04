/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Shared
import SnapKit
import BraveShared
import Data
import Combine

private struct TopToolbarViewUX {
    static let locationPadding: CGFloat = 8
    static let smallPadding: CGFloat = 2
    static let normalPadding: CGFloat = 10
    static let locationHeight: CGFloat = 34
    static let buttonHeight: CGFloat = 44
    static let locationContentOffset: CGFloat = 8
    static let textFieldCornerRadius: CGFloat = 8
    static let progressBarHeight: CGFloat = 3
    
    static let tabsButtonRotationOffset: CGFloat = 1.5
    static let tabsButtonHeight: CGFloat = 18.0
    static let toolbarButtonInsets = UIEdgeInsets(equalInset: normalPadding)
}

protocol TopToolbarDelegate: AnyObject {
    func topToolbarDidPressTabs(_ topToolbar: TopToolbarView)
    func topToolbarDidPressReaderMode(_ topToolbar: TopToolbarView)
    /// - returns: whether the long-press was handled by the delegate; i.e. return `false` when the conditions for even starting handling long-press were not satisfied
    func topToolbarDidLongPressReaderMode(_ topToolbar: TopToolbarView) -> Bool
    func topToolbarDidPressPlaylistButton(_ urlBar: TopToolbarView)
    func topToolbarDidEnterOverlayMode(_ topToolbar: TopToolbarView)
    func topToolbarDidLeaveOverlayMode(_ topToolbar: TopToolbarView)
    func topToolbarDidLongPressLocation(_ topToolbar: TopToolbarView)
    func topToolbarLocationAccessibilityActions(_ topToolbar: TopToolbarView) -> [UIAccessibilityCustomAction]?
    func topToolbarDidPressScrollToTop(_ topToolbar: TopToolbarView)
    func topToolbar(_ topToolbar: TopToolbarView, didEnterText text: String)
    func topToolbar(_ topToolbar: TopToolbarView, didSubmitText text: String)
    // Returns either (search query, true) or (url, false).
    func topToolbarDisplayTextForURL(_ url: URL?) -> (String?, Bool)
    func topToolbarDidBeginDragInteraction(_ topToolbar: TopToolbarView)
    func topToolbarDidTapBookmarkButton(_ topToolbar: TopToolbarView)
    func topToolbarDidTapBraveShieldsButton(_ topToolbar: TopToolbarView)
    func topToolbarDidTapBraveRewardsButton(_ topToolbar: TopToolbarView)
    func topToolbarDidLongPressBraveRewardsButton(_ topToolbar: TopToolbarView)
    func topToolbarDidTapMenuButton(_ topToolbar: TopToolbarView)
    
    func topToolbarDidLongPressReloadButton(_ urlBar: TopToolbarView, from button: UIButton)
    func topToolbarDidPressStop(_ urlBar: TopToolbarView)
    func topToolbarDidPressReload(_ urlBar: TopToolbarView)
    func topToolbarDidPressQrCodeButton(_ urlBar: TopToolbarView)
}

class TopToolbarView: UIView, ToolbarProtocol {
    weak var delegate: TopToolbarDelegate?
    var helper: ToolbarHelper?
    
    // MARK: - ToolbarProtocol properties
    
    weak var tabToolbarDelegate: ToolbarDelegate?
    
    // MARK: - State
    
    var isTransitioning: Bool = false {
        didSet {
            if isTransitioning {
                // Cancel any pending/in-progress animations related to the progress bar
                self.progressBar.setProgress(1, animated: false)
                self.progressBar.alpha = 0.0
            }
        }
    }
    
    var toolbarIsShowing = false
    
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
    
    // MARK: - Views
    fileprivate var locationTextField: UrlBarTextField?
    
    lazy var locationView: TabLocationView = {
        let locationView = TabLocationView()
        locationView.translatesAutoresizingMaskIntoConstraints = false
        locationView.readerModeState = ReaderModeState.unavailable
        locationView.delegate = self
        return locationView
    }()
    
    lazy var locationContainer: UIView = {
        let locationContainer = LocationContainerView()
        locationContainer.translatesAutoresizingMaskIntoConstraints = false
        locationContainer.backgroundColor = .clear
        return locationContainer
    }()
    
    private var qrCodeButton = UIButton().then {
        $0.setImage(#imageLiteral(resourceName: "recent-search-qrcode"), for: .normal)
        $0.imageView?.contentMode = .scaleAspectFit
        $0.accessibilityLabel = Strings.quickActionScanQRCode
    }

    let line = UIView().then {
        $0.backgroundColor = .braveSeparator
    }
    
    let tabsButton = TabsButton()
    
    fileprivate lazy var progressBar: GradientProgressBar = {
        let progressBar = GradientProgressBar()
        progressBar.clipsToBounds = false
        progressBar.setGradientColors(startColor: .braveOrange, endColor: .braveOrange)
        return progressBar
    }()
    
    fileprivate lazy var cancelButton: UIButton = {
        let cancelButton = InsetButton()
        cancelButton.setTitle(Strings.cancelButtonTitle, for: .normal)
        cancelButton.setTitleColor(UIColor.secondaryBraveLabel, for: .normal)
        cancelButton.accessibilityIdentifier = "topToolbarView-cancel"
        cancelButton.addTarget(self, action: #selector(didClickCancel), for: .touchUpInside)
        cancelButton.setContentCompressionResistancePriority(.required, for: .horizontal)
        cancelButton.setContentHuggingPriority(.defaultHigh, for: .horizontal)
        return cancelButton
    }()
    
    fileprivate lazy var scrollToTopButton: UIButton = {
        let button = UIButton()
        button.addTarget(self, action: #selector(tappedScrollToTopArea), for: .touchUpInside)
        return button
    }()

    lazy var bookmarkButton = ToolbarButton(top: true).then {
        $0.setImage(#imageLiteral(resourceName: "menu_bookmarks").template, for: .normal)
        $0.accessibilityLabel = Strings.bookmarksMenuItem
        $0.addTarget(self, action: #selector(didClickBookmarkButton), for: .touchUpInside)
    }
    
    var forwardButton = ToolbarButton(top: true)
    var shareButton = ToolbarButton(top: true)
    var addTabButton = ToolbarButton(top: true)
    // Do nothing with this, just required for protocol conformance
    var searchButton = ToolbarButton(top: true)
    lazy var menuButton = MenuButton(top: true).then {
        $0.accessibilityIdentifier = "topToolbarView-menuButton"
    }

    var backButton: ToolbarButton = {
        let backButton = ToolbarButton(top: true)
        backButton.accessibilityIdentifier = "TopToolbarView.backButton"
        return backButton
    }()

    lazy var actionButtons: [UIButton] =
        [self.shareButton, self.tabsButton, self.bookmarkButton,
         self.forwardButton, self.backButton, self.menuButton].compactMap { $0 }
    
    /// Update the shields icon based on whether or not shields are enabled for this site
    func refreshShieldsStatus() {
        // Default on
        var shieldIcon = "shields-menu-icon"
        let shieldsOffIcon = "shields-off-menu-icon"
        if let currentURL = currentURL {
            let isPrivateBrowsing = PrivateBrowsingManager.shared.isPrivateBrowsing
            let domain = Domain.getOrCreate(forUrl: currentURL, persistent: !isPrivateBrowsing)
            if domain.shield_allOff == 1 {
                shieldIcon = shieldsOffIcon
            }
            if currentURL.isLocal || currentURL.isLocalUtility {
                shieldIcon = shieldsOffIcon
            }
        } else {
            shieldIcon = shieldsOffIcon
        }
        
        locationView.shieldsButton.setImage(UIImage(imageLiteralResourceName: shieldIcon), for: .normal)
    }
    
    private var privateModeCancellable: AnyCancellable?
    private func updateColors(_ isPrivateBrowsing: Bool) {
        if isPrivateBrowsing {
            backgroundColor = .privateModeBackground
        } else {
            backgroundColor = .secondaryBraveBackground
        }
    }
    
    override init(frame: CGRect) {
        super.init(frame: frame)
        
        backgroundColor = .secondaryBraveBackground
        
        locationContainer.addSubview(locationView)
        
        [scrollToTopButton, line, tabsButton, progressBar, cancelButton].forEach(addSubview(_:))
        addSubview(mainStackView)
        
        helper = ToolbarHelper(toolbar: self)
        
        // Buttons won't take unnecessary space and won't shrink
        actionButtons.forEach {
            $0.setContentCompressionResistancePriority(.defaultHigh, for: .horizontal)
            $0.setContentHuggingPriority(.defaultHigh, for: .horizontal)
        }
        
        // Url bar will expand while keeping space for other items on the address bar.
        locationContainer.setContentHuggingPriority(.defaultLow, for: .horizontal)
        
        navigationStackView.addArrangedSubview(backButton)
        navigationStackView.addArrangedSubview(forwardButton)
        
        [backButton, forwardButton, bookmarkButton, tabsButton, menuButton].forEach {
            $0.contentEdgeInsets = UIEdgeInsets(top: 4, left: 8, bottom: 4, right: 8)
        }
        
        [navigationStackView, bookmarkButton, locationContainer, tabsButton, menuButton, cancelButton].forEach {
            mainStackView.addArrangedSubview($0)
        }
        
        mainStackView.setCustomSpacing(16, after: locationContainer)
        
        setupConstraints()
        
        Preferences.General.showBookmarkToolbarShortcut.observe(from: self)
        
        // Make sure we hide any views that shouldn't be showing in non-overlay mode.
        updateViewsForOverlayModeAndToolbarChanges()
        
        privateModeCancellable = PrivateBrowsingManager.shared
            .$isPrivateBrowsing
            .removeDuplicates()
            .sink(receiveValue: { [weak self] isPrivateBrowsing in
                self?.updateColors(isPrivateBrowsing)
            })
    }
    
    @available(*, unavailable)
    required init(coder: NSCoder) {
        fatalError()
    }
    
    private let mainStackView = UIStackView().then {
        $0.alignment = .center
        $0.spacing = 8
        $0.isLayoutMarginsRelativeArrangement = true
        $0.layoutMargins = UIEdgeInsets(top: 0.0, left: 0.0, bottom: 0.0, right: 10.0)
        $0.translatesAutoresizingMaskIntoConstraints = false
    }
    
    private let navigationStackView = UIStackView().then {
        $0.distribution = .fillEqually
        $0.translatesAutoresizingMaskIntoConstraints = false
    }
    
    private func setupConstraints() {
        locationContainer.snp.remakeConstraints {
            $0.height.equalTo(TopToolbarViewUX.locationHeight)
        }
        
        mainStackView.snp.remakeConstraints { make in
            make.top.bottom.equalTo(self)
            make.leading.equalTo(self.safeArea.leading).inset(topToolbarPadding)
            make.trailing.equalTo(self.safeArea.trailing).inset(topToolbarPadding)
        }
        
        line.snp.makeConstraints { make in
            make.bottom.leading.trailing.equalTo(self)
            make.height.equalTo(1.0 / UIScreen.main.scale)
        }
        
        scrollToTopButton.snp.makeConstraints { make in
            make.top.equalTo(self)
            make.left.right.equalTo(self.locationContainer)
        }
        
        progressBar.snp.makeConstraints { make in
            make.top.equalTo(self.snp.bottom).inset(TopToolbarViewUX.progressBarHeight / 2)
            make.height.equalTo(TopToolbarViewUX.progressBarHeight)
            make.left.right.equalTo(self)
        }
        
        locationView.snp.makeConstraints { make in
            make.edges.equalTo(self.locationContainer)
        }
    }
    
    private var topToolbarPadding: CGFloat {
        // The only case where we want small padding is on iPads and iPhones in landscape.
        // Instead of padding we give extra tap area for buttons on the toolbar.
        if !inOverlayMode && toolbarIsShowing { return TopToolbarViewUX.smallPadding }
        return TopToolbarViewUX.normalPadding
    }
    
    private func updateMargins() {
        mainStackView.snp.updateConstraints {
            $0.leading.equalTo(self.safeArea.leading).inset(topToolbarPadding)
            $0.trailing.equalTo(self.safeArea.trailing).inset(topToolbarPadding)
        }
    }
    
    /// Created whenever the location bar on top is selected
    ///     it is "converted" from static to actual TextField
    private func createLocationTextField() {
        guard locationTextField == nil else { return }
        
        locationTextField = UrlBarTextField()
        
        guard let locationTextField = locationTextField else { return }
        
        locationTextField.backgroundColor = .braveBackground
        locationTextField.translatesAutoresizingMaskIntoConstraints = false
        locationTextField.autocompleteDelegate = self
        locationTextField.keyboardType = .webSearch
        locationTextField.autocorrectionType = .no
        locationTextField.autocapitalizationType = .none
        locationTextField.smartDashesType = .no
        locationTextField.returnKeyType = .go
        locationTextField.clearButtonMode = .whileEditing
        locationTextField.textAlignment = .left
        locationTextField.font = UIConstants.defaultChromeFont
        locationTextField.accessibilityIdentifier = "address"
        locationTextField.accessibilityLabel = Strings.URLBarViewLocationTextViewAccessibilityLabel
        locationTextField.attributedPlaceholder = self.locationView.placeholder
        locationTextField.rightView = qrCodeButton
        locationTextField.rightViewMode = .never
        
        locationContainer.addSubview(locationTextField)
        locationTextField.snp.remakeConstraints { make in
            let insets = UIEdgeInsets(top: 0, left: TopToolbarViewUX.locationPadding,
                                      bottom: 0, right: TopToolbarViewUX.locationPadding)
            make.edges.equalTo(self.locationView).inset(insets)
        }

        qrCodeButton.addTarget(self, action: #selector(topToolbarDidPressQrCodeButton), for: .touchUpInside)
    }
    
    override func becomeFirstResponder() -> Bool {
        return self.locationTextField?.becomeFirstResponder() ?? false
    }
    
    private func removeLocationTextField() {
        locationTextField?.removeFromSuperview()
        locationTextField = nil
    }
    
    // Ideally we'd split this implementation in two, one TopToolbarView with a toolbar and one without
    // However, switching views dynamically at runtime is a difficult. For now, we just use one view
    // that can show in either mode.
    func setShowToolbar(_ shouldShow: Bool) {
        toolbarIsShowing = shouldShow
        updateMargins()
        setNeedsUpdateConstraints()
        // when we transition from portrait to landscape, calling this here causes
        // the constraints to be calculated too early and there are constraint errors
        if !toolbarIsShowing {
            updateConstraintsIfNeeded()
        }
        updateViewsForOverlayModeAndToolbarChanges()
    }
    
    func updateAlphaForSubviews(_ alpha: CGFloat) {
        locationContainer.alpha = alpha
        self.alpha = alpha
    }
    
    func currentProgress() -> Float {
        return progressBar.progress
    }
    
    static let psuedoProgressValue: Float = 0.1
    
    func updateProgressBar(_ progress: Float) {
        progressBar.alpha = 1
        progressBar.isHidden = false
        progressBar.setProgress(progress, animated: !isTransitioning)
    }
    
    func hideProgressBar() {
        progressBar.isHidden = true
        progressBar.setProgress(0, animated: false)
    }
    
    func updateReaderModeState(_ state: ReaderModeState) {
        locationView.readerModeState = state
    }
    
    func setAutocompleteSuggestion(_ suggestion: String?) {
        locationTextField?.setAutocompleteSuggestion(suggestion)
    }
    
    func setLocation(_ location: String?, search: Bool) {
        guard let text = location, !text.isEmpty else {
            locationTextField?.text = location
            updateLocationBarRightView(showQrCodeButton: true)
            return
        }
        
        updateLocationBarRightView(showQrCodeButton: false)
        
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
        createLocationTextField()
        
        // Show the overlay mode UI, which includes hiding the locationView and replacing it
        // with the editable locationTextField.
        animateToOverlayState(overlayMode: true)
        
        delegate?.topToolbarDidEnterOverlayMode(self)
        
        // Bug 1193755 Workaround - Calling becomeFirstResponder before the animation happens
        // won't take the initial frame of the label into consideration, which makes the label
        // look squished at the start of the animation and expand to be correct. As a workaround,
        // we becomeFirstResponder as the next event on UI thread, so the animation starts before we
        // set a first responder.
        if pasted {
            // Clear any existing text, focus the field, then set the actual pasted text.
            // This avoids highlighting all of the text.
            self.locationTextField?.text = ""
            DispatchQueue.main.async {
                self.locationTextField?.becomeFirstResponder()
                self.setLocation(locationText, search: search)
            }
        } else {
            DispatchQueue.main.async {
                self.locationTextField?.becomeFirstResponder()
                // Need to set location again so text could be immediately selected.
                self.setLocation(locationText, search: search)
                self.locationTextField?.selectAll(nil)
            }
        }
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
        progressBar.isHidden = inOverlayMode
        navigationStackView.isHidden = !toolbarIsShowing || inOverlayMode
        menuButton.isHidden = !toolbarIsShowing || inOverlayMode
        tabsButton.isHidden = !toolbarIsShowing || inOverlayMode
        locationView.contentView.isHidden = inOverlayMode
        
        let showBookmarkPref = Preferences.General.showBookmarkToolbarShortcut.value
        bookmarkButton.isHidden = showBookmarkPref ? inOverlayMode : true
    }
    
    private func animateToOverlayState(overlayMode overlay: Bool, didCancel cancel: Bool = false) {
        inOverlayMode = overlay
        
        if !overlay {
            removeLocationTextField()
        }
        
        if inOverlayMode {
            [progressBar, navigationStackView, bookmarkButton, menuButton, tabsButton, locationView.contentView].forEach {
                $0?.isHidden = true
            }
            
            cancelButton.isHidden = false
        } else {
            UIView.animate(withDuration: 0.3) {
                self.updateViewsForOverlayModeAndToolbarChanges()
            }
        }
        
        updateMargins()
        layoutIfNeeded()
    }
    
    private func updateLocationBarRightView(showQrCodeButton: Bool) {
        if RecentSearchQRCodeScannerController.hasCameraSupport {
            locationTextField?.clearButtonMode = showQrCodeButton ? .never : .whileEditing
            locationTextField?.rightViewMode = showQrCodeButton ? .always : .never
        } else {
            locationTextField?.clearButtonMode = .whileEditing
        }
    }
    
    func didClickAddTab() {
        delegate?.topToolbarDidPressTabs(self)
    }
    
    @objc func didClickCancel() {
        leaveOverlayMode(didCancel: true)
    }
    
    @objc func tappedScrollToTopArea() {
        delegate?.topToolbarDidPressScrollToTop(self)
    }
    
    @objc func didClickBookmarkButton() {
        delegate?.topToolbarDidTapBookmarkButton(self)
    }
    
    @objc func didClickMenu() {
        delegate?.topToolbarDidTapMenuButton(self)
    }
    
    @objc func didClickBraveShieldsButton() {
        delegate?.topToolbarDidTapBraveShieldsButton(self)
    }
    
    @objc func topToolbarDidPressQrCodeButton() {
        leaveOverlayMode(didCancel: true)
        delegate?.topToolbarDidPressQrCodeButton(self)
    }
}

// MARK: - PreferencesObserver

extension TopToolbarView: PreferencesObserver {
    func preferencesDidChange(for key: String) {
        updateViewsForOverlayModeAndToolbarChanges()
    }
}

// MARK: - TabLocationViewDelegate

extension TopToolbarView: TabLocationViewDelegate {
    func tabLocationViewDidTapShieldsButton(_ urlBar: TabLocationView) {
        delegate?.topToolbarDidTapBraveShieldsButton(self)
    }
    
    func tabLocationViewDidTapRewardsButton(_ urlBar: TabLocationView) {
        delegate?.topToolbarDidTapBraveRewardsButton(self)
    }
    
    func tabLocationViewDidLongPressRewardsButton(_ urlBar: TabLocationView) {
        delegate?.topToolbarDidLongPressBraveRewardsButton(self)
    }
    
    func tabLocationViewDidLongPressReaderMode(_ tabLocationView: TabLocationView) -> Bool {
        return delegate?.topToolbarDidLongPressReaderMode(self) ?? false
    }
    
    func tabLocationViewDidTapLocation(_ tabLocationView: TabLocationView) {
        guard let (locationText, isSearchQuery) = delegate?.topToolbarDisplayTextForURL(locationView.url as URL?) else { return }
        
        var overlayText = locationText
        // Make sure to use the result from topToolbarDisplayTextForURL as it is responsible for extracting out search terms when on a search page
        if let text = locationText, let url = URL(string: text), let host = url.host {
            overlayText = url.absoluteString.replacingOccurrences(of: host, with: host.asciiHostToUTF8())
        }
        enterOverlayMode(overlayText, pasted: false, search: isSearchQuery)
    }
    
    func tabLocationViewDidLongPressLocation(_ tabLocationView: TabLocationView) {
        delegate?.topToolbarDidLongPressLocation(self)
    }
    
    func tabLocationViewDidTapReload(_ tabLocationView: TabLocationView) {
        delegate?.topToolbarDidPressReload(self)
    }
    
    func tabLocationViewDidTapStop(_ tabLocationView: TabLocationView) {
        delegate?.topToolbarDidPressStop(self)
    }
    
    func tabLocationViewDidLongPressReload(_ tabLocationView: TabLocationView, from button: UIButton) {
        delegate?.topToolbarDidLongPressReloadButton(self, from: button)
    }

    func tabLocationViewDidTapReaderMode(_ tabLocationView: TabLocationView) {
        delegate?.topToolbarDidPressReaderMode(self)
    }
    
    func tabLocationViewDidTapPlaylist(_ tabLocationView: TabLocationView) {
        delegate?.topToolbarDidPressPlaylistButton(self)
    }

    func tabLocationViewLocationAccessibilityActions(_ tabLocationView: TabLocationView) -> [UIAccessibilityCustomAction]? {
        return delegate?.topToolbarLocationAccessibilityActions(self)
    }
    
    func tabLocationViewDidBeginDragInteraction(_ tabLocationView: TabLocationView) {
        delegate?.topToolbarDidBeginDragInteraction(self)
    }
}

// MARK: - AutocompleteTextFieldDelegate

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
    
    func autocompleteTextField(_ autocompleteTextField: AutocompleteTextField, didEnterText text: String) {
        delegate?.topToolbar(self, didEnterText: text)
        updateLocationBarRightView(showQrCodeButton: text.isEmpty)
    }
    
    func autocompleteTextField(_ autocompleteTextField: AutocompleteTextField, didDeleteAutoSelectedText text: String) {
        updateLocationBarRightView(showQrCodeButton: text.isEmpty)
    }
    
    func autocompleteTextFieldDidBeginEditing(_ autocompleteTextField: AutocompleteTextField) {
        autocompleteTextField.highlightAll()
        updateLocationBarRightView(showQrCodeButton: locationView.urlTextField.text?.isEmpty == true)
    }
    
    func autocompleteTextFieldShouldClear(_ autocompleteTextField: AutocompleteTextField) -> Bool {
        delegate?.topToolbar(self, didEnterText: "")
        updateLocationBarRightView(showQrCodeButton: true)
        return true
    }
    
    func autocompleteTextFieldDidCancel(_ autocompleteTextField: AutocompleteTextField) {
        leaveOverlayMode(didCancel: true)
        updateLocationBarRightView(showQrCodeButton: false)
    }
}
