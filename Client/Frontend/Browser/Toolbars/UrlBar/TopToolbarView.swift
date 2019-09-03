/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Shared
import SnapKit
import BraveShared
import Data

private struct TopToolbarViewUX {
    static let LocationPadding: CGFloat = 8
    static let SmallPadding: CGFloat = 2
    static let NormalPadding: CGFloat = 10
    static let LocationHeight: CGFloat = 34
    static let ButtonHeight: CGFloat = 44
    static let LocationContentOffset: CGFloat = 8
    static let TextFieldCornerRadius: CGFloat = 8
    static let ProgressBarHeight: CGFloat = 3
    
    static let TabsButtonRotationOffset: CGFloat = 1.5
    static let TabsButtonHeight: CGFloat = 18.0
    static let ToolbarButtonInsets = UIEdgeInsets(equalInset: NormalPadding)
}

protocol TopToolbarDelegate: class {
    func topToolbarDidPressTabs(_ topToolbar: TopToolbarView)
    func topToolbarDidPressReaderMode(_ topToolbar: TopToolbarView)
    /// - returns: whether the long-press was handled by the delegate; i.e. return `false` when the conditions for even starting handling long-press were not satisfied
    func topToolbarDidLongPressReaderMode(_ topToolbar: TopToolbarView) -> Bool
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
    func topToolbarDidTapMenuButton(_ topToolbar: TopToolbarView)
    
    func topToolbarDidLongPressReloadButton(_ urlBar: TopToolbarView, from button: UIButton)
    func topToolbarDidPressStop(_ urlBar: TopToolbarView)
    func topToolbarDidPressReload(_ urlBar: TopToolbarView)
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
    
    fileprivate var currentTheme: Theme = .regular
    
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
    
    var contentIsSecure: Bool {
        get { return locationView.contentIsSecure }
        set { locationView.contentIsSecure = newValue }
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
    
    let line = UIView()
    
    lazy var tabsButton: TabsButton = {
        let tabsButton = TabsButton.tabTrayButton()
        tabsButton.accessibilityIdentifier = "TopToolbarView.tabsButton"
        return tabsButton
    }()
    
    fileprivate lazy var progressBar: GradientProgressBar = {
        let progressBar = GradientProgressBar()
        progressBar.clipsToBounds = false
        return progressBar
    }()
    
    fileprivate lazy var cancelButton: UIButton = {
        let cancelButton = InsetButton()
        cancelButton.setTitle(Strings.CancelButtonTitle, for: .normal)
        cancelButton.setTitleColor(BraveUX.CancelTextColor, for: .normal)
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

    lazy var bookmarkButton = ToolbarButton().then {
        $0.setImage(#imageLiteral(resourceName: "menu_bookmarks").template, for: .normal)
        $0.accessibilityLabel = Strings.BookmarksMenuItem
        $0.addTarget(self, action: #selector(didClickBookmarkButton), for: .touchUpInside)
    }
    
    var forwardButton = ToolbarButton()
    var shareButton = ToolbarButton()
    var addTabButton = ToolbarButton()
    lazy var menuButton = ToolbarButton().then {
        $0.contentMode = .center
        $0.accessibilityIdentifier = "topToolbarView-menuButton"
    }

    var backButton: ToolbarButton = {
        let backButton = ToolbarButton()
        backButton.accessibilityIdentifier = "TopToolbarView.backButton"
        return backButton
    }()

    lazy var actionButtons: [Themeable & UIButton] =
        [self.shareButton, self.tabsButton, self.bookmarkButton,
         self.forwardButton, self.backButton, self.menuButton].compactMap { $0 }
    
    /// Update the shields icon based on whether or not shields are enabled for this site
    func refreshShieldsStatus() {
        // Default on
        var shieldIcon = "shields-menu-icon"
        if let currentURL = currentURL {
            let isPrivateBrowsing = PrivateBrowsingManager.shared.isPrivateBrowsing
            let domain = Domain.getOrCreate(forUrl: currentURL, persistent: !isPrivateBrowsing)
            if domain.shield_allOff == 1 {
                shieldIcon = "shields-off-menu-icon"
            }
        }
        
        locationView.shieldsButton.setImage(UIImage(imageLiteralResourceName: shieldIcon), for: .normal)
    }
    
    override init(frame: CGRect) {
        super.init(frame: frame)
        commonInit()
    }
    
    required init?(coder aDecoder: NSCoder) {
        super.init(coder: aDecoder)
        commonInit()
    }
    
    private let mainStackView = UIStackView().then {
        $0.alignment = .center
        $0.spacing = 8
        $0.translatesAutoresizingMaskIntoConstraints = false
    }
    
    private let navigationStackView = UIStackView().then {
        $0.distribution = .fillEqually
        $0.translatesAutoresizingMaskIntoConstraints = false
    }
    
    private func commonInit() {
        locationContainer.addSubview(locationView)
        
        [scrollToTopButton, line, tabsButton, progressBar, cancelButton].forEach(addSubview(_:))
        addSubview(mainStackView)
        
        helper = ToolbarHelper(toolbar: self)
        setupConstraints()
        
        Preferences.General.showBookmarkToolbarShortcut.observe(from: self)
        
        // Make sure we hide any views that shouldn't be showing in non-overlay mode.
        updateViewsForOverlayModeAndToolbarChanges()
    }
    
    private func setupConstraints() {
        // Buttons won't take unnecessary space and won't shrink
        actionButtons.forEach {
            $0.setContentCompressionResistancePriority(.defaultHigh, for: .horizontal)
            $0.setContentHuggingPriority(.defaultHigh, for: .horizontal)
        }
        
        // Url bar will expand while keeping space for other items on the address bar.
        locationContainer.setContentHuggingPriority(.defaultLow, for: .horizontal)
        
        locationContainer.snp.makeConstraints {
            $0.height.equalTo(TopToolbarViewUX.LocationHeight)
        }
        
        navigationStackView.addArrangedSubview(backButton)
        navigationStackView.addArrangedSubview(forwardButton)
        
        [backButton, forwardButton, bookmarkButton, tabsButton, menuButton].forEach {
            $0.contentEdgeInsets = UIEdgeInsets(top: 4, left: 8, bottom: 4, right: 8)
        }
        
        [navigationStackView, bookmarkButton, locationContainer, tabsButton, menuButton, cancelButton].forEach {
            mainStackView.addArrangedSubview($0)
        }
        
        mainStackView.setCustomSpacing(16, after: locationContainer)
        
        mainStackView.snp.makeConstraints { make in
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
            make.top.equalTo(self.snp.bottom).inset(TopToolbarViewUX.ProgressBarHeight / 2)
            make.height.equalTo(TopToolbarViewUX.ProgressBarHeight)
            make.left.right.equalTo(self)
        }
        
        locationView.snp.makeConstraints { make in
            make.edges.equalTo(self.locationContainer)
        }
    }
    
    private var topToolbarPadding: CGFloat {
        // The only case where we want small padding is on iPads and iPhones in landscape.
        // Instead of padding we give extra tap area for buttons on the toolbar.
        if !inOverlayMode && toolbarIsShowing { return TopToolbarViewUX.SmallPadding }
        return TopToolbarViewUX.NormalPadding
    }
    
    private func updateMargins() {
        mainStackView.snp.updateConstraints {
            $0.leading.equalTo(self.safeArea.leading).inset(topToolbarPadding)
            $0.trailing.equalTo(self.safeArea.trailing).inset(topToolbarPadding)
        }
    }
    
    private func createLocationTextField() {
        guard locationTextField == nil else { return }
        
        locationTextField = UrlBarTextField()
        
        guard let locationTextField = locationTextField else { return }
        
        locationTextField.translatesAutoresizingMaskIntoConstraints = false
        locationTextField.autocompleteDelegate = self
        locationTextField.keyboardType = .webSearch
        locationTextField.autocorrectionType = .no
        locationTextField.autocapitalizationType = .none
        locationTextField.smartDashesType = .no
        locationTextField.returnKeyType = .go
        locationTextField.clearButtonMode = .whileEditing
        locationTextField.textAlignment = .left
        locationTextField.font = UIConstants.DefaultChromeFont
        locationTextField.accessibilityIdentifier = "address"
        locationTextField.accessibilityLabel = Strings.URLBarViewLocationTextViewAccessibilityLabel
        locationTextField.attributedPlaceholder = self.locationView.placeholder
        locationContainer.addSubview(locationTextField)
        locationTextField.snp.remakeConstraints { make in
            let insets = UIEdgeInsets(top: 0, left: TopToolbarViewUX.LocationPadding,
                                      bottom: 0, right: TopToolbarViewUX.LocationPadding)
            make.edges.equalTo(self.locationView).inset(insets)
        }
        
        locationTextField.applyTheme(currentTheme)
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
            return
        }
        if search {
            locationTextField?.text = text
            // Not notifying when empty agrees with AutocompleteTextField.textDidChange.
            delegate?.topToolbar(self, didEnterText: text)
        } else {
            locationTextField?.setTextWithoutSearching(text)
        }
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
            // Copy the current URL to the editable text field, then activate it.
            self.setLocation(locationText, search: search)
            DispatchQueue.main.async {
                self.locationTextField?.becomeFirstResponder()
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
        cancelButton.isHidden = !inOverlayMode
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
        delegate?.topToolbarDidLongPressLocation(self)
    }

    func tabLocationViewDidTapReaderMode(_ tabLocationView: TabLocationView) {
        delegate?.topToolbarDidPressReaderMode(self)
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
    }
    
    func autocompleteTextFieldDidBeginEditing(_ autocompleteTextField: AutocompleteTextField) {
        autocompleteTextField.highlightAll()
    }
    
    func autocompleteTextFieldShouldClear(_ autocompleteTextField: AutocompleteTextField) -> Bool {
        delegate?.topToolbar(self, didEnterText: "")
        return true
    }
    
    func autocompleteTextFieldDidCancel(_ autocompleteTextField: AutocompleteTextField) {
        leaveOverlayMode(didCancel: true)
    }
}

// MARK: - Themeable

extension TopToolbarView: Themeable {
    
    func applyTheme(_ theme: Theme) {
        locationView.applyTheme(theme)
        locationTextField?.applyTheme(theme)
        actionButtons.forEach { $0.applyTheme(theme) }
        tabsButton.applyTheme(theme)
        
        progressBar.setGradientColors(startColor: UIColor.LoadingBar.Start.colorFor(theme), endColor: UIColor.LoadingBar.End.colorFor(theme))
        currentTheme = theme
        cancelButton.setTitleColor(UIColor.Browser.Tint.colorFor(theme), for: .normal)
        switch theme {
        case .regular:
            backgroundColor = BraveUX.ToolbarsBackgroundSolidColor
        case .private:
            backgroundColor = BraveUX.DarkToolbarsBackgroundSolidColor
        }
        line.backgroundColor = UIColor.Browser.URLBarDivider.colorFor(theme)
    }
}

