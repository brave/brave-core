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
    $0.spacing = 20
  }

  private var modeControl = UISegmentedControl(items: [Strings.QRCode, Strings.codeWords]).then {
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

  private let actionButtonStackView = UIStackView().then {
    $0.axis = .vertical
    $0.spacing = 4
    $0.distribution = .fillEqually
    $0.setContentCompressionResistancePriority(.required, for: .vertical)
  }

  private var doneButton = UIButton().then {
    $0.translatesAutoresizingMaskIntoConstraints = false
    $0.setTitle(Strings.done, for: .normal)
    $0.titleLabel?.font = UIFont.systemFont(ofSize: 15, weight: UIFont.Weight.semibold)
    $0.setTitleColor(.white, for: .normal)
    $0.backgroundColor = UIColor(braveSystemName: .buttonBackground)
    $0.layer.cornerRadius = 12
    $0.layer.cornerCurve = .continuous
  }

  private lazy var copyPasteButton = UIButton().then {
    $0.setTitle(Strings.copyToClipboard, for: .normal)
    $0.titleLabel?.font = UIFont.systemFont(ofSize: 15, weight: UIFont.Weight.semibold)
    $0.addTarget(self, action: #selector(copyToClipboard), for: .touchUpInside)
    $0.setTitleColor(UIColor(braveSystemName: .textPrimary), for: .normal)
    $0.isHidden = true
  }

  // MARK: Internal

  private var pageTitle: String = Strings.sync

  private var deviceType: SyncDeviceType = .mobile

  private var isSyncCodeExpired = true {
    didSet {
      changeCodeDisplayStatus()
    }
  }

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
    codeDetailsStackView = SyncAddDeviceCodeView(syncAPI: syncAPI)
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

    if !syncAPI.isInSyncGroup && !syncAPI.isSyncFeatureActive
      && !syncAPI.isInitialSyncFeatureSetupComplete
    {
      showInitializationError()
    }
  }

  override func viewDidLayoutSubviews() {
    super.viewDidLayoutSubviews()

    scrollViewContainer.contentSize = CGSize(
      width: contentStackView.frame.width,
      height: contentStackView.frame.height
    )

    titleDescriptionStackView.updateLabels(isFirstIndex: modeControl.selectedSegmentIndex == 0)
  }

  private func setTheme() {
    title =
      deviceType == .computer ? Strings.syncAddComputerTitle : Strings.syncAddTabletOrPhoneTitle

    modeControl.do {
      $0.isHidden = deviceType == .computer
      $0.addTarget(self, action: #selector(changeMode), for: .valueChanged)
    }

    doneButton.addTarget(self, action: #selector(done), for: .touchUpInside)
  }

  private func doLayout() {
    // Scroll View
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

    // Copy - Paste - Done Button
    doneButton.snp.makeConstraints {
      $0.height.equalTo(40)
    }
    actionButtonStackView.addArrangedSubview(copyPasteButton)
    actionButtonStackView.addArrangedSubview(doneButton)

    scrollViewContainer.snp.makeConstraints {
      $0.top.equalTo(view.safeArea.top).inset(10)
      $0.left.right.equalTo(view)
    }

    actionButtonStackView.snp.makeConstraints {
      $0.top.equalTo(scrollViewContainer.snp.bottom).inset(-24)
      $0.left.right.equalTo(view).inset(24)
      $0.bottom.equalTo(view.safeArea.bottom).inset(24)
    }

    changeCodeDisplayStatus()
  }

  // MARK: Private

  private func showInitializationError() {
    present(SyncAlerts.initializationError, animated: true)
  }

  private func enableDeviceType() {
    if deviceType == .computer {
      showCodewords()
    }
  }

  private func changeCodeDisplayStatus() {
    if isSyncCodeExpired {
      // Hide Active Status Elements
      codeDetailsStackView.isHidden = true
      copyPasteButton.isHidden = true

      // Reveal Expired Elements

    } else {
      // Hide Active Status Elements

      // Reveal Active Status Elements
      changeSyncCodeStatus()
    }
  }

  private func changeSyncCodeStatus() {
    let isFirstIndex = modeControl.selectedSegmentIndex == 0

    codeDetailsStackView.swapCodeViewType(isFirstIndex)
    copyPasteButton.isHidden = isFirstIndex
  }
}

// MARK: Actions

extension SyncAddDeviceViewController {
  @objc func showCodewords() {
    modeControl.selectedSegmentIndex = 1
    changeMode()
  }

  @objc func copyToClipboard() {
    if let words = codeDetailsStackView.syncChainCode {
      UIPasteboard.general.setSecureString(words, expirationDate: Date().addingTimeInterval(30))
      copyButtonPressed = true
    }
  }

  @objc func changeMode() {
    changeCodeDisplayStatus()
    titleDescriptionStackView.updateLabels(isFirstIndex: modeControl.selectedSegmentIndex == 0)
  }

  @objc func done() {
    addDeviceHandler?()
  }
}
