// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveStrings
import Combine
import DesignSystem
import NaturalLanguage
import Preferences
import Shared
import SnapKit
import Strings
import UIKit

protocol TabLocationViewDelegate {
  func tabLocationViewDidTapLocation(_ tabLocationView: TabLocationView)
  func tabLocationViewDidTapReaderMode(_ tabLocationView: TabLocationView)
  func tabLocationViewDidBeginDragInteraction(_ tabLocationView: TabLocationView)
  func tabLocationViewDidTapPlaylist(_ tabLocationView: TabLocationView)
  func tabLocationViewDidTapPlaylistMenuAction(
    _ tabLocationView: TabLocationView,
    action: PlaylistURLBarButton.MenuAction
  )
  func tabLocationViewDidTapReload(_ tabLocationView: TabLocationView)
  func tabLocationViewDidTapStop(_ tabLocationView: TabLocationView)
  func tabLocationViewDidTapVoiceSearch(_ tabLocationView: TabLocationView)
  func tabLocationViewDidTapWalletButton(_ urlBar: TabLocationView)
  func tabLocationViewDidTapSecureContentState(_ urlBar: TabLocationView)
}

private struct TabLocationViewUX {
  static let spacing: CGFloat = 8
  static let statusIconSize: CGFloat = 18
  static let buttonSize = CGSize(width: 44, height: 34.0)
  static let progressBarHeight: CGFloat = 3
}

class TabLocationView: UIView {
  var delegate: TabLocationViewDelegate?
  let contentView = UIView()
  private var tabObservers: TabObservers!
  private var privateModeCancellable: AnyCancellable?

  var url: URL? {
    didSet {
      updateLeadingItem()
      updateURLBarWithText()
      setNeedsUpdateConstraints()
    }
  }

  var secureContentState: TabSecureContentState = .unknown {
    didSet {
      updateLeadingItem()
    }
  }

  var loading: Bool = false {
    didSet {
      if loading {
        reloadButton.setImage(UIImage(systemName: "xmark"), for: .normal)
        reloadButton.accessibilityLabel = Strings.tabToolbarStopButtonAccessibilityLabel
      } else {
        reloadButton.setImage(UIImage(braveSystemNamed: "leo.browser.refresh"), for: .normal)
        reloadButton.accessibilityLabel = Strings.tabToolbarReloadButtonAccessibilityLabel
      }
    }
  }

  private var secureContentStateButtonConfiguration: UIButton.Configuration {
    let clampedTraitCollection = traitCollection.clampingSizeCategory(maximum: .accessibilityLarge)
    var configuration = UIButton.Configuration.plain()
    configuration.preferredSymbolConfigurationForImage = .init(
      font: .preferredFont(forTextStyle: .subheadline, compatibleWith: clampedTraitCollection),
      scale: .small
    )
    configuration.buttonSize = .small
    configuration.imagePadding = 4
    // A bit extra on the leading edge for visual spacing
    configuration.contentInsets = .init(top: 0, leading: 12, bottom: 0, trailing: 8)

    var title = AttributedString(Strings.tabToolbarNotSecureTitle)
    title.font = .preferredFont(forTextStyle: .subheadline, compatibleWith: clampedTraitCollection)

    // Hide the title with mixed content due to a WebKit bug (https://bugs.webkit.org/show_bug.cgi?id=258711)
    // which fails to update `hasOnlySecureContent` even when promoting all http content.
    let isTitleVisible =
      !traitCollection.preferredContentSizeCategory.isAccessibilityCategory && bounds.width > 250
      && secureContentState != .mixedContent

    switch secureContentState {
    case .localhost, .secure:
      break
    case .invalidCert:
      configuration.baseForegroundColor = UIColor(braveSystemName: .systemfeedbackErrorIcon)
      if isTitleVisible {
        configuration.attributedTitle = title
      }
      configuration.image = UIImage(braveSystemNamed: "leo.warning.triangle-filled")
    case .missingSSL, .mixedContent:
      configuration.baseForegroundColor = UIColor(braveSystemName: .textTertiary)
      if isTitleVisible {
        configuration.attributedTitle = title
      }
      configuration.image = UIImage(braveSystemNamed: "leo.warning.triangle-filled")
    case .unknown:
      configuration.baseForegroundColor = UIColor(braveSystemName: .iconDefault)
      configuration.image = UIImage(braveSystemNamed: "leo.info.filled")
    }
    return configuration
  }

  private func updateLeadingItem() {
    var leadingView: UIView?
    defer { leadingItemView = leadingView }
    if !secureContentState.shouldDisplayWarning {
      // Consider reader mode
      leadingView = readerModeState != .unavailable ? readerModeButton : nil
      return
    }

    let button = UIButton(
      configuration: secureContentStateButtonConfiguration,
      primaryAction: .init(handler: { [weak self] _ in
        guard let self = self else { return }
        self.delegate?.tabLocationViewDidTapSecureContentState(self)
      })
    )
    button.configurationUpdateHandler = { [unowned self] btn in
      btn.configuration = secureContentStateButtonConfiguration
    }
    button.tintAdjustmentMode = .normal
    secureContentStateButton = button
    leadingView = button
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
      defer { updateLeadingItem() }
      if newReaderModeState != self.readerModeButton.readerModeState {
        let wasHidden = leadingItemView == nil
        self.readerModeButton.readerModeState = newReaderModeState
        if wasHidden != (newReaderModeState == ReaderModeState.unavailable) {
          UIAccessibility.post(notification: .layoutChanged, argument: nil)
          if !readerModeButton.isHidden {
            // Delay the Reader Mode accessibility announcement briefly to prevent interruptions.
            DispatchQueue.main.asyncAfter(deadline: .now() + .seconds(2)) {
              UIAccessibility.post(
                notification: .announcement,
                argument: Strings.readerModeAvailableVoiceOverAnnouncement
              )
            }
          }
        }
        UIView.animate(
          withDuration: 0.1,
          animations: { () -> Void in
            self.readerModeButton.alpha = newReaderModeState == .unavailable ? 0 : 1
          }
        )
      }
    }
  }

  lazy var urlDisplayLabel: UILabel = {
    let urlDisplayLabel = DisplayURLLabel()

    // Prevent the field from compressing the toolbar buttons on the 4S in landscape.
    urlDisplayLabel.setContentCompressionResistancePriority(
      UILayoutPriority(rawValue: 250),
      for: .horizontal
    )
    urlDisplayLabel.setContentCompressionResistancePriority(.required, for: .vertical)
    urlDisplayLabel.accessibilityIdentifier = "url"
    urlDisplayLabel.font = .preferredFont(forTextStyle: .body)
    urlDisplayLabel.backgroundColor = .clear
    urlDisplayLabel.clipsToBounds = true
    urlDisplayLabel.lineBreakMode = .byClipping
    urlDisplayLabel.numberOfLines = 1
    return urlDisplayLabel
  }()

  private(set) lazy var readerModeButton: ReaderModeButton = {
    let readerModeButton = ReaderModeButton(frame: .zero)
    readerModeButton.addTarget(self, action: #selector(didTapReaderModeButton), for: .touchUpInside)
    readerModeButton.isAccessibilityElement = true
    readerModeButton.imageView?.contentMode = .scaleAspectFit
    readerModeButton.accessibilityLabel = Strings.tabToolbarReaderViewButtonAccessibilityLabel
    readerModeButton.accessibilityIdentifier = "TabLocationView.readerModeButton"
    return readerModeButton
  }()

  private(set) lazy var playlistButton = PlaylistURLBarButton(frame: .zero).then {
    $0.accessibilityIdentifier = "TabToolbar.playlistButton"
    $0.isAccessibilityElement = true
    $0.buttonState = .none
    $0.tintColor = .white
    $0.addTarget(self, action: #selector(didTapPlaylistButton), for: .touchUpInside)
  }

  private(set) lazy var walletButton = WalletURLBarButton(frame: .zero).then {
    $0.accessibilityIdentifier = "TabToolbar.walletButton"
    $0.isAccessibilityElement = true
    $0.buttonState = .inactive
    $0.addTarget(self, action: #selector(didTapWalletButton), for: .touchUpInside)
  }

  lazy var reloadButton = ToolbarButton().then {
    $0.accessibilityIdentifier = "TabToolbar.stopReloadButton"
    $0.isAccessibilityElement = true
    $0.accessibilityLabel = Strings.tabToolbarReloadButtonAccessibilityLabel
    $0.setImage(UIImage(braveSystemNamed: "leo.browser.refresh", compatibleWith: nil), for: .normal)
    $0.tintColor = .braveLabel
    $0.addTarget(self, action: #selector(didTapStopReloadButton), for: .touchUpInside)
  }

  private lazy var voiceSearchButton = ToolbarButton().then {
    $0.accessibilityIdentifier = "TabToolbar.voiceSearchButton"
    $0.isAccessibilityElement = true
    $0.accessibilityLabel = Strings.tabToolbarVoiceSearchButtonAccessibilityLabel
    $0.isHidden = !isVoiceSearchAvailable
    $0.setImage(UIImage(braveSystemNamed: "leo.microphone", compatibleWith: nil), for: .normal)
    $0.tintColor = .braveLabel
    $0.addTarget(self, action: #selector(didTapVoiceSearchButton), for: .touchUpInside)
  }

  lazy var trailingTabOptionsStackView = UIStackView().then {
    $0.alignment = .center
    $0.insetsLayoutMarginsFromSafeArea = false
  }

  private var isVoiceSearchAvailable: Bool
  private let privateBrowsingManager: PrivateBrowsingManager

  private let placeholderLabel = UILabel().then {
    $0.text = Strings.tabToolbarSearchAddressPlaceholderText
    $0.setContentHuggingPriority(.defaultHigh, for: .horizontal)
    $0.isHidden = true
    $0.adjustsFontSizeToFitWidth = true
    $0.minimumScaleFactor = 0.5
  }

  // A layout guide defining the available space for the URL itself
  private let urlLayoutGuide = UILayoutGuide().then {
    $0.identifier = "url-layout-guide"
  }

  private let leadingItemContainerView = UIView()
  private var leadingItemView: UIView? {
    willSet {
      leadingItemView?.removeFromSuperview()
    }
    didSet {
      if let leadingItemView {
        leadingItemContainerView.addSubview(leadingItemView)
        leadingItemView.snp.makeConstraints {
          $0.edges.equalToSuperview()
        }
      }
    }
  }

  private(set) var secureContentStateButton: UIButton?

  private(set) lazy var progressBar = GradientProgressBar().then {
    $0.clipsToBounds = false
    $0.setGradientColors(startColor: .braveBlurpleTint, endColor: .braveBlurpleTint)
  }

  init(voiceSearchSupported: Bool, privateBrowsingManager: PrivateBrowsingManager) {
    self.privateBrowsingManager = privateBrowsingManager
    isVoiceSearchAvailable = voiceSearchSupported

    super.init(frame: .zero)

    tabObservers = registerFor(.didChangeContentBlocking, .didGainFocus, queue: .main)

    addGestureRecognizer(UITapGestureRecognizer(target: self, action: #selector(didTapLocationBar)))

    readerModeButton.do {
      $0.setContentCompressionResistancePriority(.defaultHigh, for: .horizontal)
      $0.setContentHuggingPriority(.defaultHigh, for: .horizontal)
    }

    var trailingOptionSubviews: [UIView] = [walletButton, playlistButton]
    if isVoiceSearchAvailable {
      trailingOptionSubviews.append(voiceSearchButton)
    }
    trailingOptionSubviews.append(contentsOf: [reloadButton])

    trailingOptionSubviews.forEach {
      $0.setContentCompressionResistancePriority(.defaultHigh, for: .horizontal)
      $0.setContentHuggingPriority(.defaultHigh, for: .horizontal)
      trailingTabOptionsStackView.addArrangedSubview($0)
    }

    urlDisplayLabel.setContentHuggingPriority(.defaultLow, for: .horizontal)

    addLayoutGuide(urlLayoutGuide)

    addSubview(contentView)
    contentView.addSubview(leadingItemContainerView)
    contentView.addSubview(urlDisplayLabel)
    contentView.addSubview(trailingTabOptionsStackView)
    contentView.addSubview(placeholderLabel)
    contentView.addSubview(progressBar)

    contentView.snp.makeConstraints {
      $0.edges.equalToSuperview()
    }

    urlDisplayLabel.snp.makeConstraints {
      $0.center.equalToSuperview().priority(.low)
      $0.leading.greaterThanOrEqualTo(urlLayoutGuide)
      $0.trailing.lessThanOrEqualTo(urlLayoutGuide)
    }

    leadingItemContainerView.snp.makeConstraints {
      $0.leading.equalToSuperview()
      $0.top.bottom.equalToSuperview()
    }

    trailingTabOptionsStackView.snp.makeConstraints {
      $0.trailing.equalToSuperview()
      $0.top.bottom.equalToSuperview()
    }

    urlLayoutGuide.snp.makeConstraints {
      $0.leading.greaterThanOrEqualTo(TabLocationViewUX.spacing * 2)
      $0.leading.equalTo(leadingItemContainerView.snp.trailing).priority(.medium)
      $0.trailing.equalTo(trailingTabOptionsStackView.snp.leading)
      $0.top.bottom.equalTo(self)
    }

    placeholderLabel.snp.makeConstraints {
      $0.top.bottom.equalToSuperview()
      // Needs double spacing to line up
      $0.leading.equalToSuperview().inset(TabLocationViewUX.spacing * 2)
      $0.trailing.lessThanOrEqualTo(trailingTabOptionsStackView.snp.leading)
    }

    progressBar.snp.makeConstraints {
      $0.bottom.equalToSuperview()
      $0.height.equalTo(TabLocationViewUX.progressBarHeight)
      $0.leading.trailing.equalToSuperview()
    }

    privateModeCancellable = privateBrowsingManager.$isPrivateBrowsing
      .removeDuplicates()
      .receive(on: RunLoop.main)
      .sink(receiveValue: { [weak self] _ in
        self?.updateColors()
      })

    playlistButton.menuActionHandler = { [unowned self] action in
      self.delegate?.tabLocationViewDidTapPlaylistMenuAction(self, action: action)
    }

    updateForTraitCollection()
    updateColors()
  }

  required init(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
    super.traitCollectionDidChange(previousTraitCollection)
    updateForTraitCollection()
    updateColors()
  }

  override var accessibilityElements: [Any]? {
    get {
      return [urlDisplayLabel, placeholderLabel, readerModeButton, playlistButton, reloadButton]
        .filter { !$0.isHidden }
    }
    set {
      super.accessibilityElements = newValue
    }
  }

  override func layoutSubviews() {
    super.layoutSubviews()

    secureContentStateButton?.setNeedsUpdateConfiguration()
  }

  private func updateForTraitCollection() {
    let clampedTraitCollection = traitCollection.clampingSizeCategory(maximum: .accessibilityLarge)
    let toolbarTraitCollection = UITraitCollection(
      preferredContentSizeCategory: traitCollection.toolbarButtonContentSizeCategory
    )
    urlDisplayLabel.font = .preferredFont(
      forTextStyle: .body,
      compatibleWith: clampedTraitCollection
    )
    placeholderLabel.font = urlDisplayLabel.font
    let pointSize = UIFont.preferredFont(
      forTextStyle: .footnote,
      compatibleWith: toolbarTraitCollection
    ).pointSize
    reloadButton.setPreferredSymbolConfiguration(
      .init(pointSize: pointSize, weight: .regular, scale: .large),
      forImageIn: .normal
    )
    voiceSearchButton.setPreferredSymbolConfiguration(
      .init(pointSize: pointSize, weight: .regular, scale: .large),
      forImageIn: .normal
    )
    let width = UIFontMetrics(forTextStyle: .body).scaledValue(
      for: 44,
      compatibleWith: toolbarTraitCollection
    )
    reloadButton.snp.remakeConstraints {
      $0.width.equalTo(width)
    }
    voiceSearchButton.snp.remakeConstraints {
      $0.width.equalTo(width)
    }
  }

  private func updateColors() {
    let browserColors = privateBrowsingManager.browserColors
    backgroundColor = browserColors.containerBackground
    urlDisplayLabel.textColor = browserColors.textPrimary
    placeholderLabel.textColor = browserColors.textTertiary
    readerModeButton.unselectedTintColor = browserColors.iconDefault
    readerModeButton.selectedTintColor = browserColors.iconActive

    (urlDisplayLabel as! DisplayURLLabel).clippingFade.gradientLayer.colors = [
      browserColors.containerBackground,
      browserColors.containerBackground.withAlphaComponent(0.0),
    ].map {
      $0.resolvedColor(with: traitCollection).cgColor
    }
    for button in [reloadButton, voiceSearchButton] {
      button.primaryTintColor = browserColors.iconDefault
      button.disabledTintColor = browserColors.iconDisabled
      button.selectedTintColor = browserColors.iconActive
    }
  }

  // We must always set isWebScheme before setting the URLDisplayLabel text (so it will display the clipping fade correctly)
  private func updateURLBarWithText() {
    guard let urlDisplayLabel = urlDisplayLabel as? DisplayURLLabel else { return }

    if let url = url {
      if let internalURL = InternalURL(url), internalURL.isBasicAuthURL {
        urlDisplayLabel.isWebScheme = false
        urlDisplayLabel.text = Strings.PageSecurityView.signIntoWebsiteURLBarTitle
      } else {
        // Matches LocationBarModelImpl::GetFormattedURL in Chromium (except for omitHTTP)
        // components/omnibox/browser/location_bar_model_impl.cc
        // TODO: Export omnibox related APIs and use directly

        // If we can't parse the origin and the URL can't be classified via AutoCompleteClassifier
        // the URL is likely a broken deceptive URL. Example: `about:blank#https://apple.com`
        if URLOrigin(url: url).url == nil && URIFixup.getURL(url.absoluteString) == nil {
          urlDisplayLabel.isWebScheme = false
          urlDisplayLabel.text = ""
        } else {
          urlDisplayLabel.isWebScheme = ["http", "https"].contains(url.scheme ?? "")
          urlDisplayLabel.text = URLFormatter.formatURL(
            URLOrigin(url: url).url?.absoluteString ?? url.absoluteString,
            formatTypes: [
              .trimAfterHost, .omitHTTPS, .omitTrivialSubdomains,
            ],
            unescapeOptions: .normal
          )
        }
      }
    } else {
      urlDisplayLabel.isWebScheme = false
      urlDisplayLabel.text = ""
    }

    reloadButton.isHidden = url == nil
    voiceSearchButton.isHidden = (url != nil) || !isVoiceSearchAvailable
    placeholderLabel.isHidden = url != nil
    urlDisplayLabel.isHidden = url == nil
    leadingItemContainerView.isHidden = url == nil
  }

  // MARK: Tap Actions

  @objc func didTapReaderModeButton() {
    delegate?.tabLocationViewDidTapReaderMode(self)
  }

  @objc func didTapPlaylistButton() {
    delegate?.tabLocationViewDidTapPlaylist(self)
  }

  @objc func didTapStopReloadButton() {
    if loading {
      delegate?.tabLocationViewDidTapStop(self)
    } else {
      delegate?.tabLocationViewDidTapReload(self)
    }
  }

  @objc func didTapVoiceSearchButton() {
    delegate?.tabLocationViewDidTapVoiceSearch(self)
  }

  @objc func didTapLocationBar(_ recognizer: UITapGestureRecognizer) {
    delegate?.tabLocationViewDidTapLocation(self)
  }

  @objc func didTapWalletButton() {
    delegate?.tabLocationViewDidTapWalletButton(self)
  }
}

// MARK: - TabEventHandler

extension TabLocationView: TabEventHandler {
  func tabDidGainFocus(_ tab: Tab) {
  }

  func tabDidChangeContentBlockerStatus(_ tab: Tab) {
  }
}

private class DisplayURLLabel: UILabel {
  let pathPadding: CGFloat = 5.0

  let clippingFade = GradientView(
    colors: [.braveBackground, .braveBackground.withAlphaComponent(0.0)],
    positions: [0, 1],
    startPoint: .init(x: 0, y: 0.5),
    endPoint: .init(x: 1, y: 0.5)
  )

  override init(frame: CGRect) {
    super.init(frame: frame)
    addSubview(clippingFade)
  }

  private var textSize: CGSize = .zero
  private var isRightToLeft: Bool = false
  fileprivate var isWebScheme: Bool = false {
    didSet {
      updateClippingDirection()
      setNeedsLayout()
      setNeedsDisplay()
    }
  }

  override var font: UIFont! {
    didSet {
      updateText()
      updateTextSize()
    }
  }

  override var text: String? {
    didSet {
      clippingFade.isHidden = true
      if oldValue != text {
        updateText()
        updateTextSize()
        detectLanguageForNaturalDirectionClipping()
        updateClippingDirection()
      }
      setNeedsDisplay()
    }
  }

  private func updateText() {
    if let text = text {
      // Without attributed string, the label will always render RTL characters even if you force LTR layout.
      // This can introduce a security flaw! We must not flip the URL around based on RTL characters (Safari does not).
      let paragraphStyle = NSMutableParagraphStyle()
      paragraphStyle.lineBreakMode = .byClipping
      paragraphStyle.baseWritingDirection = .leftToRight

      self.attributedText = NSAttributedString(
        string: text,
        attributes: [
          .font: font ?? .preferredFont(forTextStyle: .body),
          .paragraphStyle: paragraphStyle,
        ]
      )
    } else {
      self.attributedText = nil
    }
  }

  private func updateTextSize() {
    textSize = attributedText?.size() ?? .zero
    setNeedsLayout()
    setNeedsDisplay()
  }

  private func detectLanguageForNaturalDirectionClipping() {
    guard let text, let language = NLLanguageRecognizer.dominantLanguage(for: text) else { return }
    switch language {
    case .arabic, .hebrew, .persian, .urdu:
      isRightToLeft = true
    default:
      isRightToLeft = false
    }
  }

  private func updateClippingDirection() {
    // Update clipping fade direction
    clippingFade.gradientLayer.startPoint = .init(x: isRightToLeft || !isWebScheme ? 1 : 0, y: 0.5)
    clippingFade.gradientLayer.endPoint = .init(x: isRightToLeft || !isWebScheme ? 0 : 1, y: 0.5)
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }

  override var accessibilityTraits: UIAccessibilityTraits {
    get { [.staticText, .button] }
    set {}
  }

  override var canBecomeFirstResponder: Bool {
    return false
  }

  override func layoutSubviews() {
    super.layoutSubviews()

    clippingFade.frame = .init(
      x: isRightToLeft || !isWebScheme ? bounds.width - 20 : 0,
      y: 0,
      width: 20,
      height: bounds.height
    )
  }

  // This override is done in case the eTLD+1 string overflows the width of textField.
  // In that case the textRect is adjusted to show right aligned and truncate left.
  // Since this textField changes with WebView domain change, performance implications are low.
  override func drawText(in rect: CGRect) {
    var rect = rect
    if textSize.width > bounds.width {
      let delta = (textSize.width - bounds.width)
      if !isRightToLeft && isWebScheme {
        rect.origin.x -= delta
        rect.size.width += delta
      }
      bringSubviewToFront(clippingFade)
      clippingFade.isHidden = false
    } else {
      clippingFade.isHidden = true
    }
    super.drawText(in: rect)
  }
}
