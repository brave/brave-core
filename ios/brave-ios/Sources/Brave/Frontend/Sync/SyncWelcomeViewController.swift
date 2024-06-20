// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShared
import BraveUI
import Data
import Preferences
import Shared
import UIKit
import os.log

/// Sometimes during heavy operations we want to prevent user from navigating back, changing screen etc.
protocol NavigationPrevention {
  func enableNavigationPrevention()
  func disableNavigationPrevention()
}

class SyncWelcomeViewController: SyncViewController {

  private enum ActionType {
    case newUser, existingUser, internalSettings
  }

  private enum SyncDeviceLimitLevel {
    case safe, approvalNeeded, blocked
  }

  private enum DeviceRetriavalError: Error {
    case decodeError, fetchError, deviceNumberError

    // Text localization not necesseray, only used for logs
    var errorDescription: String {
      switch self {
      case .decodeError:
        return "Decoding Error while retrieving list of sync devices"
      case .fetchError:
        return "Fetch Error while retrieving list of sync devices"
      case .deviceNumberError:
        return "Incorrect number while retrieving list of sync devices"
      }
    }
  }

  private var overlayView: UIView?

  override var isLoading: Bool {
    didSet {
      overlayView?.removeFromSuperview()

      // Toggle 'restore' button.
      navigationItem.rightBarButtonItem?.isEnabled = !isLoading

      // Prevent dismissing the modal by swipe when migration happens.
      navigationController?.isModalInPresentation = isLoading == true

      if !isLoading { return }

      let overlay = UIView().then {
        $0.backgroundColor = UIColor.black.withAlphaComponent(0.5)
        let activityIndicator = UIActivityIndicatorView().then { indicator in
          indicator.startAnimating()
          indicator.autoresizingMask = [.flexibleWidth, .flexibleHeight]
        }

        $0.addSubview(activityIndicator)
      }

      view.addSubview(overlay)
      overlay.snp.makeConstraints {
        $0.edges.equalToSuperview()
      }

      overlayView = overlay
    }
  }

  private var syncServiceObserver: AnyObject?
  private var syncDeviceInfoObserver: AnyObject?
  private let tabManager: TabManager

  lazy var mainStackView: UIStackView = {
    let stackView = UIStackView()
    stackView.axis = .vertical
    stackView.distribution = .equalSpacing
    stackView.alignment = .fill
    stackView.spacing = 8
    return stackView
  }()

  lazy var syncImage: UIImageView = {
    let imageView = UIImageView(image: UIImage(named: "sync-art", in: .module, compatibleWith: nil))
    // Shrinking image a bit on smaller devices.
    imageView.setContentCompressionResistancePriority(
      UILayoutPriority(rawValue: 250),
      for: .vertical
    )
    imageView.contentMode = .scaleAspectFit

    return imageView
  }()

  lazy var textStackView: UIStackView = {
    let stackView = UIStackView()
    stackView.axis = .vertical
    stackView.spacing = 4
    return stackView
  }()

  lazy var titleLabel: UILabel = {
    let label = UILabel()
    label.translatesAutoresizingMaskIntoConstraints = false
    label.font = UIFont.systemFont(ofSize: 20, weight: UIFont.Weight.semibold)
    label.text = Strings.Sync.syncTitle
    label.textAlignment = .center
    return label
  }()

  lazy var descriptionLabel: UILabel = {
    let label = UILabel()
    label.translatesAutoresizingMaskIntoConstraints = false
    label.font = UIFont.systemFont(ofSize: 15, weight: UIFont.Weight.regular)
    label.numberOfLines = 0
    label.lineBreakMode = .byWordWrapping
    label.textAlignment = .center
    label.text = Strings.Sync.welcome
    label.setContentHuggingPriority(UILayoutPriority(rawValue: 250), for: .horizontal)

    return label
  }()

  lazy var buttonsStackView: UIStackView = {
    let stackView = UIStackView()
    stackView.axis = .vertical
    stackView.spacing = 4
    return stackView
  }()

  lazy var newToSyncButton: RoundInterfaceButton = {
    let button = RoundInterfaceButton(type: .roundedRect)
    button.translatesAutoresizingMaskIntoConstraints = false
    button.setTitle(Strings.Sync.newSyncCode, for: .normal)
    button.titleLabel?.font = UIFont.systemFont(ofSize: 17, weight: UIFont.Weight.bold)
    button.setTitleColor(.white, for: .normal)
    button.backgroundColor = .braveBlurpleTint
    button.addTarget(self, action: #selector(newToSyncAction), for: .touchUpInside)

    button.snp.makeConstraints { make in
      make.height.equalTo(50)
    }

    return button
  }()

  lazy var existingUserButton: RoundInterfaceButton = {
    let button = RoundInterfaceButton(type: .roundedRect)
    button.translatesAutoresizingMaskIntoConstraints = false
    button.setTitle(Strings.Sync.scanSyncCode, for: .normal)
    button.titleLabel?.font = UIFont.systemFont(ofSize: 15, weight: UIFont.Weight.semibold)
    button.setTitleColor(.braveLabel, for: .normal)
    button.addTarget(self, action: #selector(existingUserAction), for: .touchUpInside)
    return button
  }()

  private let syncAPI: BraveSyncAPI
  private let syncProfileServices: BraveSyncProfileServiceIOS

  init(
    syncAPI: BraveSyncAPI,
    syncProfileServices: BraveSyncProfileServiceIOS,
    tabManager: TabManager,
    windowProtection: WindowProtection?,
    isModallyPresented: Bool = false
  ) {
    self.syncAPI = syncAPI
    self.syncProfileServices = syncProfileServices
    self.tabManager = tabManager

    super.init(windowProtection: windowProtection, isModallyPresented: isModallyPresented)
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }

  override func viewDidLoad() {
    super.viewDidLoad()

    title = Strings.Sync.syncTitle

    view.addSubview(mainStackView)
    mainStackView.snp.makeConstraints { make in
      make.top.equalTo(self.view.safeArea.top)
      // This VC doesn't rotate, no need to check for left and right safe area constraints.
      make.left.right.equalTo(self.view).inset(16)
      make.bottom.equalTo(self.view.safeArea.bottom).inset(32)
    }

    // Adding top margin to the image.
    let syncImageStackView = UIStackView(arrangedSubviews: [
      UIView.spacer(.vertical, amount: 60), syncImage,
    ])
    syncImageStackView.axis = .vertical
    mainStackView.addArrangedSubview(syncImageStackView)

    textStackView.addArrangedSubview(titleLabel)
    // Side margins for description text.
    let descriptionStackView = UIStackView(arrangedSubviews: [
      UIView.spacer(.horizontal, amount: 8),
      descriptionLabel,
      UIView.spacer(.horizontal, amount: 8),
    ])

    textStackView.addArrangedSubview(descriptionStackView)
    mainStackView.addArrangedSubview(textStackView)

    buttonsStackView.addArrangedSubview(existingUserButton)
    buttonsStackView.addArrangedSubview(newToSyncButton)
    mainStackView.addArrangedSubview(buttonsStackView)

    navigationItem.rightBarButtonItem = UIBarButtonItem(
      image: UIImage(systemName: "gearshape"),
      style: .plain,
      target: self,
      action: #selector(onSyncInternalsAction)
    )
  }

  // MARK: Actions

  @objc
  private func newToSyncAction() {
    let addDevice = SyncSelectDeviceTypeViewController()
    addDevice.syncInitHandler = { [weak self] (title, type) in
      guard let self = self else { return }

      func pushAddDeviceVC() {
        self.syncServiceObserver = nil
        guard self.syncAPI.isInSyncGroup else {
          addDevice.disableNavigationPrevention()
          let alert = UIAlertController(
            title: Strings.Sync.syncUnsuccessful,
            message: Strings.Sync.unableCreateGroup,
            preferredStyle: .alert
          )
          alert.addAction(UIAlertAction(title: Strings.OKString, style: .default, handler: nil))
          addDevice.present(alert, animated: true, completion: nil)
          return
        }

        let view = SyncAddDeviceViewController(title: title, type: type, syncAPI: self.syncAPI)
        view.addDeviceHandler = self.pushSettings
        view.navigationItem.hidesBackButton = true
        self.navigationController?.pushViewController(view, animated: true)
      }

      if self.syncAPI.isInSyncGroup {
        pushAddDeviceVC()
        return
      }

      addDevice.enableNavigationPrevention()

      // DidJoinSyncChain result should be also checked when creating a new chain
      self.syncAPI.setDidJoinSyncChain { result in
        if result {
          self.syncDeviceInfoObserver = self.syncAPI.addDeviceStateObserver { [weak self] in
            guard let self else { return }
            self.syncServiceObserver = nil
            self.syncDeviceInfoObserver = nil

            pushAddDeviceVC()
          }
        } else {
          self.syncAPI.leaveSyncGroup()
          addDevice.disableNavigationPrevention()
          self.navigationController?.popViewController(animated: true)
        }
      }

      self.syncAPI.joinSyncGroup(
        codeWords: self.syncAPI.getSyncCode(),
        syncProfileService: self.syncProfileServices
      )
      self.handleSyncSetupFailure()
    }

    navigationController?.pushViewController(addDevice, animated: true)
  }

  @objc
  private func existingUserAction() {
    if ProcessInfo.processInfo.isiOSAppOnVisionOS {
      // VisionOS can't access the main camera, so we skip showing QR code scan screen entirely
      let wordsVC = SyncPairWordsViewController(syncAPI: syncAPI)
      wordsVC.delegate = self
      navigationController?.pushViewController(wordsVC, animated: true)
      return
    }
    let pairCamera = SyncPairCameraViewController(syncAPI: syncAPI)
    pairCamera.delegate = self
    navigationController?.pushViewController(pairCamera, animated: true)
  }

  @objc
  private func onSyncInternalsAction() {
    askForAuthentication(viewType: .sync) { [weak self] status, error in
      guard let self = self, status else { return }

      let syncInternalsController = ChromeWebViewController(privateBrowsing: false).then {
        $0.title = Strings.Sync.internalsTitle
        $0.loadURL("brave://sync-internals")
      }

      navigationController?.pushViewController(syncInternalsController, animated: true)
    }
  }

  // MARK: Internal

  private func pushSettings() {
    if !DeviceInfo.hasConnectivity() {
      present(SyncAlerts.noConnection, animated: true)
      return
    }

    let syncSettingsVC = SyncSettingsTableViewController(
      isModallyPresented: true,
      syncAPI: syncAPI,
      syncProfileService: syncProfileServices,
      tabManager: tabManager,
      windowProtection: windowProtection
    )

    navigationController?.pushViewController(syncSettingsVC, animated: true)
  }

  /// Sync setup failure is handled here because it can happen from few places in children VCs(new chain, qr code, codewords)
  /// This makes all presented Sync View Controllers to dismiss, cleans up any sync setup and shows user a friendly message.
  private func handleSyncSetupFailure() {
    syncServiceObserver = syncAPI.addServiceStateObserver { [weak self] in
      guard let self = self else { return }

      if !self.syncAPI.isInSyncGroup && !self.syncAPI.isSyncFeatureActive
        && !self.syncAPI.isInitialSyncFeatureSetupComplete
      {
        let bvc = self.currentScene?.browserViewController
        self.dismiss(animated: true) {
          bvc?.present(SyncAlerts.initializationError, animated: true)
        }
      }
    }
  }
}

extension SyncWelcomeViewController: SyncPairControllerDelegate {
  func syncOnScannedHexCode(_ controller: UIViewController & NavigationPrevention, hexCode: String)
  {
    syncOnWordsEntered(
      controller,
      codeWords: syncAPI.syncCode(fromHexSeed: hexCode),
      isCodeScanned: true
    )
  }

  func syncOnWordsEntered(
    _ controller: UIViewController & NavigationPrevention,
    codeWords: String,
    isCodeScanned: Bool
  ) {
    // Start Loading after Sync chain code words are entered
    controller.enableNavigationPrevention()

    // Join the sync chain but do not enable any sync type
    // This includes the bookmark default type
    runPostJoinSyncActions(using: controller, isCodeScanned: isCodeScanned)

    // In parallel set code words - request sync and setup complete
    // should be called on brave-core side
    syncAPI.joinSyncGroup(codeWords: codeWords, syncProfileService: syncProfileServices)
    handleSyncSetupFailure()
  }

  private func runPostJoinSyncActions(
    using controller: UIViewController & NavigationPrevention,
    isCodeScanned: Bool
  ) {
    // DidJoinSyncChain is checking If the chain user trying to join is deleted recently
    // returning an error accordingly - only error is Deleted Sync Chain atm
    syncAPI.setDidJoinSyncChain { result in
      if result {
        // If chain is not deleted start listening for device state observer
        // to validate devices are added to chain and show settings
        self.syncDeviceInfoObserver = self.syncAPI.addDeviceStateObserver { [weak self] in
          guard let self else { return }
          self.syncServiceObserver = nil
          self.syncDeviceInfoObserver = nil

          // Stop Loading after device list can be fetched
          controller.disableNavigationPrevention()

          let deviceLimit = retrieveDeviceLimitLevel()
          guard let deviceLimitLevel = deviceLimit.level else {
            clearSyncChainWithAlert(
              title: Strings.genericErrorTitle,
              message: Strings.Sync.deviceFetchErrorAlertDescription,
              controller: controller
            )
            return
          }

          switch deviceLimitLevel {
          case .safe:
            self.enableDefaultTypeAndPushSettings()
          case .approvalNeeded:
            let devicesSyncChain = fetchNamesOfDevicesInSyncChain()

            // Showing and alert with device list; if user answers no - leave chain, if yes - enable the bookmarks type
            var alertMessage = ""

            if !devicesSyncChain.isEmpty {
              for device in devicesSyncChain where !device.name.isEmpty {
                if device.isCurrentDevice {
                  var currentDeviceNameList = "\n\(device.name) (\(Strings.Sync.thisDevice))"
                  currentDeviceNameList += alertMessage
                  alertMessage = currentDeviceNameList
                } else {
                  alertMessage += "\n\(device.name)"
                }
              }

              alertMessage = "\n\(Strings.Sync.devicesInSyncChainTitle):\n" + alertMessage
            }

            alertMessage += "\n\n \(Strings.Sync.joinChainCodewordsWarning)"

            let alert = UIAlertController(
              title: Strings.Sync.joinChainWarningTitle,
              message: alertMessage,
              preferredStyle: .alert
            )
            alert.addAction(
              UIAlertAction(title: Strings.cancelButtonTitle, style: .default) { _ in
                self.leaveIncompleteSyncChain()
              }
            )
            alert.addAction(
              UIAlertAction(title: Strings.confirm, style: .default) { _ in
                self.enableDefaultTypeAndPushSettings()
              }
            )
            present(alert, animated: true, completion: nil)
          case .blocked:
            // Devices 10 and more - add alert to block and prevent sync
            let alert = UIAlertController(
              title: Strings.genericErrorTitle,
              message: Strings.Sync.maximumDeviceReachedErrorDescription,
              preferredStyle: .alert
            )
            alert.addAction(
              UIAlertAction(title: Strings.OKString, style: .default) { _ in
                self.leaveIncompleteSyncChain()
              }
            )
            present(alert, animated: true, completion: nil)
          }
        }
      } else {
        // Show an alert if the sync chain is deleted
        // Leave sync chain should be called if there is deleted chain alert
        // to reset sync and local preferences with observer
        self.clearSyncChainWithAlert(
          title: Strings.Sync.chainAlreadyDeletedAlertTitle,
          message: Strings.Sync.chainAlreadyDeletedAlertDescription,
          controller: controller
        )
      }
    }
  }

  private func retrieveDeviceLimitLevel() -> (
    level: SyncDeviceLimitLevel?, error: DeviceRetriavalError?
  ) {
    let deviceListJSON = syncAPI.getDeviceListJSON()
    let deviceList = fetchSyncDeviceList(listJSON: deviceListJSON)

    if let error = deviceList.error {
      return (nil, error)
    }

    guard let devices = deviceList.devices else {
      return (nil, DeviceRetriavalError.deviceNumberError)
    }

    var deviceLimitLevel: SyncDeviceLimitLevel?

    switch devices.count {
    case 1...4:
      deviceLimitLevel = .safe
    case 5...9:
      deviceLimitLevel = .approvalNeeded
    case 10...:
      deviceLimitLevel = .blocked
    default:
      Logger.module.error("\(DeviceRetriavalError.deviceNumberError.errorDescription)")
      return (nil, DeviceRetriavalError.deviceNumberError)
    }

    return (deviceLimitLevel, nil)
  }

  private func fetchNamesOfDevicesInSyncChain() -> [(name: String, isCurrentDevice: Bool)] {
    let deviceListJSON = syncAPI.getDeviceListJSON()
    let deviceList = fetchSyncDeviceList(listJSON: deviceListJSON)

    if deviceList.error != nil {
      return []
    }

    guard let devices = deviceList.devices else {
      return []
    }

    return devices.map { ($0.name ?? "", $0.isCurrentDevice) }
  }

  private func fetchSyncDeviceList(
    listJSON: String?
  ) -> (devices: [BraveSyncDevice]?, error: DeviceRetriavalError?) {
    if let json = listJSON, let data = json.data(using: .utf8) {
      do {
        let devices = try JSONDecoder().decode([BraveSyncDevice].self, from: data)
        return (devices, nil)
      } catch {
        Logger.module.error(
          "\(DeviceRetriavalError.decodeError.errorDescription) - \(error.localizedDescription)"
        )
        return (nil, DeviceRetriavalError.decodeError)
      }
    } else {
      Logger.module.error("\(DeviceRetriavalError.fetchError.errorDescription)")
      return (nil, DeviceRetriavalError.fetchError)
    }
  }

  private func enableDefaultTypeAndPushSettings() {
    // Enable default sync type Bookmarks and push settings
    Preferences.Chromium.syncBookmarksEnabled.value = true
    syncAPI.enableSyncTypes(syncProfileService: syncProfileServices)
    pushSettings()
  }

  private func leaveIncompleteSyncChain() {
    syncAPI.leaveSyncGroup()
    navigationController?.popToRootViewController(animated: true)
  }

  private func clearSyncChainWithAlert(
    title: String,
    message: String,
    controller: UIViewController & NavigationPrevention
  ) {
    let alert = UIAlertController(
      title: title,
      message: message,
      preferredStyle: .alert
    )

    alert.addAction(
      UIAlertAction(title: Strings.OKString, style: .default) { _ in
        self.syncAPI.leaveSyncGroup()

        controller.disableNavigationPrevention()
        self.navigationController?.popViewController(animated: true)
      }
    )

    present(alert, animated: true, completion: nil)
  }
}
