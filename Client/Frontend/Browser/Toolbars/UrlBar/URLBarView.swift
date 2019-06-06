/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Shared
import SnapKit
import BraveShared
import Data

private struct URLBarViewUX {
    static let LocationPadding: CGFloat = 8
    static let Padding: CGFloat = 10
    static let LocationHeight: CGFloat = 34
    static let ButtonHeight: CGFloat = 44
    static let LocationContentOffset: CGFloat = 8
    static let TextFieldCornerRadius: CGFloat = 8
    static let ProgressBarHeight: CGFloat = 3
    
    static let TabsButtonRotationOffset: CGFloat = 1.5
    static let TabsButtonHeight: CGFloat = 18.0
    static let ToolbarButtonInsets = UIEdgeInsets(equalInset: Padding)
}

protocol URLBarDelegate: class {
    func urlBarDidPressTabs(_ urlBar: URLBarView)
    func urlBarDidPressReaderMode(_ urlBar: URLBarView)
    /// - returns: whether the long-press was handled by the delegate; i.e. return `false` when the conditions for even starting handling long-press were not satisfied
    func urlBarDidLongPressReaderMode(_ urlBar: URLBarView) -> Bool
    func urlBarDidPressStop(_ urlBar: URLBarView)
    func urlBarDidPressReload(_ urlBar: URLBarView)
    func urlBarDidEnterOverlayMode(_ urlBar: URLBarView)
    func urlBarDidLeaveOverlayMode(_ urlBar: URLBarView)
    func urlBarDidLongPressLocation(_ urlBar: URLBarView)
    func urlBarLocationAccessibilityActions(_ urlBar: URLBarView) -> [UIAccessibilityCustomAction]?
    func urlBarDidPressScrollToTop(_ urlBar: URLBarView)
    func urlBar(_ urlBar: URLBarView, didEnterText text: String)
    func urlBar(_ urlBar: URLBarView, didSubmitText text: String)
    // Returns either (search query, true) or (url, false).
    func urlBarDisplayTextForURL(_ url: URL?) -> (String?, Bool)
    func urlBarDidBeginDragInteraction(_ urlBar: URLBarView)
    func urlBarDidTapBraveShieldsButton(_ urlBar: URLBarView)
    func urlBarDidTapMenuButton(_ urlBar: URLBarView)
    func urlBarDidLongPressReloadButton(_ urlBar: URLBarView, from button: UIButton)
}

class URLBarView: UIView {
    weak var delegate: URLBarDelegate?
    weak var tabToolbarDelegate: ToolbarDelegate?
    var helper: ToolbarHelper?
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
    
    fileprivate var locationTextField: UrlBarTextField?
    
    /// Overlay mode is the state where the lock/reader icons are hidden, the home panels are shown,
    /// and the Cancel button is visible (allowing the user to leave overlay mode). Overlay mode
    /// is *not* tied to the location text field's editing state; for instance, when selecting
    /// a panel, the first responder will be resigned, yet the overlay mode UI is still active.
    var inOverlayMode = false
    
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
        tabsButton.accessibilityIdentifier = "URLBarView.tabsButton"
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
        cancelButton.accessibilityIdentifier = "urlBar-cancel"
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

    var bookmarkButton = ToolbarButton()
    var forwardButton = ToolbarButton()
    var shareButton = ToolbarButton()
    var addTabButton = ToolbarButton()
    lazy var menuButton = ToolbarButton().then {
        $0.contentMode = .center
        $0.setImage(#imageLiteral(resourceName: "nav-menu").template, for: .normal)
        $0.accessibilityLabel = Strings.AppMenuButtonAccessibilityLabel
        $0.addTarget(self, action: #selector(didClickMenu), for: .touchUpInside)
        $0.accessibilityIdentifier = "urlBar-menuButton"
    }
    
    lazy var shieldsButton: ToolbarButton = {
        let button = ToolbarButton()
        button.setImage(UIImage(imageLiteralResourceName: "shields-menu-icon"), for: .normal)
        button.addTarget(self, action: #selector(didClickBraveShieldsButton), for: .touchUpInside)
        button.imageView?.contentMode = .center
        button.accessibilityLabel = Strings.Brave_Panel
        button.accessibilityIdentifier = "urlBar-shieldsButton"
        return button
    }()

    var backButton: ToolbarButton = {
        let backButton = ToolbarButton()
        backButton.accessibilityIdentifier = "URLBarView.backButton"
        return backButton
    }()

    lazy var actionButtons: [Themeable & UIButton] = [self.shareButton, self.tabsButton, self.forwardButton, self.backButton, self.menuButton]

    var currentURL: URL? {
        get {
            return locationView.url as URL?
        }
        
        set(newURL) {
            locationView.url = newURL
            refreshShieldsStatus()
        }
    }
    
    /// Update the shields icon based on whether or not shields are enabled for this site
    func refreshShieldsStatus() {
        // Default on
        var shieldIcon = "shields-menu-icon"
        if let currentURL = currentURL {
            let domain = Domain.getOrCreate(forUrl: currentURL)
            if domain.shield_allOff == 1 {
                shieldIcon = "shields-off-menu-icon"
            }
        }
        shieldsButton.setImage(UIImage(imageLiteralResourceName: shieldIcon), for: .normal)
    }
    
    var contentIsSecure: Bool {
        get {
            return locationView.contentIsSecure
        }
        set {
            locationView.contentIsSecure = newValue
        }
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
        $0.spacing = 16
        $0.translatesAutoresizingMaskIntoConstraints = false
    }
    
    private let navigationStackView = UIStackView().then {
        $0.distribution = .fillEqually
        $0.spacing = 16
        $0.translatesAutoresizingMaskIntoConstraints = false
    }
    
    private func commonInit() {
        locationContainer.addSubview(locationView)
    
        [scrollToTopButton, line, tabsButton, progressBar, cancelButton].forEach { addSubview($0) }
        addSubview(mainStackView)
        
        helper = ToolbarHelper(toolbar: self)
        setupConstraints()
        
        // Make sure we hide any views that shouldn't be showing in non-overlay mode.
        updateViewsForOverlayModeAndToolbarChanges()
    }
    
    private func setupConstraints() {
        // Button won't take unnecessary space and won't shrink
        [shieldsButton, menuButton, cancelButton, tabsButton, backButton, forwardButton].forEach {
            $0.setContentCompressionResistancePriority(.defaultHigh, for: .horizontal)
            $0.setContentHuggingPriority(.defaultHigh, for: .horizontal)
        }
        
        // Url bar will expand while keeping space for other items on the address bar.
        locationContainer.setContentHuggingPriority(.defaultLow, for: .horizontal)
        
        navigationStackView.addArrangedSubview(backButton)
        navigationStackView.addArrangedSubview(forwardButton)
        
        [navigationStackView, locationContainer, shieldsButton, tabsButton, menuButton, cancelButton].forEach {
            mainStackView.addArrangedSubview($0)
        }
        
        mainStackView.snp.makeConstraints { make in
            make.top.bottom.equalTo(self)
            make.leading.equalTo(self.safeArea.leading).inset(URLBarViewUX.Padding)
            make.trailing.equalTo(self.safeArea.trailing).inset(URLBarViewUX.Padding)
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
            make.top.equalTo(self.snp.bottom).inset(URLBarViewUX.ProgressBarHeight / 2)
            make.height.equalTo(URLBarViewUX.ProgressBarHeight)
            make.left.right.equalTo(self)
        }
        
        locationView.snp.makeConstraints { make in
            make.edges.equalTo(self.locationContainer)
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
            let insets = UIEdgeInsets(top: 0, left: URLBarViewUX.LocationPadding,
                                      bottom: 0, right: URLBarViewUX.LocationPadding)
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
    
    // Ideally we'd split this implementation in two, one URLBarView with a toolbar and one without
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
            delegate?.urlBar(self, didEnterText: text)
        } else {
            locationTextField?.setTextWithoutSearching(text)
        }
    }
    
    func enterOverlayMode(_ locationText: String?, pasted: Bool, search: Bool) {
        createLocationTextField()
        
        // Show the overlay mode UI, which includes hiding the locationView and replacing it
        // with the editable locationTextField.
        animateToOverlayState(overlayMode: true)
        
        delegate?.urlBarDidEnterOverlayMode(self)
        
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
        delegate?.urlBarDidLeaveOverlayMode(self)
    }
    
    private func updateViewsForOverlayModeAndToolbarChanges() {
        cancelButton.isHidden = !inOverlayMode
        progressBar.isHidden = inOverlayMode
        navigationStackView.isHidden = !toolbarIsShowing || inOverlayMode
        menuButton.isHidden = !toolbarIsShowing || inOverlayMode
        shieldsButton.isHidden = inOverlayMode
        tabsButton.isHidden = !toolbarIsShowing || inOverlayMode
        locationView.contentView.isHidden = inOverlayMode
    }
    
    private func animateToOverlayState(overlayMode overlay: Bool, didCancel cancel: Bool = false) {
        inOverlayMode = overlay
        
        if !overlay {
            removeLocationTextField()
        }
        
        if inOverlayMode {
            [progressBar, navigationStackView, menuButton, shieldsButton, tabsButton, locationView.contentView].forEach {
                $0?.isHidden = true
            }
            
            cancelButton.isHidden = false
        } else {
            UIView.animate(withDuration: 0.3) {
                self.updateViewsForOverlayModeAndToolbarChanges()
            }
        }
        
        layoutIfNeeded()
    }
    
    func didClickAddTab() {
        delegate?.urlBarDidPressTabs(self)
    }
    
    @objc func didClickCancel() {
        leaveOverlayMode(didCancel: true)
    }
    
    @objc func tappedScrollToTopArea() {
        delegate?.urlBarDidPressScrollToTop(self)
    }
    
    @objc func didClickMenu() {
        delegate?.urlBarDidTapMenuButton(self)
    }
    
    @objc func didClickBraveShieldsButton() {
        delegate?.urlBarDidTapBraveShieldsButton(self)
    }
}

// MARK: - ToolbarProtocol

extension URLBarView: ToolbarProtocol {
    
    func updateBackStatus(_ canGoBack: Bool) {
        backButton.isEnabled = canGoBack
    }
    
    func updateForwardStatus(_ canGoForward: Bool) {
        forwardButton.isEnabled = canGoForward
    }
    
    func updateTabCount(_ count: Int) {
        tabsButton.updateTabCount(count)
    }

    func updatePageStatus(_ isWebPage: Bool) {
        locationView.reloadButton.isEnabled = isWebPage
        shareButton.isEnabled = isWebPage
    }
    
    var access: [Any]? {
        get {
            if inOverlayMode {
                guard let locationTextField = locationTextField else { return nil }
                return [locationTextField, cancelButton]
            } else {
                if toolbarIsShowing {
                    return [backButton, forwardButton, menuButton, locationView, shareButton, tabsButton, progressBar]
                } else {
                    return [menuButton, locationView, progressBar]
                }
            }
        }
        set {
            super.accessibilityElements = newValue
        }
    }
}

// MARK: - TabLocationViewDelegate

extension URLBarView: TabLocationViewDelegate {
    func tabLocationViewDidLongPressReaderMode(_ tabLocationView: TabLocationView) -> Bool {
        return delegate?.urlBarDidLongPressReaderMode(self) ?? false
    }
    
    func tabLocationViewDidTapLocation(_ tabLocationView: TabLocationView) {
        guard let (locationText, isSearchQuery) = delegate?.urlBarDisplayTextForURL(locationView.url as URL?) else { return }
        
        var overlayText = locationText
        // Make sure to use the result from urlBarDisplayTextForURL as it is responsible for extracting out search terms when on a search page
        if let text = locationText, let url = URL(string: text), let host = url.host, AppConstants.MOZ_PUNYCODE {
            overlayText = url.absoluteString.replacingOccurrences(of: host, with: host.asciiHostToUTF8())
        }
        enterOverlayMode(overlayText, pasted: false, search: isSearchQuery)
    }
    
    func tabLocationViewDidLongPressLocation(_ tabLocationView: TabLocationView) {
        delegate?.urlBarDidLongPressLocation(self)
    }
    
    func tabLocationViewDidTapReload(_ tabLocationView: TabLocationView) {
        delegate?.urlBarDidPressReload(self)
    }
    
    func tabLocationViewDidTapStop(_ tabLocationView: TabLocationView) {
        delegate?.urlBarDidPressStop(self)
    }
    
    func tabLocationViewDidLongPressReload(_ tabLocationView: TabLocationView, from button: UIButton) {
        delegate?.urlBarDidLongPressReloadButton(self, from: button)
    }

    func tabLocationViewDidTapReaderMode(_ tabLocationView: TabLocationView) {
        delegate?.urlBarDidPressReaderMode(self)
    }

    func tabLocationViewLocationAccessibilityActions(_ tabLocationView: TabLocationView) -> [UIAccessibilityCustomAction]? {
        return delegate?.urlBarLocationAccessibilityActions(self)
    }
    
    func tabLocationViewDidBeginDragInteraction(_ tabLocationView: TabLocationView) {
        delegate?.urlBarDidBeginDragInteraction(self)
    }
}

// MARK: - AutocompleteTextFieldDelegate

extension URLBarView: AutocompleteTextFieldDelegate {
    func autocompleteTextFieldShouldReturn(_ autocompleteTextField: AutocompleteTextField) -> Bool {
        guard let text = locationTextField?.text else { return true }
        if !text.trimmingCharacters(in: .whitespaces).isEmpty {
            delegate?.urlBar(self, didSubmitText: text)
            return true
        } else {
            return false
        }
    }
    
    func autocompleteTextField(_ autocompleteTextField: AutocompleteTextField, didEnterText text: String) {
        delegate?.urlBar(self, didEnterText: text)
    }
    
    func autocompleteTextFieldDidBeginEditing(_ autocompleteTextField: AutocompleteTextField) {
        autocompleteTextField.highlightAll()
    }
    
    func autocompleteTextFieldShouldClear(_ autocompleteTextField: AutocompleteTextField) -> Bool {
        delegate?.urlBar(self, didEnterText: "")
        return true
    }
    
    func autocompleteTextFieldDidCancel(_ autocompleteTextField: AutocompleteTextField) {
        leaveOverlayMode(didCancel: true)
    }
}

// MARK: - Themeable

extension URLBarView: Themeable {
    
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

