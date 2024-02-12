/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import Shared
import BraveShared
import BraveCore
import Data
import BraveWallet
import BraveUI

enum SyncDeviceType {
  case mobile
  case computer
}

class SyncAddDeviceViewController: SyncViewController {

  // MARK: UX
  
  private let barcodeSize: CGFloat = 300.0

  private var scrollViewContainer = UIScrollView()
  
  private var stackView = UIStackView().then {
    $0.axis = .vertical
    $0.spacing = 20
  }
  
  private var modeControl = UISegmentedControl(items: [Strings.QRCode, Strings.codeWords]).then {
    $0.translatesAutoresizingMaskIntoConstraints = false
    $0.selectedSegmentIndex = 0
    $0.setContentCompressionResistancePriority(.defaultHigh, for: .vertical)
    $0.selectedSegmentTintColor = UIColor.braveBlurpleTint
    $0.setTitleTextAttributes([.foregroundColor: UIColor.white], for: .selected)
  }
  
  private let titleDescriptionStackView = UIStackView().then {
    $0.axis = .vertical
    $0.spacing = 2
    $0.alignment = .center
  }
  
  private var titleLabel = UILabel().then {
    $0.translatesAutoresizingMaskIntoConstraints = false
    $0.font = UIFont.systemFont(ofSize: 20, weight: UIFont.Weight.semibold)
    $0.textColor = .braveLabel
    $0.setContentCompressionResistancePriority(.defaultLow, for: .vertical)
    $0.setContentHuggingPriority(.defaultHigh, for: .vertical)
  }
  
  private var descriptionLabel = UILabel().then {
    $0.font = UIFont.systemFont(ofSize: 15, weight: UIFont.Weight.regular)
    $0.textColor = .braveLabel
    $0.numberOfLines = 0
    $0.lineBreakMode = .byTruncatingTail
    $0.textAlignment = .center
    $0.adjustsFontSizeToFitWidth = true
    $0.minimumScaleFactor = 0.5
    $0.setContentCompressionResistancePriority(.defaultLow, for: .vertical)
    $0.setContentHuggingPriority(.defaultHigh, for: .vertical)
  }

  let syncDetailsContainerView = UIStackView().then {
    $0.axis = .vertical
    $0.alignment = .center
  }
  
  // This view is created to create white frame around the QR code
  // It is done to solve problems scanning QR code with Android devices
  private let qrCodeContainerView = UIView().then {
    $0.backgroundColor = .white
  }
  
  private let qrCodeView: SyncQRCodeView
  
  private lazy var codewordsView = UILabel().then {
    $0.font = UIFont.systemFont(ofSize: 18.0, weight: UIFont.Weight.medium)
    $0.lineBreakMode = NSLineBreakMode.byWordWrapping
    $0.textAlignment = .center
    $0.numberOfLines = 0
    $0.setContentHuggingPriority(.defaultHigh, for: .vertical)
  }
  
  private let doneEnterWordsStackView = UIStackView().then {
    $0.axis = .vertical
    $0.spacing = 4
    $0.distribution = .fillEqually
    $0.setContentCompressionResistancePriority(.required, for: .vertical)
  }
  
  private var doneButton = RoundInterfaceButton(type: .roundedRect).then {
    $0.translatesAutoresizingMaskIntoConstraints = false
    $0.setTitle(Strings.done, for: .normal)
    $0.titleLabel?.font = UIFont.systemFont(ofSize: 17, weight: UIFont.Weight.bold)
    $0.setTitleColor(.white, for: .normal)
    $0.backgroundColor = .braveBlurpleTint
  }
  
  private lazy var copyPasteButton = UIButton().then {
    $0.setTitle(Strings.copyToClipboard, for: .normal)
    $0.addTarget(self, action: #selector(copyToClipboard), for: .touchUpInside)
    $0.setTitleColor(UIColor.braveBlurpleTint, for: .normal)
    $0.contentEdgeInsets = UIEdgeInsets(top: 8, left: 8, bottom: 8, right: 8)
    $0.isHidden = true
  }
  
  // MARK: Internal
  
  private var pageTitle: String = Strings.sync
  
  private var deviceType: SyncDeviceType = .mobile
  
  private var copyButtonPressed = false {
    didSet {
      if copyButtonPressed {
        copyPasteButton.setTitle(Strings.copiedToClipboard, for: .normal)
      } else {
        copyPasteButton.setTitle(Strings.copyToClipboard, for: .normal)
      }
    }
  }

  private let syncAPI: BraveSyncAPI
  var addDeviceHandler: (() -> Void)?

  // MARK: Lifecycle
  
  init(title: String, type: SyncDeviceType, syncAPI: BraveSyncAPI) {
    self.syncAPI = syncAPI
    qrCodeView = SyncQRCodeView(syncApi: syncAPI)
    super.init()

    pageTitle = title
    deviceType = type
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  override func viewDidLoad() {
    super.viewDidLoad()

    setTheme()
    doLayout()
    
    enableDeviceType()
  }
  
  override func viewDidAppear(_ animated: Bool) {
    super.viewDidAppear(animated)
    
    if !syncAPI.isInSyncGroup && !syncAPI.isSyncFeatureActive && !syncAPI.isInitialSyncFeatureSetupComplete {
      showInitializationError()
    }
  }
  
  override func viewDidLayoutSubviews() {
    super.viewDidLayoutSubviews()
    
    scrollViewContainer.contentSize = CGSize(width: stackView.frame.width, height: stackView.frame.height)
    updateLabels()
  }
  
  private func setTheme() {
    title = deviceType == .computer ? Strings.syncAddComputerTitle : Strings.syncAddTabletOrPhoneTitle

    codewordsView.do {
      $0.text = syncAPI.getTimeLimitedWords(fromWords: syncAPI.getSyncCode())
      $0.isHidden = true
    }
    
    modeControl.do {
      $0.isHidden = deviceType == .computer
      $0.addTarget(self, action: #selector(changeMode), for: .valueChanged)
    }
    
    doneButton.addTarget(self, action: #selector(done), for: .touchUpInside)
  }

  private func doLayout() {
    // Scroll View
    view.addSubview(scrollViewContainer)
    scrollViewContainer.snp.makeConstraints {
      $0.top.equalTo(view.safeArea.top).inset(10)
      $0.left.right.equalTo(view)
      $0.bottom.equalTo(view.safeArea.bottom).inset(24)
    }
    
    // Content StackView
    scrollViewContainer.addSubview(stackView)
    stackView.snp.makeConstraints {
      $0.top.bottom.equalToSuperview()
      $0.leading.trailing.equalTo(view).inset(16)
      $0.width.equalToSuperview()
    }
    
    // Segmented Control
    stackView.addArrangedSubview(modeControl)

    // Title - Description Label
    titleDescriptionStackView.addArrangedSubview(titleLabel)
    titleDescriptionStackView.addArrangedSubview(descriptionLabel)
    stackView.addArrangedSubview(titleDescriptionStackView)

    // Code Words View - QR Code
    syncDetailsContainerView.addArrangedSubview(qrCodeContainerView)
    syncDetailsContainerView.addArrangedSubview(codewordsView)
    
    qrCodeContainerView.snp.makeConstraints {
      $0.leading.trailing.equalToSuperview().inset(12)
      $0.width.equalTo(qrCodeContainerView.snp.height)
    }
    
    qrCodeContainerView.addSubview(qrCodeView)
    
    qrCodeView.snp.makeConstraints {
      $0.edges.equalToSuperview().inset(12)
    }
    
    codewordsView.snp.makeConstraints {
      $0.edges.equalToSuperview()
    }
    
    stackView.addArrangedSubview(syncDetailsContainerView)

    // Copy - Paste - Done Button
    doneButton.snp.makeConstraints {
      $0.height.equalTo(40)
    }
    doneEnterWordsStackView.addArrangedSubview(copyPasteButton)
    doneEnterWordsStackView.addArrangedSubview(doneButton)
    stackView.addArrangedSubview(doneEnterWordsStackView)
  }

  // MARK: Private
  
  private func showInitializationError() {
    present(SyncAlerts.initializationError, animated: true)
  }
  
  private func updateLabels() {
    let isFirstIndex = modeControl.selectedSegmentIndex == 0

    titleLabel.text = isFirstIndex ? Strings.syncAddDeviceScan : Strings.syncAddDeviceWords

    if isFirstIndex {
      let description = Strings.syncAddDeviceScanDescription
      let attributedDescription = NSMutableAttributedString(string: description)

      if let lastSentenceRange = lastSentenceRange(text: description) {
        attributedDescription.addAttribute(.foregroundColor, value: UIColor.braveErrorLabel, range: lastSentenceRange)
      }

      descriptionLabel.attributedText = attributedDescription
    } else {
      // The button name should be the same as in codewords instructions.
      let buttonName = Strings.scanSyncCode
      let addDeviceWords = String(format: Strings.syncAddDeviceWordsDescription, buttonName)
      let description = NSMutableAttributedString(string: addDeviceWords)
      let fontSize = descriptionLabel.font.pointSize

      let boldRange = (addDeviceWords as NSString).range(of: buttonName)
      description.addAttribute(.font, value: UIFont.boldSystemFont(ofSize: fontSize), range: boldRange)

      if let lastSentenceRange = lastSentenceRange(text: addDeviceWords) {
        description.addAttribute(.foregroundColor, value: UIColor.braveErrorLabel, range: lastSentenceRange)
      }

      descriptionLabel.attributedText = description
    }
  }

  private func lastSentenceRange(text: String) -> NSRange? {
    guard let lastSentence = text.split(separator: "\n").last else { return nil }
    return (text as NSString).range(of: String(lastSentence))
  }
  
  private func enableDeviceType() {
    if deviceType == .computer {
      showCodewords()
    }
  }
}

// MARK: Actions

extension SyncAddDeviceViewController {
  @objc func showCodewords() {
    modeControl.selectedSegmentIndex = 1
    changeMode()
  }

  @objc func copyToClipboard() {
    if let words = self.codewordsView.text {
      UIPasteboard.general.setSecureString(words, expirationDate: Date().addingTimeInterval(30))
      copyButtonPressed = true
    }
  }

  @objc func changeMode() {
    let isFirstIndex = modeControl.selectedSegmentIndex == 0

    qrCodeContainerView.isHidden = !isFirstIndex
    codewordsView.isHidden = isFirstIndex
    copyPasteButton.isHidden = isFirstIndex
    
    updateLabels()
  }

  @objc func done() {
    addDeviceHandler?()
  }
}
