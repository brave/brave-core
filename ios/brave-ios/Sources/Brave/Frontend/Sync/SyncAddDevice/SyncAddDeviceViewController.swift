// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShared
import BraveUI
import BraveWallet
import Data
import Shared
import UIKit

enum SyncDeviceType {
  case mobile
  case computer
}

class SyncAddDeviceViewController: SyncViewController {

  // MARK: UX

  private var scrollViewContainer = UIScrollView()

  private var contentStackView = UIStackView().then {
    $0.axis = .vertical
    $0.spacing = 18
  }

  private var modeControl = UISegmentedControl(items: [Strings.Sync.QRCode, Strings.Sync.codeWords])
    .then {
      $0.translatesAutoresizingMaskIntoConstraints = false
      $0.selectedSegmentIndex = 0
      $0.setContentCompressionResistancePriority(.defaultHigh, for: .vertical)
      $0.selectedSegmentTintColor = UIColor(braveSystemName: .containerBackground)
      $0.setTitleTextAttributes(
        [.foregroundColor: UIColor(braveSystemName: .textPrimary)],
        for: .selected
      )
    }

  private let titleDescriptionStackView = SyncAddDeviceInformationView()

  private let codeDetailsStackView: SyncAddDeviceCodeView

  private let codeExpirationStackView = SyncAddDeviceCodeExpirationView()

  private let actionButtonStackView = SyncAddDeviceActionView()

  // MARK: Internal

  private var pageTitle: String = Strings.Sync.syncTitle

  private var deviceType: SyncDeviceType = .mobile

  private var isSyncCodeExpired = false {
    didSet {
      changeCodeDisplayStatus()
    }
  }

  private var isSyncCodePresented: Bool {
    modeControl.selectedSegmentIndex == 0
  }

  private let syncAPI: BraveSyncAPI

  var addDeviceHandler: (() -> Void)?

  // MARK: Lifecycle

  init(title: String, type: SyncDeviceType, syncAPI: BraveSyncAPI) {
    self.syncAPI = syncAPI
    codeDetailsStackView = SyncAddDeviceCodeView(syncAPI: syncAPI)

    super.init()

    resetExpirationDate()

    pageTitle = title
    deviceType = type

    view.maximumContentSizeCategory = .extraLarge
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

    if !syncAPI.isInSyncGroup && !syncAPI.isSyncFeatureActive
      && !syncAPI.isInitialSyncFeatureSetupComplete
    {
      showInitializationError()
    }
  }

  override func viewDidLayoutSubviews() {
    super.viewDidLayoutSubviews()

    scrollViewContainer.contentSize = contentStackView.frame.size

    titleDescriptionStackView.updateLabels(isFirstIndex: modeControl.selectedSegmentIndex == 0)
  }

  private func setTheme() {
    title =
      deviceType == .computer ? Strings.Sync.addComputerTitle : Strings.Sync.addTabletOrPhoneTitle

    modeControl.do {
      $0.isHidden = deviceType == .computer
      $0.addTarget(self, action: #selector(changeMode), for: .valueChanged)
    }

    codeExpirationStackView.delegate = self
    actionButtonStackView.delegate = self
  }

  private func doLayout() {
    view.addSubview(scrollViewContainer)
    view.addSubview(actionButtonStackView)

    // Content StackView
    scrollViewContainer.addSubview(contentStackView)
    contentStackView.snp.makeConstraints {
      $0.top.bottom.equalToSuperview()
      $0.leading.trailing.equalTo(view).inset(24)
      $0.width.equalToSuperview()
    }

    // Segmented Control
    contentStackView.addArrangedSubview(modeControl)

    // Title - Description Label
    contentStackView.addArrangedSubview(titleDescriptionStackView)

    // Code Words View - QR Code
    contentStackView.addArrangedSubview(codeDetailsStackView)

    // Code Expiration Label Image
    contentStackView.addArrangedSubview(codeExpirationStackView)

    // Copy - Paste - Done Button
    actionButtonStackView.snp.makeConstraints {
      $0.top.equalTo(scrollViewContainer.snp.bottom).inset(-24)
      $0.left.right.equalTo(view).inset(24)
      $0.bottom.equalTo(view.safeArea.bottom).inset(24)
    }

    // Scroll View
    scrollViewContainer.snp.makeConstraints {
      $0.top.equalTo(view.safeArea.top).inset(10)
      $0.left.right.equalTo(view)
    }

    changeCodeDisplayStatus()
  }

  // MARK: Private

  private func fetchExpirationDate(for type: SyncDeviceType, using syncAPI: BraveSyncAPI) -> Date? {
    if type == .mobile, modeControl.selectedSegmentIndex == 0 {
      let json = syncAPI.qrCodeJson(
        fromHexSeed: syncAPI.hexSeed(fromSyncCode: syncAPI.getSyncCode())
      )

      guard let syncCodeData = json.data(using: .utf8), !syncCodeData.isEmpty,
        let syncCodeJsonObject = try? JSONSerialization.jsonObject(with: syncCodeData, options: [])
          as? [String: Any]
      else {
        return nil
      }

      guard let expiration = syncCodeJsonObject["not_after"] as? String,
        let expirationSeconds = Double(expiration)
      else {
        return nil
      }

      return Date(timeIntervalSince1970: expirationSeconds)
    } else {
      let timeLimitedWords = syncAPI.getTimeLimitedWords(fromWords: syncAPI.getSyncCode())
      return syncAPI.getExpirationFromTimeLimitedWords(timeLimitedWords)
    }
  }

  private func resetExpirationDate() {
    if let expiryDate = fetchExpirationDate(for: deviceType, using: syncAPI) {
      codeExpirationStackView.resetExpiration(expirationTime: expiryDate)
    }
  }

  private func showInitializationError() {
    present(SyncAlerts.initializationError, animated: true)
  }

  private func enableDeviceType() {
    if deviceType == .computer {
      showCodewords()
    }
  }

  private func changeCodeDisplayStatus() {
    resetExpirationDate()

    if isSyncCodeExpired {
      codeDetailsStackView.isHidden = true
      actionButtonStackView.swapCodeViewType(true)
    } else {
      codeDetailsStackView.isHidden = false
      changeSyncCodeStatus()
    }

    codeExpirationStackView.swapCodeExpirationType(isSyncCodeExpired)
    actionButtonStackView.swapCodeExpirationType(
      isSyncCodePresented: isSyncCodePresented,
      isExpired: isSyncCodeExpired
    )
  }

  private func changeSyncCodeStatus() {
    codeDetailsStackView.swapCodeViewType(isSyncCodePresented)
    actionButtonStackView.swapCodeViewType(isSyncCodePresented)
  }

  private func showCodewords() {
    modeControl.selectedSegmentIndex = 1
    changeMode()
  }

  @objc private func changeMode() {
    changeCodeDisplayStatus()
    titleDescriptionStackView.updateLabels(isFirstIndex: modeControl.selectedSegmentIndex == 0)
  }
}

// MARK: Action-Delegate

extension SyncAddDeviceViewController: SyncAddDeviceActionView.ActionDelegate {

  func copyToClipboard() {
    if let words = codeDetailsStackView.syncChainCode {
      UIPasteboard.general.setSecureString(words, expirationDate: Date().addingTimeInterval(30))
    }
  }

  func dismiss() {
    addDeviceHandler?()
  }

  func generateNewCode() {
    codeDetailsStackView.refreshSyncCodeViews()
    // Refresh with proper API
    if let expiryDate = fetchExpirationDate(for: deviceType, using: syncAPI) {
      codeExpirationStackView.resetExpiration(expirationTime: expiryDate)
    }

    isSyncCodeExpired = false

    changeCodeDisplayStatus()
  }
}

extension SyncAddDeviceViewController: SyncAddDeviceCodeExpirationView.ActionDelegate {

  func codeExpired() {
    isSyncCodeExpired = true
    changeCodeDisplayStatus()
  }
}
