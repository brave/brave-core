/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import Shared
import SnapKit
import XCGLogger
import BraveShared

private let log = Logger.browserLogger

protocol TabLocationViewDelegate {
    func tabLocationViewDidTapLocation(_ tabLocationView: TabLocationView)
    func tabLocationViewDidLongPressLocation(_ tabLocationView: TabLocationView)
    func tabLocationViewDidTapReaderMode(_ tabLocationView: TabLocationView)
    func tabLocationViewDidBeginDragInteraction(_ tabLocationView: TabLocationView)
    func tabLocationViewDidTapReload(_ tabLocationView: TabLocationView)
    func tabLocationViewDidLongPressReload(_ tabLocationView: TabLocationView, from button: UIButton)
    func tabLocationViewDidTapStop(_ tabLocationView: TabLocationView)
    func tabLocationViewDidTapShieldsButton(_ urlBar: TabLocationView)
    func tabLocationViewDidTapRewardsButton(_ urlBar: TabLocationView)
    
    /// - returns: whether the long-press was handled by the delegate; i.e. return `false` when the conditions for even starting handling long-press were not satisfied
    @discardableResult func tabLocationViewDidLongPressReaderMode(_ tabLocationView: TabLocationView) -> Bool
    func tabLocationViewLocationAccessibilityActions(_ tabLocationView: TabLocationView) -> [UIAccessibilityCustomAction]?
}

private struct TabLocationViewUX {
    static let hostFontColor = UIColor.black
    static let baseURLFontColor = UIColor.Photon.grey50
    static let spacing: CGFloat = 8
    static let statusIconSize: CGFloat = 18
    static let TPIconSize: CGFloat = 24
    static let buttonSize = CGSize(width: 44, height: 34.0)
    static let URLBarPadding = 4
}

class TabLocationView: UIView {
    var delegate: TabLocationViewDelegate?
    var longPressRecognizer: UILongPressGestureRecognizer!
    var tapRecognizer: UITapGestureRecognizer!
    var contentView: UIStackView!
    private var tabObservers: TabObservers!

    @objc dynamic var baseURLFontColor: UIColor = TabLocationViewUX.baseURLFontColor {
        didSet { updateTextWithURL() }
    }

    var url: URL? {
        didSet {
            updateLockImageView()
            updateTextWithURL()
            setNeedsUpdateConstraints()
        }
    }

    var secureContentState: TabSecureContentState = .unknown {
        didSet {
            updateLockImageView()
        }
    }
    
    var loading: Bool = false {
        didSet {
            if loading {
                reloadButton.setImage(#imageLiteral(resourceName: "nav-stop").template, for: .normal)
                reloadButton.accessibilityLabel = Strings.tabToolbarStopButtonAccessibilityLabel
            } else {
                reloadButton.setImage(#imageLiteral(resourceName: "nav-refresh").template, for: .normal)
                reloadButton.accessibilityLabel = Strings.tabToolbarReloadButtonAccessibilityLabel
            }
        }
    }
    
    private func updateLockImageView() {
        lockImageView.isHidden = false
        
        switch secureContentState {
        case .localHost:
            lockImageView.isHidden = true
        case .insecure:
            lockImageView.tintColor = .red
        case .secure, .unknown:
            lockImageView.tintColor = #colorLiteral(red: 0.3764705882, green: 0.3843137255, blue: 0.4, alpha: 1)
        }
    }

    deinit {
        NotificationCenter.default.removeObserver(self)
        unregister(tabObservers)
    }

    var readerModeState: ReaderModeState {
        get {
            return readerModeButton.readerModeState
        }
        set (newReaderModeState) {
            if newReaderModeState != self.readerModeButton.readerModeState {
                let wasHidden = readerModeButton.isHidden
                self.readerModeButton.readerModeState = newReaderModeState
                readerModeButton.isHidden = (newReaderModeState == ReaderModeState.unavailable)
                if wasHidden != readerModeButton.isHidden {
                    UIAccessibility.post(notification: .layoutChanged, argument: nil)
                    if !readerModeButton.isHidden {
                        // Delay the Reader Mode accessibility announcement briefly to prevent interruptions.
                        DispatchQueue.main.asyncAfter(deadline: .now() + .seconds(2)) {
                            UIAccessibility.post(notification: .announcement, argument: Strings.readerModeAvailableVoiceOverAnnouncement)
                        }
                    }
                }
                UIView.animate(withDuration: 0.1, animations: { () -> Void in
                    self.readerModeButton.alpha = newReaderModeState == .unavailable ? 0 : 1
                })
            }
        }
    }

    lazy var placeholder: NSAttributedString = {
        return NSAttributedString(string: Strings.tabToolbarSearchAddressPlaceholderText, attributes: [NSAttributedString.Key.foregroundColor: UIColor.Photon.grey40])
    }()

    lazy var urlTextField: UITextField = {
        let urlTextField = DisplayTextField()

        // Prevent the field from compressing the toolbar buttons on the 4S in landscape.
        urlTextField.setContentCompressionResistancePriority(UILayoutPriority(rawValue: 250), for: .horizontal)
        urlTextField.attributedPlaceholder = self.placeholder
        urlTextField.accessibilityIdentifier = "url"
        urlTextField.accessibilityActionsSource = self
        urlTextField.font = UIConstants.defaultChromeFont
        urlTextField.backgroundColor = .clear
        urlTextField.clipsToBounds = true
        // Remove the default drop interaction from the URL text field so that our
        // custom drop interaction on the BVC can accept dropped URLs.
        if let dropInteraction = urlTextField.textDropInteraction {
            urlTextField.removeInteraction(dropInteraction)
        }

        return urlTextField
    }()

    fileprivate lazy var lockImageView: UIImageView = {
        let lockImageView = UIImageView(image: #imageLiteral(resourceName: "lock_verified").template)
        lockImageView.isHidden = true
        lockImageView.tintColor = #colorLiteral(red: 0.3764705882, green: 0.3843137255, blue: 0.4, alpha: 1)
        lockImageView.isAccessibilityElement = true
        lockImageView.contentMode = .center
        lockImageView.accessibilityLabel = Strings.tabToolbarLockImageAccessibilityLabel
        lockImageView.setContentHuggingPriority(.defaultHigh, for: .horizontal)
        return lockImageView
    }()

    fileprivate lazy var readerModeButton: ReaderModeButton = {
        let readerModeButton = ReaderModeButton(frame: .zero)
        readerModeButton.addTarget(self, action: #selector(tapReaderModeButton), for: .touchUpInside)
        readerModeButton.addGestureRecognizer(UILongPressGestureRecognizer(target: self, action: #selector(longPressReaderModeButton)))
        readerModeButton.isAccessibilityElement = true
        readerModeButton.isHidden = true
        readerModeButton.imageView?.contentMode = .scaleAspectFit
        readerModeButton.contentHorizontalAlignment = .center
        readerModeButton.accessibilityLabel = Strings.tabToolbarReaderViewButtonAccessibilityLabel
        readerModeButton.accessibilityIdentifier = "TabLocationView.readerModeButton"
        readerModeButton.accessibilityCustomActions = [UIAccessibilityCustomAction(name: Strings.tabToolbarReaderViewButtonTitle, target: self, selector: #selector(readerModeCustomAction))]
        return readerModeButton
    }()
    
    lazy var reloadButton = ToolbarButton(top: true).then {
        $0.accessibilityIdentifier = "TabToolbar.stopReloadButton"
        $0.isAccessibilityElement = true
        $0.accessibilityLabel = Strings.tabToolbarReloadButtonAccessibilityLabel
        $0.setImage(#imageLiteral(resourceName: "nav-refresh").template, for: .normal)
        $0.tintColor = UIColor.Photon.grey30
        let longPressGestureStopReloadButton = UILongPressGestureRecognizer(target: self, action: #selector(didLongPressStopReload(_:)))
        $0.addGestureRecognizer(longPressGestureStopReloadButton)
        $0.addTarget(self, action: #selector(didClickStopReload), for: .touchUpInside)
    }

    lazy var shieldsButton: ToolbarButton = {
        let button = ToolbarButton(top: true)
        button.setImage(UIImage(imageLiteralResourceName: "shields-menu-icon"), for: .normal)
        button.addTarget(self, action: #selector(didClickBraveShieldsButton), for: .touchUpInside)
        button.imageView?.contentMode = .center
        button.accessibilityLabel = Strings.bravePanel
        button.accessibilityIdentifier = "urlBar-shieldsButton"
        return button
    }()
    
    lazy var rewardsButton: RewardsButton = {
        let button = RewardsButton()
        button.addTarget(self, action: #selector(didClickBraveRewardsButton), for: .touchUpInside)
        return button
    }()
    
    lazy var separatorLine: UIView = CustomSeparatorView(lineSize: .init(width: 1, height: 26), cornerRadius: 2)
    
    lazy var tabOptionsStackView = UIStackView()

    override init(frame: CGRect) {
        super.init(frame: frame)

        self.tabObservers = registerFor(.didChangeContentBlocking, .didGainFocus, queue: .main)

        longPressRecognizer = UILongPressGestureRecognizer(target: self, action: #selector(longPressLocation))
        longPressRecognizer.delegate = self

        tapRecognizer = UITapGestureRecognizer(target: self, action: #selector(tapLocation))
        tapRecognizer.delegate = self

        addGestureRecognizer(longPressRecognizer)
        addGestureRecognizer(tapRecognizer)
        
        var optionSubviews = [readerModeButton, reloadButton, separatorLine, shieldsButton]
        separatorLine.isUserInteractionEnabled = false
        
        optionSubviews.append(rewardsButton)
        
        let buttonContentEdgeInsets = UIEdgeInsets(top: 0, left: 5, bottom: 0, right: 5)
        optionSubviews.forEach {
            ($0 as? CustomSeparatorView)?.layoutMargins = UIEdgeInsets(top: 0, left: 2, bottom: 0, right: 2)
            ($0 as? UIButton)?.contentEdgeInsets = buttonContentEdgeInsets
            $0.setContentCompressionResistancePriority(.defaultHigh, for: .horizontal)
            $0.setContentHuggingPriority(.defaultHigh, for: .horizontal)
        }
        optionSubviews.forEach(tabOptionsStackView.addArrangedSubview)
        
        urlTextField.setContentHuggingPriority(.defaultLow, for: .horizontal)
        
        let subviews = [lockImageView, urlTextField, tabOptionsStackView]
        contentView = UIStackView(arrangedSubviews: subviews)
        contentView.distribution = .fill
        contentView.alignment = .center
        contentView.layoutMargins = UIEdgeInsets(top: 0, left: TabLocationViewUX.spacing, bottom: 0, right: 0)
        contentView.isLayoutMarginsRelativeArrangement = true
        contentView.insetsLayoutMarginsFromSafeArea = false
        contentView.spacing = 10
        contentView.setCustomSpacing(5, after: urlTextField)
        
        tabOptionsStackView.layoutMargins = UIEdgeInsets(top: 0, left: 0, bottom: 0, right: 6)
        tabOptionsStackView.isLayoutMarginsRelativeArrangement = true
        addSubview(contentView)
        
        contentView.snp.makeConstraints { make in
            make.leading.trailing.top.bottom.equalTo(self)
        }
        
        tabOptionsStackView.snp.makeConstraints { make in
            make.top.bottom.equalTo(contentView)
        }

        // Setup UIDragInteraction to handle dragging the location
        // bar for dropping its URL into other apps.
        let dragInteraction = UIDragInteraction(delegate: self)
        dragInteraction.allowsSimultaneousRecognitionDuringLift = true
        self.addInteraction(dragInteraction)
    }

    required init(coder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }

    override var accessibilityElements: [Any]? {
        get {
            return [lockImageView, urlTextField, readerModeButton, reloadButton, shieldsButton].filter { !$0.isHidden }
        }
        set {
            super.accessibilityElements = newValue
        }
    }

    @objc func tapReaderModeButton() {
        delegate?.tabLocationViewDidTapReaderMode(self)
    }

    @objc func longPressReaderModeButton(_ recognizer: UILongPressGestureRecognizer) {
        if recognizer.state == .began {
            delegate?.tabLocationViewDidLongPressReaderMode(self)
        }
    }
    
    @objc func didClickStopReload() {
        if loading {
            delegate?.tabLocationViewDidTapStop(self)
        } else {
            delegate?.tabLocationViewDidTapReload(self)
        }
    }
    
    @objc func didLongPressStopReload(_ recognizer: UILongPressGestureRecognizer) {
        if recognizer.state == .began && !loading {
            delegate?.tabLocationViewDidLongPressReload(self, from: reloadButton)
        }
    }
    
    @objc func longPressLocation(_ recognizer: UITapGestureRecognizer) {
        if recognizer.state == .began {
            delegate?.tabLocationViewDidLongPressLocation(self)
        }
    }

    @objc func tapLocation(_ recognizer: UITapGestureRecognizer) {
        delegate?.tabLocationViewDidTapLocation(self)
    }

    @objc func readerModeCustomAction() -> Bool {
        return delegate?.tabLocationViewDidLongPressReaderMode(self) ?? false
    }
    
    @objc func didClickBraveShieldsButton() {
        delegate?.tabLocationViewDidTapShieldsButton(self)
    }
    
    @objc func didClickBraveRewardsButton() {
        delegate?.tabLocationViewDidTapRewardsButton(self)
    }

    fileprivate func updateTextWithURL() {
        (urlTextField as? DisplayTextField)?.hostString = url?.host ?? ""
        urlTextField.text = url?.withoutWWW.schemelessAbsoluteString.trim("/")
    }
}

// MARK: - UIGestureRecognizerDelegate

extension TabLocationView: UIGestureRecognizerDelegate {
    func gestureRecognizer(_ gestureRecognizer: UIGestureRecognizer, shouldRecognizeSimultaneouslyWith otherGestureRecognizer: UIGestureRecognizer) -> Bool {
        // When long pressing a button make sure the textfield's long press gesture is not triggered
        return !(otherGestureRecognizer.view is UIButton)
    }

    func gestureRecognizer(_ gestureRecognizer: UIGestureRecognizer, shouldBeRequiredToFailBy otherGestureRecognizer: UIGestureRecognizer) -> Bool {
        // If the longPressRecognizer is active, fail the tap recognizer to avoid conflicts.
        return gestureRecognizer == longPressRecognizer && otherGestureRecognizer == tapRecognizer
    }
    
    func gestureRecognizer(_ gestureRecognizer: UIGestureRecognizer, shouldReceive touch: UITouch) -> Bool {
        if gestureRecognizer == tapRecognizer && touch.view == tabOptionsStackView {
            return false
        }
        return true
    }
}

// MARK: - UIDragInteractionDelegate

extension TabLocationView: UIDragInteractionDelegate {
    func dragInteraction(_ interaction: UIDragInteraction, itemsForBeginning session: UIDragSession) -> [UIDragItem] {
        // Ensure we actually have a URL in the location bar and that the URL is not local.
        guard let url = self.url, !url.isLocal, let itemProvider = NSItemProvider(contentsOf: url),
            !reloadButton.isHighlighted else {
            return []
        }

        let dragItem = UIDragItem(itemProvider: itemProvider)
        return [dragItem]
    }

    func dragInteraction(_ interaction: UIDragInteraction, sessionWillBegin session: UIDragSession) {
        delegate?.tabLocationViewDidBeginDragInteraction(self)
    }
}

// MARK: - AccessibilityActionsSource

extension TabLocationView: AccessibilityActionsSource {
    func accessibilityCustomActionsForView(_ view: UIView) -> [UIAccessibilityCustomAction]? {
        if view === urlTextField {
            return delegate?.tabLocationViewLocationAccessibilityActions(self)
        }
        return nil
    }
}

// MARK: - Themeable

extension TabLocationView: Themeable {
    var themeableChildren: [Themeable?]? {
        return [reloadButton]
    }
    
    func applyTheme(_ theme: Theme) {
        styleChildren(theme: theme)
        
        backgroundColor = theme.colors.addressBar.withAlphaComponent(theme.colors.transparencies.addressBarAlpha)
        
        urlTextField.textColor = theme.colors.tints.addressBar

        readerModeButton.unselectedTintColor = theme.colors.tints.header
        readerModeButton.selectedTintColor = theme.colors.accent

        separatorLine.backgroundColor = theme.colors.border.withAlphaComponent(theme.colors.transparencies.borderAlpha)
    }
}

// MARK: - TabEventHandler

extension TabLocationView: TabEventHandler {
    func tabDidGainFocus(_ tab: Tab) {
    }

    func tabDidChangeContentBlockerStatus(_ tab: Tab) {
    }
}

class DisplayTextField: UITextField {
    weak var accessibilityActionsSource: AccessibilityActionsSource?
    var hostString: String = ""
    let pathPadding: CGFloat = 5.0

    override var accessibilityCustomActions: [UIAccessibilityCustomAction]? {
        get {
            return accessibilityActionsSource?.accessibilityCustomActionsForView(self)
        }
        set {
            super.accessibilityCustomActions = newValue
        }
    }

    override var canBecomeFirstResponder: Bool {
        return false
    }
    
    // This override is done in case the eTLD+1 string overflows the width of textField.
    // In that case the textRect is adjusted to show right aligned and truncate left.
    // Since this textField changes with WebView domain change, performance implications are low.
    override func textRect(forBounds bounds: CGRect) -> CGRect {
        var rect: CGRect = super.textRect(forBounds: bounds)
        
        if let size: CGSize = (self.hostString as NSString?)?.size(withAttributes: [.font: self.font!]) {
            if size.width > self.bounds.width {
                rect.origin.x = rect.origin.x - (size.width + pathPadding - self.bounds.width)
                rect.size.width = size.width + pathPadding
            }
        }
        return rect
    }
}

private class CustomSeparatorView: UIView {
    
    private let innerView: UIView
    init(lineSize: CGSize, cornerRadius: CGFloat = 0) {
        innerView = UIView(frame: .init(origin: .zero, size: lineSize))
        super.init(frame: .zero)
        backgroundColor = .clear
        innerView.layer.cornerRadius = cornerRadius
        addSubview(innerView)
        innerView.snp.makeConstraints {
            $0.width.height.equalTo(lineSize)
            $0.centerY.equalTo(self)
            $0.leading.trailing.equalTo(self.layoutMarginsGuide)
        }
    }
    
    override var backgroundColor: UIColor? {
        get {
            return innerView.backgroundColor
        }
        set {
            innerView.backgroundColor = newValue
        }
    }
    
    required init?(coder aDecoder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }
}
