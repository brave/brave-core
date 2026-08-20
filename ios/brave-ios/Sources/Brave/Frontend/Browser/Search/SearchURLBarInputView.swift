// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveUI
import DesignSystem
import Shared
import SnapKit
import SpeechRecognition
import Then
import UIKit

/// Actions triggered by the buttons alongside the URL input field.
protocol SearchURLBarInputViewDelegate: AnyObject {
  func searchURLBarInputViewDidTapCancel(_ inputView: SearchURLBarInputView)
  func searchURLBarInputViewDidTapPasteAndGo(_ inputView: SearchURLBarInputView)
  func searchURLBarInputViewDidTapQRCode(_ inputView: SearchURLBarInputView)
  func searchURLBarInputViewDidTapVoiceSearch(_ inputView: SearchURLBarInputView)
}

/// The editable URL/search input field shown by `SearchContainerViewController`.
///
/// Replaces the address entry that previously lived inside `TopToolbarView`, allowing the
/// browser toolbar to remain static while the user edits the URL.
class SearchURLBarInputView: UIView {
  private struct UX {
    static let locationPadding: CGFloat = 8
    static let locationHeight: CGFloat = 44
  }

  weak var actionDelegate: SearchURLBarInputViewDelegate?

  private let privateBrowsingManager: PrivateBrowsingManager
  private let speechRecognizer: SpeechRecognizer

  let textField: AutocompleteTextField

  private lazy var searchImageView = UIImageView().then {
    $0.image = UIImage(braveSystemNamed: "leo.search", compatibleWith: nil)
    $0.contentMode = .scaleAspectFit
    $0.tintColor = UIColor(braveSystemName: .iconDefault)
    $0.setContentCompressionResistancePriority(.required, for: .horizontal)
    $0.setContentCompressionResistancePriority(.defaultHigh, for: .vertical)
    $0.setContentHuggingPriority(.defaultHigh, for: .horizontal)
    $0.setContentHuggingPriority(.defaultHigh, for: .vertical)
  }

  private lazy var qrCodeButton = ToolbarButton().then {
    $0.accessibilityIdentifier = "TabToolbar.qrCodeButton"
    $0.isAccessibilityElement = true
    $0.accessibilityLabel = Strings.quickActionScanQRCode
    $0.setImage(UIImage(braveSystemNamed: "leo.qr.code", compatibleWith: nil), for: .normal)
    $0.tintColor = UIColor(braveSystemName: .iconDefault)
    $0.contentEdgeInsets = UIEdgeInsets(top: 0, left: 5, bottom: 0, right: 5)
    $0.setContentCompressionResistancePriority(.required, for: .horizontal)
    $0.setContentCompressionResistancePriority(.defaultHigh, for: .vertical)
    $0.setContentHuggingPriority(.defaultHigh, for: .horizontal)
    $0.setContentHuggingPriority(.defaultHigh, for: .vertical)
    $0.addTarget(self, action: #selector(didTapQRCodeButton), for: .touchUpInside)
  }

  private lazy var voiceSearchButton = ToolbarButton().then {
    $0.accessibilityIdentifier = "TabToolbar.voiceSearchButton"
    $0.isAccessibilityElement = true
    $0.accessibilityLabel = Strings.tabToolbarVoiceSearchButtonAccessibilityLabel
    $0.setImage(UIImage(braveSystemNamed: "leo.microphone", compatibleWith: nil), for: .normal)
    $0.tintColor = UIColor(braveSystemName: .iconDefault)
    $0.contentEdgeInsets = UIEdgeInsets(top: 0, left: 5, bottom: 0, right: 5)
    $0.setContentCompressionResistancePriority(.required, for: .horizontal)
    $0.setContentCompressionResistancePriority(.defaultHigh, for: .vertical)
    $0.setContentHuggingPriority(.defaultHigh, for: .horizontal)
    $0.setContentHuggingPriority(.defaultHigh, for: .vertical)
    $0.addTarget(self, action: #selector(didTapVoiceSearchButton), for: .touchUpInside)
  }

  private lazy var pasteAndGoButton = ToolbarButton().then {
    $0.accessibilityIdentifier = "TabToolbar.pasteAndGoButton"
    $0.isAccessibilityElement = true
    $0.accessibilityLabel = Strings.tabToolbarPasteAndGoButtonAccessibilityLabel
    $0.setImage(UIImage(braveSystemNamed: "leo.clipboard", compatibleWith: nil), for: .normal)
    $0.tintColor = UIColor(braveSystemName: .iconDefault)
    $0.isHidden = !UIPasteboard.general.hasStrings && !UIPasteboard.general.hasURLs
    $0.contentEdgeInsets = UIEdgeInsets(top: 0, left: 5, bottom: 0, right: 5)
    $0.setContentCompressionResistancePriority(.required, for: .horizontal)
    $0.setContentCompressionResistancePriority(.defaultHigh, for: .vertical)
    $0.setContentHuggingPriority(.defaultHigh, for: .horizontal)
    $0.setContentHuggingPriority(.defaultHigh, for: .vertical)
    $0.addTarget(self, action: #selector(didTapPasteAndGoButton), for: .touchUpInside)
  }

  private lazy var optionsStackView = UIStackView().then {
    $0.alignment = .center
    $0.layoutMargins = UIEdgeInsets(top: 0, left: 0, bottom: 0, right: 3)
    $0.isLayoutMarginsRelativeArrangement = true
    $0.insetsLayoutMarginsFromSafeArea = false
  }

  private lazy var cancelButton = InsetButton().then {
    $0.setTitle(Strings.cancelButtonTitle, for: .normal)
    $0.setTitleColor(UIColor(braveSystemName: .textSecondary), for: .normal)
    $0.accessibilityIdentifier = "searchURLBarInputView-cancel"
    $0.addTarget(self, action: #selector(didTapCancelButton), for: .touchUpInside)
    $0.setContentCompressionResistancePriority(.required, for: .horizontal)
    $0.setContentHuggingPriority(.defaultHigh, for: .horizontal)
  }

  private let locationContainer = LocationContainerView()

  private let mainStackView = UIStackView().then {
    $0.spacing = 8
    $0.alignment = .center
  }

  init(privateBrowsingManager: PrivateBrowsingManager, speechRecognizer: SpeechRecognizer) {
    self.privateBrowsingManager = privateBrowsingManager
    self.speechRecognizer = speechRecognizer
    self.textField = AutocompleteTextField(privateBrowsingManager: privateBrowsingManager)

    super.init(frame: .zero)

    textField.do {
      $0.translatesAutoresizingMaskIntoConstraints = false
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
      $0.rightViewMode = .never
      if let dropInteraction = $0.textDropInteraction {
        $0.removeInteraction(dropInteraction)
      }
    }

    optionsStackView.addArrangedSubview(pasteAndGoButton)
    if RecentSearchQRCodeScannerController.hasCameraSupport {
      optionsStackView.addArrangedSubview(qrCodeButton)
    }
    optionsStackView.addArrangedSubview(voiceSearchButton)
    voiceSearchButton.isHidden = !speechRecognizer.isVoiceSearchAvailable

    let locationContentView = UIStackView(arrangedSubviews: [
      searchImageView, textField, optionsStackView,
    ]).then {
      $0.layoutMargins = UIEdgeInsets(top: 2, left: 8, bottom: 2, right: 0)
      $0.isLayoutMarginsRelativeArrangement = true
      $0.insetsLayoutMarginsFromSafeArea = false
      $0.spacing = 8
      $0.setCustomSpacing(4, after: textField)
    }

    locationContainer.contentView.addSubview(locationContentView)
    locationContentView.snp.makeConstraints {
      $0.edges.equalTo(locationContainer).inset(
        UIEdgeInsets(top: 0, left: UX.locationPadding, bottom: 0, right: UX.locationPadding)
      )
    }

    locationContainer.setContentHuggingPriority(.defaultLow, for: .horizontal)

    // We need to create a small container view here to avoid corner adaption on iOS 26 from basing
    // its required horizontal spacing based on the SearchURLBarInputView itself being constrained
    // in the safe area (so that the background can render all the way to the edge). We then use
    // this containers layout guide instead of the main views.
    let contentView = UIView()
    contentView.layoutMargins = .init(vertical: 8, horizontal: 12)
    addSubview(contentView)
    contentView.addSubview(mainStackView)
    mainStackView.addArrangedSubview(locationContainer)
    mainStackView.addArrangedSubview(cancelButton)

    contentView.snp.makeConstraints {
      $0.edges.equalTo(safeAreaLayoutGuide)
    }

    mainStackView.snp.makeConstraints {
      if #available(iOS 26.0, *) {
        $0.leading.trailing.equalTo(
          contentView.layoutGuide(for: .margins(cornerAdaptation: .horizontal))
        )
      } else {
        $0.leading.trailing.equalTo(contentView.layoutMarginsGuide)
      }
      $0.top.bottom.equalTo(contentView.layoutMarginsGuide)
    }
    locationContainer.snp.makeConstraints {
      $0.height.greaterThanOrEqualTo(UX.locationHeight)
    }

    updateColors()
    updateLocationBarRightView(showToolbarActions: true)
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }

  override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
    super.traitCollectionDidChange(previousTraitCollection)
    let clampedTraitCollection = traitCollection.clampingSizeCategory(maximum: .accessibilityLarge)
    textField.font = .preferredFont(forTextStyle: .body, compatibleWith: clampedTraitCollection)
  }

  private func makePlaceholder(colors: some BrowserColors) -> NSAttributedString {
    NSAttributedString(
      string: Strings.tabToolbarSearchAddressPlaceholderText,
      attributes: [.foregroundColor: colors.textTertiary]
    )
  }

  func updateColors() {
    overrideUserInterfaceStyle = privateBrowsingManager.isPrivateBrowsing ? .dark : .unspecified
    let browserColors = privateBrowsingManager.browserColors
    backgroundColor = browserColors.chromeBackground
    locationContainer.contentView.backgroundColor = browserColors.containerBackground
    textField.backgroundColor = browserColors.containerBackground
    textField.textColor = browserColors.textPrimary
    textField.attributedPlaceholder = makePlaceholder(colors: browserColors)
    for button in [qrCodeButton, voiceSearchButton, pasteAndGoButton] {
      button.primaryTintColor = browserColors.iconDefault
      button.disabledTintColor = browserColors.iconDisabled
      button.selectedTintColor = browserColors.iconActive
    }
  }

  /// Toggles the paste/QR/voice action buttons. They are only relevant when the field is empty.
  func updateLocationBarRightView(showToolbarActions: Bool) {
    optionsStackView.isHidden = !showToolbarActions

    if RecentSearchQRCodeScannerController.hasCameraSupport {
      qrCodeButton.isHidden = !showToolbarActions
    } else {
      qrCodeButton.isHidden = true
    }

    if speechRecognizer.isVoiceSearchAvailable {
      voiceSearchButton.isHidden = !showToolbarActions
    } else {
      voiceSearchButton.isHidden = true
    }

    pasteAndGoButton.isHidden =
      !showToolbarActions
      || (!UIPasteboard.general.hasStrings && !UIPasteboard.general.hasURLs)
  }

  func setAutocompleteSuggestion(_ suggestion: String?) {
    textField.setAutocompleteSuggestion(suggestion)
  }

  /// Sets the field text. When `search` is true the text is treated as a query and the search
  /// pipeline is triggered; otherwise it is set as a URL being edited without searching.
  func setLocation(_ location: String?, search: Bool) {
    guard let text = location, !text.isEmpty else {
      textField.text = location
      updateLocationBarRightView(showToolbarActions: true)
      return
    }

    updateLocationBarRightView(showToolbarActions: false)

    if search {
      textField.text = text
      // Seeding `text` programmatically does not fire `AutocompleteTextField`'s change
      // notification (it only fires for real keystrokes), so notify the delegate explicitly to
      // drive the search.
      textField.autocompleteDelegate?.autocompleteTextField(textField, didEnterText: text)
    } else {
      textField.setTextWithoutSearching(text)
    }
  }

  @discardableResult
  override func becomeFirstResponder() -> Bool {
    return textField.becomeFirstResponder()
  }

  /// Selects the entire entry so the user can type over the seeded text.
  func selectAllText() {
    textField.selectedTextRange = textField.textRange(
      from: textField.beginningOfDocument,
      to: textField.endOfDocument
    )
  }

  @discardableResult
  override func resignFirstResponder() -> Bool {
    return textField.resignFirstResponder()
  }

  // MARK: - Actions

  @objc private func didTapCancelButton() {
    actionDelegate?.searchURLBarInputViewDidTapCancel(self)
  }

  @objc private func didTapPasteAndGoButton() {
    actionDelegate?.searchURLBarInputViewDidTapPasteAndGo(self)
  }

  @objc private func didTapQRCodeButton() {
    actionDelegate?.searchURLBarInputViewDidTapQRCode(self)
  }

  @objc private func didTapVoiceSearchButton() {
    actionDelegate?.searchURLBarInputViewDidTapVoiceSearch(self)
  }
}
