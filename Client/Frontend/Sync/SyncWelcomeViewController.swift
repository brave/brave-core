/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import Shared
import Data
import BraveShared

/// Sometimes during heavy operations we want to prevent user from navigating back, changing screen etc.
protocol NavigationPrevention {
    func enableNavigationPrevention()
    func disableNavigationPrevention()
}

class SyncWelcomeViewController: SyncViewController {
    var dismissHandler: (() -> Void)?
    
    lazy var mainStackView: UIStackView = {
        let stackView = UIStackView()
        stackView.axis = .vertical
        stackView.distribution = .equalSpacing
        stackView.alignment = .fill
        stackView.spacing = 8
        return stackView
    }()

    lazy var syncImage: UIImageView = {
        let imageView = UIImageView(image: UIImage(named: "sync-art"))
        // Shrinking image a bit on smaller devices.
        imageView.setContentCompressionResistancePriority(UILayoutPriority(rawValue: 250), for: .vertical)
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
        label.textColor = BraveUX.GreyJ
        label.text = Strings.BraveSync
        label.textAlignment = .center
        return label
    }()

    lazy var descriptionLabel: UILabel = {
        let label = UILabel()
        label.translatesAutoresizingMaskIntoConstraints = false
        label.font = UIFont.systemFont(ofSize: 15, weight: UIFont.Weight.regular)
        label.textColor = BraveUX.GreyH
        label.numberOfLines = 0
        label.lineBreakMode = .byWordWrapping
        label.textAlignment = .center
        label.text = Strings.BraveSyncWelcome
        label.setContentHuggingPriority(UILayoutPriority(rawValue: 250), for: .horizontal)

        return label
    }()

    lazy var buttonsStackView: UIStackView = {
        let stackView = UIStackView()
        stackView.axis = .vertical
        stackView.spacing = 4
        return stackView
    }()

    lazy var existingUserButton: RoundInterfaceButton = {
        let button = RoundInterfaceButton(type: .roundedRect)
        button.translatesAutoresizingMaskIntoConstraints = false
        button.setTitle(Strings.ScanSyncCode, for: .normal)
        button.titleLabel?.font = UIFont.systemFont(ofSize: 17, weight: UIFont.Weight.bold)
        button.setTitleColor(UIColor.white, for: .normal)
        button.backgroundColor = BraveUX.BraveOrange
        button.addTarget(self, action: #selector(existingUserAction), for: .touchUpInside)

        button.snp.makeConstraints { make in
            make.height.equalTo(50)
        }

        return button
    }()

    lazy var newToSyncButton: RoundInterfaceButton = {
        let button = RoundInterfaceButton(type: .roundedRect)
        button.translatesAutoresizingMaskIntoConstraints = false
        button.setTitle(Strings.NewSyncCode, for: .normal)
        button.titleLabel?.font = UIFont.systemFont(ofSize: 15, weight: UIFont.Weight.semibold)
        button.setTitleColor(BraveUX.GreyH, for: .normal)
        button.addTarget(self, action: #selector(newToSyncAction), for: .touchUpInside)
        return button
    }()
    
    override func viewDidLoad() {
        super.viewDidLoad()

        title = Strings.Sync

        view.addSubview(mainStackView)
        mainStackView.snp.makeConstraints { make in
            make.top.equalTo(self.view.safeArea.top)
            // This VC doesn't rotate, no need to check for left and right safe area constraints.
            make.left.right.equalTo(self.view).inset(16)
            make.bottom.equalTo(self.view.safeArea.bottom).inset(32)
        }

        // Adding top margin to the image.
        let syncImageStackView = UIStackView(arrangedSubviews: [UIView.spacer(.vertical, amount: 60), syncImage])
        syncImageStackView.axis = .vertical
        mainStackView.addArrangedSubview(syncImageStackView)

        textStackView.addArrangedSubview(titleLabel)
        // Side margins for description text.
        let descriptionStackView = UIStackView(arrangedSubviews: [UIView.spacer(.horizontal, amount: 8),
                                                                  descriptionLabel,
                                                                  UIView.spacer(.horizontal, amount: 8)])

        textStackView.addArrangedSubview(descriptionStackView)
        mainStackView.addArrangedSubview(textStackView)

        buttonsStackView.addArrangedSubview(existingUserButton)
        buttonsStackView.addArrangedSubview(newToSyncButton)
        mainStackView.addArrangedSubview(buttonsStackView)
    }
    
    @objc func newToSyncAction() {
        let addDevice = SyncSelectDeviceTypeViewController()
        addDevice.syncInitHandler = { (title, type) in
            weak var weakSelf = self
            func pushAddDeviceVC() {
                guard Sync.shared.isInSyncGroup else {
                    addDevice.disableNavigationPrevention()
                    let alert = UIAlertController(title: Strings.SyncUnsuccessful, message: Strings.SyncUnableCreateGroup, preferredStyle: .alert)
                    alert.addAction(UIAlertAction(title: Strings.OKString, style: .default, handler: nil))
                    addDevice.present(alert, animated: true, completion: nil)
                    return
                }

                let view = SyncAddDeviceViewController(title: title, type: type)
                view.doneHandler = self.pushSettings
                view.navigationItem.hidesBackButton = true
                weakSelf?.navigationController?.pushViewController(view, animated: true)
            }
            
            if Sync.shared.isInSyncGroup {
                pushAddDeviceVC()
                return
            }

            addDevice.enableNavigationPrevention()
            self.addSyncReadyNotificationObserver { pushAddDeviceVC() }
            
            Sync.shared.initializeNewSyncGroup(deviceName: UIDevice.current.name)
        }

        navigationController?.pushViewController(addDevice, animated: true)
    }
    
    @objc func existingUserAction() {
        let pairCamera = SyncPairCameraViewController()
        
        pairCamera.syncHandler = { bytes in
            pairCamera.enableNavigationPrevention()
            Sync.shared.initializeSync(seed: bytes, deviceName: UIDevice.current.name)

            self.addSyncReadyNotificationObserver {
                pairCamera.disableNavigationPrevention()
                self.pushSettings()
            }
        }
        
        navigationController?.pushViewController(pairCamera, animated: true)
    }

    private func addSyncReadyNotificationObserver(completion: @escaping () -> Void) {
        NotificationCenter.default.addObserver(forName: NSNotification.Name(rawValue: NotificationSyncReady),
                                               object: nil,
                                               queue: OperationQueue.main,
                                               using: { notification in
                                                completion()
                                                // This is a one-time notification, removing it immediately.
                                                NotificationCenter.default.removeObserver(notification)
        })
    }
    
    private func pushSettings() {
        let syncSettingsVC = SyncSettingsTableViewController(style: .grouped)
        syncSettingsVC.dismissHandler = dismissHandler
        syncSettingsVC.disableBackButton = true
        navigationController?.pushViewController(syncSettingsVC, animated: true)
    }
}
