/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import Shared
import SnapKit
import Preferences
import Combine
import BraveCore
import DesignSystem

protocol TabLocationViewDelegate {
  func tabLocationViewDidTapLocation(_ tabLocationView: TabLocationView)
  func tabLocationViewDidLongPressLocation(_ tabLocationView: TabLocationView)
  func tabLocationViewDidTapReaderMode(_ tabLocationView: TabLocationView)
  func tabLocationViewDidBeginDragInteraction(_ tabLocationView: TabLocationView)
  func tabLocationViewDidTapPlaylist(_ tabLocationView: TabLocationView)
  func tabLocationViewDidTapLockImageView(_ tabLocationView: TabLocationView)
  func tabLocationViewDidTapReload(_ tabLocationView: TabLocationView)
  func tabLocationViewDidLongPressReload(_ tabLocationView: TabLocationView, from button: UIButton)
  func tabLocationViewDidTapStop(_ tabLocationView: TabLocationView)
  func tabLocationViewDidTapShieldsButton(_ urlBar: TabLocationView)
  func tabLocationViewDidTapRewardsButton(_ urlBar: TabLocationView)
  func tabLocationViewDidLongPressRewardsButton(_ urlBar: TabLocationView)
  func tabLocationViewDidTapWalletButton(_ urlBar: TabLocationView)
  
  /// - returns: whether the long-press was handled by the delegate; i.e. return `false` when the conditions for even starting handling long-press were not satisfied
  @discardableResult func tabLocationViewDidLongPressReaderMode(_ tabLocationView: TabLocationView) -> Bool
}

private struct TabLocationViewUX {
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
  private var privateModeCancellable: AnyCancellable?

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
        reloadButton.setImage(UIImage(systemName: "xmark"), for: .normal)
        reloadButton.accessibilityLabel = Strings.tabToolbarStopButtonAccessibilityLabel
      } else {
        reloadButton.setImage(UIImage(braveSystemNamed: "brave.refresh"), for: .normal)
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
      lockImageView.setImage(UIImage(braveSystemNamed: "brave.exclamationmark.circle.fill")?
        .withRenderingMode(.alwaysOriginal)
        .withTintColor(.braveErrorLabel), for: .normal)
      lockImageView.accessibilityLabel = Strings.tabToolbarWarningImageAccessibilityLabel
    case .secure, .unknown:
      lockImageView.setImage(UIImage(braveSystemNamed: "brave.lock.alt", compatibleWith: nil), for: .normal)
      lockImageView.accessibilityLabel = Strings.tabToolbarLockImageAccessibilityLabel
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
    set(newReaderModeState) {
      if newReaderModeState != self.readerModeButton.readerModeState {
        let wasHidden = readerModeButton.isHidden
        self.readerModeButton.readerModeState = newReaderModeState
        if wasHidden != (newReaderModeState == ReaderModeState.unavailable) {
          UIAccessibility.post(notification: .layoutChanged, argument: nil)
          if !readerModeButton.isHidden {
            // Delay the Reader Mode accessibility announcement briefly to prevent interruptions.
            DispatchQueue.main.asyncAfter(deadline: .now() + .seconds(2)) {
              UIAccessibility.post(notification: .announcement, argument: Strings.readerModeAvailableVoiceOverAnnouncement)
            }
          }
        }
        UIView.animate(
          withDuration: 0.1,
          animations: { () -> Void in
            self.readerModeButton.alpha = newReaderModeState == .unavailable ? 0 : 1
          })
      }
    }
  }

  lazy var placeholder: NSAttributedString = {
    return NSAttributedString(string: Strings.tabToolbarSearchAddressPlaceholderText, attributes: [NSAttributedString.Key.foregroundColor: UIColor.secondaryBraveLabel])
  }()

  lazy var urlTextField: UITextField = {
    let urlTextField = DisplayTextField()

    // Prevent the field from compressing the toolbar buttons on the 4S in landscape.
    urlTextField.setContentCompressionResistancePriority(UILayoutPriority(rawValue: 250), for: .horizontal)
    urlTextField.setContentCompressionResistancePriority(.required, for: .vertical)
    urlTextField.attributedPlaceholder = self.placeholder
    urlTextField.accessibilityIdentifier = "url"
    urlTextField.font = .preferredFont(forTextStyle: .body)
    urlTextField.backgroundColor = .clear
    urlTextField.clipsToBounds = true
    urlTextField.textColor = .braveLabel
    urlTextField.isEnabled = false
    urlTextField.defaultTextAttributes = {
      var attributes = urlTextField.defaultTextAttributes
      let style = (attributes[.paragraphStyle, default: NSParagraphStyle.default] as! NSParagraphStyle).mutableCopy() as! NSMutableParagraphStyle // swiftlint:disable:this force_cast
      style.lineBreakMode = .byClipping
      attributes[.paragraphStyle] = style
      return attributes
    }()
    // Remove the default drop interaction from the URL text field so that our
    // custom drop interaction on the BVC can accept dropped URLs.
    if let dropInteraction = urlTextField.textDropInteraction {
      urlTextField.removeInteraction(dropInteraction)
    }

    return urlTextField
  }()

  private(set) lazy var lockImageView = ToolbarButton(top: true).then {
    $0.setImage(UIImage(braveSystemNamed: "brave.lock.alt", compatibleWith: nil), for: .normal)
    $0.isHidden = true
    $0.tintColor = .braveLabel
    $0.isAccessibilityElement = true
    $0.imageView?.contentMode = .center
    $0.contentHorizontalAlignment = .center
    $0.accessibilityLabel = Strings.tabToolbarLockImageAccessibilityLabel
    $0.setContentHuggingPriority(.defaultHigh, for: .horizontal)
    $0.addTarget(self, action: #selector(didTapLockImageView), for: .touchUpInside)
  }

  private(set) lazy var readerModeButton: ReaderModeButton = {
    let readerModeButton = ReaderModeButton(frame: .zero)
    readerModeButton.addTarget(self, action: #selector(tapReaderModeButton), for: .touchUpInside)
    readerModeButton.addGestureRecognizer(UILongPressGestureRecognizer(target: self, action: #selector(longPressReaderModeButton)))
    readerModeButton.isAccessibilityElement = true
    readerModeButton.isHidden = true
    readerModeButton.imageView?.contentMode = .scaleAspectFit
    readerModeButton.accessibilityLabel = Strings.tabToolbarReaderViewButtonAccessibilityLabel
    readerModeButton.accessibilityIdentifier = "TabLocationView.readerModeButton"
    readerModeButton.accessibilityCustomActions = [UIAccessibilityCustomAction(name: Strings.tabToolbarReaderViewButtonTitle, target: self, selector: #selector(readerModeCustomAction))]
    readerModeButton.unselectedTintColor = .braveLabel
    readerModeButton.selectedTintColor = .braveBlurpleTint
    return readerModeButton
  }()

  private(set) lazy var playlistButton = PlaylistURLBarButton(frame: .zero).then {
    $0.accessibilityIdentifier = "TabToolbar.playlistButton"
    $0.isAccessibilityElement = true
    $0.accessibilityLabel = Strings.tabToolbarPlaylistButtonAccessibilityLabel
    $0.buttonState = .none
    $0.tintColor = .white
    $0.addTarget(self, action: #selector(didClickPlaylistButton), for: .touchUpInside)
  }
  
  private(set) lazy var walletButton = WalletURLBarButton(frame: .zero).then {
    $0.accessibilityIdentifier = "TabToolbar.walletButton"
    $0.isAccessibilityElement = true
    $0.buttonState = .inactive
    $0.addTarget(self, action: #selector(tappedWalletButton), for: .touchUpInside)
  }
  
  lazy var reloadButton = ToolbarButton(top: true).then {
    $0.accessibilityIdentifier = "TabToolbar.stopReloadButton"
    $0.isAccessibilityElement = true
    $0.accessibilityLabel = Strings.tabToolbarReloadButtonAccessibilityLabel
    $0.setImage(UIImage(braveSystemNamed: "brave.refresh", compatibleWith: nil), for: .normal)
    $0.tintColor = .braveLabel
    let longPressGestureStopReloadButton = UILongPressGestureRecognizer(target: self, action: #selector(didLongPressStopReload(_:)))
    $0.addGestureRecognizer(longPressGestureStopReloadButton)
    $0.addTarget(self, action: #selector(didClickStopReload), for: .touchUpInside)
  }

  lazy var shieldsButton: ToolbarButton = {
    let button = ToolbarButton(top: true)
    button.setImage(UIImage(sharedNamed: "brave.logo"), for: .normal)
    button.addTarget(self, action: #selector(didClickBraveShieldsButton), for: .touchUpInside)
    button.imageView?.contentMode = .scaleAspectFit
    button.accessibilityLabel = Strings.bravePanel
    button.imageView?.adjustsImageSizeForAccessibilityContentSizeCategory = true
    button.accessibilityIdentifier = "urlBar-shieldsButton"
    return button
  }()

  lazy var rewardsButton: RewardsButton = {
    let button = RewardsButton()
    button.addTarget(self, action: #selector(didClickBraveRewardsButton), for: .touchUpInside)
    let longPressGestureRewardsButton = UILongPressGestureRecognizer(target: self, action: #selector(didLongPressRewardsButton(_:)))
    button.addGestureRecognizer(longPressGestureRewardsButton)
    return button
  }()

  lazy var separatorLine: UIView = CustomSeparatorView(lineSize: .init(width: 1, height: 26), cornerRadius: 2).then {
    $0.isUserInteractionEnabled = false
    $0.backgroundColor = .braveSeparator
    $0.layoutMargins = UIEdgeInsets(top: 0, left: 2, bottom: 0, right: 2)
  }

  lazy var tabOptionsStackView = UIStackView().then {
    $0.alignment = .center
    $0.layoutMargins = UIEdgeInsets(top: 0, left: 0, bottom: 0, right: 3)
    $0.isLayoutMarginsRelativeArrangement = true
    $0.insetsLayoutMarginsFromSafeArea = false
  }

  override init(frame: CGRect) {
    super.init(frame: frame)

    backgroundColor = .braveBackground

    self.tabObservers = registerFor(.didChangeContentBlocking, .didGainFocus, queue: .main)

    longPressRecognizer = UILongPressGestureRecognizer(target: self, action: #selector(longPressLocation))
    longPressRecognizer.delegate = self

    tapRecognizer = UITapGestureRecognizer(target: self, action: #selector(tapLocation))
    tapRecognizer.delegate = self

    addGestureRecognizer(longPressRecognizer)
    addGestureRecognizer(tapRecognizer)
    
    let optionSubviews = [readerModeButton, walletButton, playlistButton, reloadButton, separatorLine, shieldsButton, rewardsButton]
    optionSubviews.forEach {
      ($0 as? UIButton)?.contentEdgeInsets = UIEdgeInsets(top: 0, left: 5, bottom: 0, right: 5)
      $0.setContentCompressionResistancePriority(.defaultHigh, for: .horizontal)
      $0.setContentHuggingPriority(.defaultHigh, for: .horizontal)
      tabOptionsStackView.addArrangedSubview($0)
    }

    // Visual centering
    rewardsButton.contentEdgeInsets = .init(top: 1, left: 5, bottom: 1, right: 5)
    playlistButton.contentEdgeInsets = .init(top: 2, left: 10, bottom: 2, right: 6)
    
    urlTextField.setContentHuggingPriority(.defaultLow, for: .horizontal)

    let subviews = [lockImageView, urlTextField, tabOptionsStackView]
    contentView = UIStackView(arrangedSubviews: subviews)
    contentView.layoutMargins = UIEdgeInsets(top: 2, left: TabLocationViewUX.spacing, bottom: 2, right: 0)
    contentView.isLayoutMarginsRelativeArrangement = true
    contentView.insetsLayoutMarginsFromSafeArea = false
    contentView.spacing = 8
    contentView.setCustomSpacing(4, after: urlTextField)
    addSubview(contentView)

    contentView.snp.makeConstraints { make in
      make.leading.trailing.top.bottom.equalTo(self)
    }

    // Setup UIDragInteraction to handle dragging the location
    // bar for dropping its URL into other apps.
    let dragInteraction = UIDragInteraction(delegate: self)
    dragInteraction.allowsSimultaneousRecognitionDuringLift = true
    self.addInteraction(dragInteraction)
    
    privateModeCancellable = PrivateBrowsingManager.shared.$isPrivateBrowsing
      .removeDuplicates()
      .sink(receiveValue: { [weak self] isPrivateBrowsing in
        self?.updateColors(isPrivateBrowsing)
      })
    
    updateForTraitCollection()
  }

  required init(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }
  
  override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
    super.traitCollectionDidChange(previousTraitCollection)
    if previousTraitCollection?.preferredContentSizeCategory != traitCollection.preferredContentSizeCategory {
      updateForTraitCollection()
    }
  }
  
  private func updateForTraitCollection() {
    let clampedTraitCollection = traitCollection.clampingSizeCategory(maximum: .accessibilityLarge)
    lockImageView.setPreferredSymbolConfiguration(
      .init(pointSize: UIFont.preferredFont(forTextStyle: .body, compatibleWith: clampedTraitCollection).pointSize, weight: .heavy, scale: .small),
      forImageIn: .normal
    )
    let toolbarTraitCollection = UITraitCollection(preferredContentSizeCategory: traitCollection.toolbarButtonContentSizeCategory)
    urlTextField.font = .preferredFont(forTextStyle: .body, compatibleWith: clampedTraitCollection)
    let pointSize = UIFont.preferredFont(
      forTextStyle: .footnote,
      compatibleWith: toolbarTraitCollection
    ).pointSize
    reloadButton.setPreferredSymbolConfiguration(
      .init(pointSize: pointSize, weight: .regular, scale: .large),
      forImageIn: .normal
    )
    reloadButton.snp.remakeConstraints {
      $0.width.equalTo(UIFontMetrics(forTextStyle: .body).scaledValue(for: 32, compatibleWith: toolbarTraitCollection))
    }
  }
  
  private func updateColors(_ isPrivateBrowsing: Bool) {
    if isPrivateBrowsing {
      overrideUserInterfaceStyle = .dark
      backgroundColor = .braveBackground.resolvedColor(with: .init(userInterfaceStyle: .dark))
    } else {
      overrideUserInterfaceStyle = DefaultTheme(
        rawValue: Preferences.General.themeNormalMode.value)?.userInterfaceStyleOverride ?? .unspecified
      backgroundColor = .braveBackground
    }
  }

  override var accessibilityElements: [Any]? {
    get {
      return [lockImageView, urlTextField, readerModeButton, playlistButton, reloadButton, shieldsButton].filter { !$0.isHidden }
    }
    set {
      super.accessibilityElements = newValue
    }
  }

  @objc func didTapLockImageView() {
    if !loading {
      delegate?.tabLocationViewDidTapLockImageView(self)
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

  @objc func didClickPlaylistButton() {
    delegate?.tabLocationViewDidTapPlaylist(self)
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

  @objc func didLongPressRewardsButton(_ gesture: UILongPressGestureRecognizer) {
    if gesture.state == .began {
      delegate?.tabLocationViewDidLongPressRewardsButton(self)
    }
  }
  
  @objc func tappedWalletButton() {
    delegate?.tabLocationViewDidTapWalletButton(self)
  }
  
  fileprivate func updateTextWithURL() {
    (urlTextField as? DisplayTextField)?.hostString = url?.withoutWWW.host ?? ""
    
    // Note: Only use `URLFormatter.formatURLOrigin(forSecurityDisplay: url?.withoutWWW.absoluteString ?? "", schemeDisplay: .omitHttpAndHttps)`
    // If displaying the host ONLY! This follows Google Chrome and Safari.
    // However, for Brave as no decision has been made on what shows YET, we will display the entire URL (truncated!)
    // Therefore we only omit defaults (username & password, http [not https], and trailing slash) + omit "www".
    // We must NOT un-escape the URL!
    // --
    // The requirement to remove scheme comes from Desktop. Also we do not remove the path like in other browsers either.
    // Therefore, we follow Brave Desktop instead of Chrome or Safari iOS
    urlTextField.text = URLFormatter.formatURL(url?.withoutWWW.absoluteString ?? "", formatTypes: [.omitDefaults], unescapeOptions: []).removeSchemeFromURLString(url?.scheme)
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
    guard let url = self.url, !InternalURL.isValid(url: url), let itemProvider = NSItemProvider(contentsOf: url),
      !reloadButton.isHighlighted
    else {
      return []
    }

    let dragItem = UIDragItem(itemProvider: itemProvider)
    return [dragItem]
  }

  func dragInteraction(_ interaction: UIDragInteraction, sessionWillBegin session: UIDragSession) {
    delegate?.tabLocationViewDidBeginDragInteraction(self)
  }
}

// MARK: - TabEventHandler

extension TabLocationView: TabEventHandler {
  func tabDidGainFocus(_ tab: Tab) {
  }

  func tabDidChangeContentBlockerStatus(_ tab: Tab) {
  }
}

// MARK: - Hit Test
extension TabLocationView {
  override func hitTest(_ point: CGPoint, with event: UIEvent?) -> UIView? {
    if lockImageView.frame.insetBy(dx: -10, dy: -30).contains(point) {
      return lockImageView
    }
    return super.hitTest(point, with: event)
  }
}

class DisplayTextField: UITextField {
  weak var accessibilityActionsSource: AccessibilityActionsSource?
  var hostString: String = ""
  let pathPadding: CGFloat = 5.0
  
  private let leadingClippingFade = GradientView(
    colors: [.braveBackground, .braveBackground.withAlphaComponent(0.0)],
    positions: [0, 1],
    startPoint: .init(x: 0, y: 0.5),
    endPoint: .init(x: 1, y: 0.5)
  )
  
  override init(frame: CGRect) {
    super.init(frame: frame)
    
    addSubview(leadingClippingFade)
    leadingClippingFade.snp.makeConstraints {
      $0.leading.top.bottom.equalToSuperview()
      $0.width.equalTo(20)
    }
  }
  
  override var text: String? {
    didSet {
      leadingClippingFade.isHidden = true
    }
  }
  
  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
  
  override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
    super.traitCollectionDidChange(previousTraitCollection)
    leadingClippingFade.gradientLayer.colors = [
      UIColor.braveBackground,
      UIColor.braveBackground.withAlphaComponent(0.0)
    ].map {
      $0.resolvedColor(with: traitCollection).cgColor
    }
  }

  override var accessibilityCustomActions: [UIAccessibilityCustomAction]? {
    get {
      return accessibilityActionsSource?.accessibilityCustomActionsForView(self)
    }
    set {
      super.accessibilityCustomActions = newValue
    }
  }
  
  override var accessibilityTraits: UIAccessibilityTraits {
    get { [.staticText, .button] }
    set { }
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
        bringSubviewToFront(leadingClippingFade)
        leadingClippingFade.isHidden = false
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
    innerView.layer.cornerCurve = .continuous
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
