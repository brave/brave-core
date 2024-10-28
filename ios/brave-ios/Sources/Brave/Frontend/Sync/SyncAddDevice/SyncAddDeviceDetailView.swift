// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShared
import Shared
import UIKit

class SyncAddDeviceInformationView: UIStackView {

  private var titleLabel = UILabel().then {
    $0.translatesAutoresizingMaskIntoConstraints = false
    $0.font = .preferredFont(for: .title2, weight: .semibold)
    $0.textColor = UIColor(braveSystemName: .textPrimary)
    $0.setContentCompressionResistancePriority(.defaultLow, for: .vertical)
    $0.setContentHuggingPriority(.defaultHigh, for: .vertical)
  }

  private var descriptionLabel = UILabel().then {
    $0.font = .preferredFont(forTextStyle: .subheadline)
    $0.textColor = UIColor(braveSystemName: .textPrimary)
    $0.numberOfLines = 0
    $0.lineBreakMode = .byTruncatingTail
    $0.adjustsFontSizeToFitWidth = true
    $0.minimumScaleFactor = 0.5
    $0.setContentCompressionResistancePriority(.defaultLow, for: .vertical)
    $0.setContentHuggingPriority(.defaultHigh, for: .vertical)
  }

  override init(frame: CGRect) {
    super.init(frame: frame)

    axis = .vertical
    spacing = 20
    alignment = .leading

    addArrangedSubview(titleLabel)
    addArrangedSubview(descriptionLabel)
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }

  func updateLabels(isFirstIndex: Bool) {
    titleLabel.text = isFirstIndex ? Strings.Sync.addDeviceScan : Strings.Sync.addDeviceWords

    if isFirstIndex {
      let description = Strings.Sync.addDeviceScanDescription
      let attributedDescription = NSMutableAttributedString(string: description)

      if let lastSentenceRange = lastSentenceRange(text: description) {
        attributedDescription.addAttributes(
          [
            .font: UIFont.preferredFont(for: .footnote, weight: .semibold),
            .foregroundColor: UIColor.braveErrorLabel,
          ],
          range: lastSentenceRange
        )
      }

      descriptionLabel.attributedText = attributedDescription
    } else {
      // The button name should be the same as in codewords instructions.
      let buttonName = Strings.Sync.scanSyncCode
      let addDeviceWords = String(format: Strings.Sync.addDeviceWordsDescription, buttonName)
      let description = NSMutableAttributedString(string: addDeviceWords)

      let boldRange = (addDeviceWords as NSString).range(of: buttonName)
      description.addAttribute(
        .font,
        value: UIFont.preferredFont(for: .subheadline, weight: .bold),
        range: boldRange
      )

      if let lastSentenceRange = lastSentenceRange(text: addDeviceWords) {
        description.addAttributes(
          [
            .font: UIFont.preferredFont(for: .footnote, weight: .semibold),
            .foregroundColor: UIColor.braveErrorLabel,
          ],
          range: lastSentenceRange
        )
      }

      descriptionLabel.attributedText = description
    }
  }

  private func lastSentenceRange(text: String) -> NSRange? {
    guard let lastSentence = text.split(separator: "\n").last else { return nil }
    return (text as NSString).range(of: String(lastSentence))
  }
}

class SyncAddDeviceCodeView: UIStackView {

  // View is created to create white frame around the QR code
  // It is done to solve problems scanning QR code with Android devices
  // Do not change how QR code is presented (size, background and inset)
  // Or do not try to Brave Logo in the middle
  private let qrCodeContainerView = UIView().then {
    $0.backgroundColor = .white
    $0.layer.cornerRadius = 8
    $0.layer.cornerCurve = .continuous
    $0.layer.borderColor = UIColor(braveSystemName: .dividerSubtle).cgColor
    $0.layer.borderWidth = 1.0
  }

  private lazy var qrCodeView = SyncAddDeviceQRCodeView(frame: .zero)

  private let codeWordsContainerView = UIView().then {
    $0.backgroundColor = UIColor(braveSystemName: .containerBackground)
    $0.layer.cornerRadius = 8
    $0.layer.cornerCurve = .continuous
    $0.layer.borderColor = UIColor(braveSystemName: .dividerSubtle).cgColor
    $0.layer.borderWidth = 1.0
    $0.isHidden = true
  }

  private lazy var codewordsView = UILabel().then {
    $0.font = UIFont.monospacedSystemFont(ofSize: 15.0, weight: UIFont.Weight.regular)
    $0.lineBreakMode = NSLineBreakMode.byWordWrapping
    $0.textColor = UIColor(braveSystemName: .textPrimary)
    $0.textAlignment = .left
    $0.numberOfLines = 0
    $0.setContentHuggingPriority(.defaultHigh, for: .vertical)
  }

  var syncChainCode: String? {
    return codewordsView.text
  }

  private let syncAPI: BraveSyncAPI

  required init(syncAPI: BraveSyncAPI) {
    self.syncAPI = syncAPI

    super.init(frame: .zero)

    axis = .vertical
    alignment = .center

    createSyncCodeViews(syncAPI: syncAPI)

    doLayout()
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }

  private func doLayout() {
    addArrangedSubview(qrCodeContainerView)
    addArrangedSubview(codeWordsContainerView)

    qrCodeContainerView.snp.makeConstraints {
      $0.centerX.equalToSuperview()
      $0.width.equalToSuperview().multipliedBy(0.75)
      $0.width.equalTo(qrCodeContainerView.snp.height)
    }

    codeWordsContainerView.snp.makeConstraints {
      $0.leading.trailing.equalToSuperview()
    }

    qrCodeContainerView.addSubview(qrCodeView)

    qrCodeView.snp.makeConstraints {
      $0.edges.equalToSuperview().inset(12)
    }

    codeWordsContainerView.addSubview(codewordsView)

    codewordsView.snp.makeConstraints {
      $0.edges.equalToSuperview().inset(16)
    }
  }

  private func createSyncCodeViews(syncAPI: BraveSyncAPI) {
    qrCodeView = SyncAddDeviceQRCodeView(syncApi: syncAPI)

    codewordsView.do {
      $0.text = syncAPI.getTimeLimitedWords(fromWords: syncAPI.getSyncCode())
    }
  }

  func swapCodeViewType(_ isSyncCodePresented: Bool) {
    qrCodeContainerView.isHidden = !isSyncCodePresented
    codeWordsContainerView.isHidden = isSyncCodePresented
  }

  func refreshSyncCodeViews() {
    removeArrangedSubview(qrCodeContainerView)
    removeArrangedSubview(codeWordsContainerView)

    createSyncCodeViews(syncAPI: syncAPI)

    doLayout()
  }
}

class SyncAddDeviceActionView: UIStackView {

  public protocol ActionDelegate: AnyObject {
    func copyToClipboard()
    func dismiss()
    func generateNewCode()
  }

  private lazy var doneButton = UIButton().then {
    $0.setTitle(Strings.done, for: .normal)
    $0.titleLabel?.font = .preferredFont(for: .subheadline, weight: .semibold)
    $0.addTarget(self, action: #selector(dismiss), for: .touchUpInside)
    $0.setTitleColor(UIColor(braveSystemName: .schemesOnPrimary), for: .normal)
    $0.backgroundColor = UIColor(braveSystemName: .buttonBackground)
    $0.layer.cornerRadius = 12
    $0.layer.cornerCurve = .continuous
  }

  private lazy var copyPasteButton = UIButton().then {
    $0.setTitle(Strings.Sync.copyToClipboard, for: .normal)
    $0.titleLabel?.font = .preferredFont(for: .subheadline, weight: .semibold)
    $0.addTarget(self, action: #selector(copyToClipboard), for: .touchUpInside)
    $0.setTitleColor(UIColor(braveSystemName: .textPrimary), for: .normal)
    $0.isHidden = true
  }

  private lazy var generateNewCodeButton = UIButton().then {
    $0.setTitle("Generate New Code", for: .normal)
    $0.titleLabel?.font = .preferredFont(for: .subheadline, weight: .semibold)
    $0.addTarget(self, action: #selector(generateNewCode), for: .touchUpInside)
    $0.setTitleColor(UIColor(braveSystemName: .textInteractive), for: .normal)
    $0.backgroundColor = .clear
    $0.isHidden = true
    $0.layer.cornerRadius = 12
    $0.layer.cornerCurve = .continuous
    $0.layer.borderColor = UIColor(braveSystemName: .dividerInteractive).cgColor
    $0.layer.borderWidth = 1.0
  }

  private var copyButtonPressed = false {
    didSet {
      if copyButtonPressed {
        copyPasteButton.setTitle(Strings.Sync.copiedToClipboard, for: .normal)
      } else {
        copyPasteButton.setTitle(Strings.Sync.copyToClipboard, for: .normal)
      }
    }
  }

  weak var delegate: ActionDelegate?

  required init() {
    super.init(frame: .zero)

    axis = .vertical
    spacing = 8
    distribution = .fillEqually
    setContentCompressionResistancePriority(.required, for: .vertical)

    addArrangedSubview(generateNewCodeButton)
    addArrangedSubview(copyPasteButton)
    addArrangedSubview(doneButton)

    generateNewCodeButton.snp.makeConstraints {
      $0.height.equalTo(40)
    }

    doneButton.snp.makeConstraints {
      $0.height.equalTo(40)
    }
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }

  func swapCodeViewType(_ isSyncCodePresented: Bool) {
    copyPasteButton.isHidden = isSyncCodePresented
  }

  func swapCodeExpirationType(isSyncCodePresented: Bool, isExpired: Bool) {
    copyPasteButton.isHidden = isSyncCodePresented || isExpired
    generateNewCodeButton.isHidden = !isExpired
  }

  @objc private func copyToClipboard() {
    copyButtonPressed = true
    delegate?.copyToClipboard()
  }

  @objc private func dismiss() {
    delegate?.dismiss()
  }
  @objc private func generateNewCode() {
    delegate?.generateNewCode()
  }
}

class SyncAddDeviceCodeExpirationView: UIStackView {

  public protocol ActionDelegate: AnyObject {
    func codeExpired()
  }

  // MARK: UX

  private let timeRemainingContainerView = UIView().then {
    $0.backgroundColor = UIColor(braveSystemName: .systemfeedbackInfoBackground)
    $0.layer.cornerRadius = 8
    $0.layer.cornerCurve = .continuous
  }

  private let timeRemainingStackView = UIStackView().then {
    $0.axis = .horizontal
    $0.spacing = 20
    $0.alignment = .firstBaseline
    $0.setContentCompressionResistancePriority(.required, for: .vertical)
  }

  private let timeRemainingTitleLabel = UILabel().then {
    $0.font = .preferredFont(forTextStyle: .subheadline)
    $0.lineBreakMode = .byWordWrapping
    $0.textColor = UIColor(braveSystemName: .systemfeedbackInfoText)
    $0.textAlignment = .left
    $0.numberOfLines = 0
    $0.setContentHuggingPriority(.defaultHigh, for: .vertical)
  }

  private let timeRemainingIconImageView = UIImageView().then {
    $0.image = UIImage(braveSystemNamed: "leo.timer")
    $0.tintColor = UIColor(braveSystemName: .systemfeedbackInfoIcon)
    $0.snp.makeConstraints {
      $0.size.equalTo(20)
    }
    $0.setContentHuggingPriority(.required, for: .horizontal)
  }

  private let codeExpirationContainerView = UIView().then {
    $0.backgroundColor = UIColor(braveSystemName: .systemfeedbackErrorBackground)
    $0.layer.cornerRadius = 8
    $0.layer.cornerCurve = .continuous
    $0.isHidden = true
  }

  private let codeExpirationStackView = UIStackView().then {
    $0.axis = .horizontal
    $0.spacing = 20
    $0.alignment = .firstBaseline
    $0.setContentCompressionResistancePriority(.required, for: .vertical)
  }

  private let codeExpirationTitleLabel = UILabel().then {
    $0.font = .preferredFont(forTextStyle: .subheadline)
    $0.text = Strings.Sync.codeExpirationTitleLabel
    $0.lineBreakMode = .byWordWrapping
    $0.textColor = UIColor(braveSystemName: .systemfeedbackErrorText)
    $0.textAlignment = .left
    $0.numberOfLines = 0
    $0.setContentHuggingPriority(.defaultHigh, for: .vertical)
  }

  private let codeExpirationIconImageView = UIImageView().then {
    $0.image = UIImage(braveSystemNamed: "leo.warning.circle-filled")?.template
    $0.tintColor = UIColor(braveSystemName: .systemfeedbackErrorIcon)
    $0.snp.makeConstraints {
      $0.size.equalTo(20)
    }
    $0.setContentHuggingPriority(.required, for: .horizontal)
  }

  // MARK: Internal

  private var expirationTime = Date()

  private var remainingInterval: TimeInterval {
    expirationTime.timeIntervalSinceReferenceDate - Date().timeIntervalSinceReferenceDate
  }

  private var codeExpiryTimer: Timer?

  weak var delegate: ActionDelegate?

  required init() {
    super.init(frame: .zero)

    axis = .vertical
    alignment = .center

    addArrangedSubview(timeRemainingContainerView)
    addArrangedSubview(codeExpirationContainerView)

    timeRemainingContainerView.snp.makeConstraints {
      $0.leading.trailing.equalToSuperview()
    }

    codeExpirationContainerView.snp.makeConstraints {
      $0.leading.trailing.equalToSuperview()
    }

    timeRemainingContainerView.addSubview(timeRemainingStackView)

    timeRemainingStackView.snp.makeConstraints {
      $0.edges.equalToSuperview().inset(16)
    }

    timeRemainingStackView.addArrangedSubview(timeRemainingIconImageView)
    timeRemainingStackView.addArrangedSubview(timeRemainingTitleLabel)

    codeExpirationContainerView.addSubview(codeExpirationStackView)

    codeExpirationStackView.snp.makeConstraints {
      $0.edges.equalToSuperview().inset(16)
    }

    codeExpirationStackView.addArrangedSubview(codeExpirationIconImageView)
    codeExpirationStackView.addArrangedSubview(codeExpirationTitleLabel)
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }

  private func startExpirationTimer() {
    codeExpiryTimer =
      Timer.scheduledTimer(
        timeInterval: 1,
        target: self,
        selector: #selector(updateRemainingTime),
        userInfo: nil,
        repeats: true
      )

    updateRemainingTime()
  }

  @objc private func updateRemainingTime() {
    let timeRemainingString = fetchRemainingTimeString(time: remainingInterval)

    if Int(remainingInterval) < 0 {
      swapCodeExpirationType(true)
      codeExpiryTimer?.invalidate()
      delegate?.codeExpired()
    } else {
      timeRemainingTitleLabel.text = String(
        format: Strings.Sync.codeTimeRemainingTitleLabel,
        timeRemainingString
      )
    }
  }

  private func fetchRemainingTimeString(time: TimeInterval) -> String {
    let hours = Int(time) / 3600
    let minutes = Int(time) / 60 % 60
    let seconds = Int(time) % 60

    let hoursText =
      hours > 1
      ? "\(hours) \(Strings.Sync.codeTimeMultipleHourText)"
      : "\(hours) \(Strings.Sync.codeTimeSingleHourText)"
    let minutesText = "\(minutes) \(Strings.Sync.codeTimeMinutesAbbreviation)"
    let secondsText =
      seconds > 1
      ? "\(seconds) \(Strings.Sync.codeTimeMultipleSecondsText)"
      : "\(seconds) \(Strings.Sync.codeTimeSingleSecondText)"

    return "\(hoursText) \(minutesText) \(secondsText)"
  }

  func resetExpiration(expirationTime: Date) {
    self.expirationTime = expirationTime

    startExpirationTimer()
  }

  func swapCodeExpirationType(_ isExpired: Bool) {
    timeRemainingContainerView.isHidden = isExpired
    codeExpirationContainerView.isHidden = !isExpired
  }

}
