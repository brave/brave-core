/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import LocalAuthentication
import Static
import SwiftKeychainWrapper
import Shared

class PasscodeSettingsViewController: TableViewController {
    
    init() {
        super.init(style: .grouped)
    }
    
    @available(*, unavailable)
    required init?(coder aDecoder: NSCoder) {
        fatalError()
    }
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        updateTitleForTouchIDState()
        
        tableView.accessibilityIdentifier = "PasscodeSettingsViewController.tableView"
        tableView.separatorColor = UIConstants.TableViewSeparatorColor
        tableView.backgroundColor = UIConstants.TableViewHeaderBackgroundColor
    }
    
    override func viewWillAppear(_ animated: Bool) {
        reloadSections()
    }
    
    func reloadSections() {
        if let authenticationInfo = KeychainWrapper.sharedAppContainerKeychain.authenticationInfo() {
            // Passcode
            dataSource.sections = [
                Section(rows: [
                    Row(text: Strings.AuthenticationTurnOffPasscode,
                        selection: { [unowned self] in
                            let setupPasscodeController = RemovePasscodeViewController()
                            let container = UINavigationController(rootViewController: setupPasscodeController)
                            self.present(container, animated: true)
                        },
                        cellClass: ButtonCell.self
                    ),
                    Row(text: Strings.AuthenticationChangePasscode,
                        selection: { [unowned self] in
                            let changePasscodeController = ChangePasscodeViewController()
                            let container = UINavigationController(rootViewController: changePasscodeController)
                            self.present(container, animated: true)
                        }
                    )
                    ])
            ]
            
            var otherSection = Section(rows: [
                // TODO: Need localized copy of this
                Row(text: "Require Passcode Immediately",
                    accessory: .switchToggle(
                        value: authenticationInfo.isPasscodeRequiredImmediately,
                        // TODO: Make a new option "infinite" instead of using time intervals
                        { on in authenticationInfo.isPasscodeRequiredImmediately = on; KeychainWrapper.sharedAppContainerKeychain.setAuthenticationInfo(authenticationInfo) }
                    )
                )
            ])
            
            if deviceBiometryType != .none {
                let title = deviceBiometryType == .faceID ? Strings.UseFaceID : Strings.UseTouchID
                otherSection.rows.append(Row(text: title, accessory: .switchToggle(value: authenticationInfo.useTouchID, { authenticationInfo.useTouchID = $0; KeychainWrapper.sharedAppContainerKeychain.setAuthenticationInfo(authenticationInfo) })))
            }
            
            dataSource.sections.append(otherSection)
        } else {
            // No Passcode
            dataSource.sections = [
                Section(rows: [
                    Row(text: Strings.AuthenticationTurnOnPasscode,
                        selection: { [unowned self] in
                            let setupPasscodeController = SetupPasscodeViewController()
                            let container = UINavigationController(rootViewController: setupPasscodeController)
                            self.present(container, animated: true)
                        },
                        cellClass: ButtonCell.self
                    ),
                    Row(text: Strings.AuthenticationChangePasscode, cellClass: DisabledCell.self)
                ]),
                Section(rows: [
                    // TODO: Need localized copy of this
                    Row(text: "Require Passcode Immediately", accessory: .switchToggle(value: false, { _ in }), cellClass: DisabledCell.self),
                ])
            ]
        }
    }
    
    private var deviceBiometryType: LABiometryType {
        let context = LAContext()
        if context.canEvaluatePolicy(.deviceOwnerAuthenticationWithBiometrics, error: nil) {
            return context.biometryType
        }
        return .none
    }
    
    func updateTitleForTouchIDState() {
        switch deviceBiometryType {
        case .faceID:
            navigationItem.title = Strings.AuthenticationFaceIDPasscodeSetting
        case .touchID:
            navigationItem.title = Strings.AuthenticationTouchIDPasscodeSetting
        case .none:
            navigationItem.title = Strings.AuthenticationPasscode
        }
    }
}

class DisabledCell: Value1Cell {
    override init(style: UITableViewCell.CellStyle, reuseIdentifier: String?) {
        super.init(style: style, reuseIdentifier: reuseIdentifier)
        
        selectionStyle = .none
        textLabel?.textColor = .lightGray
    }
    
    override var accessoryView: UIView? {
        didSet {
            if let control = accessoryView as? UIControl {
                control.isEnabled = false
            }
        }
    }
    
    @available(*, unavailable)
    required init?(coder aDecoder: NSCoder) {
        fatalError()
    }
}
