/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import LocalAuthentication
import Static
import SwiftKeychainWrapper
import Shared
import BraveUI
import BraveShared

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
        
        navigationItem.title = titleForTouchIDState
        
        tableView.accessibilityIdentifier = "PasscodeSettingsViewController.tableView"
        tableView.separatorColor = .braveSeparator
        tableView.backgroundColor = .braveGroupedBackground
    }
    
    override func viewWillAppear(_ animated: Bool) {
        reloadSections()
    }
    
    func reloadSections() {
        dataSource.sections = [
            .init(
                rows: [ .boolRow(title: titleForTouchIDState, option: Preferences.Privacy.lockWithPasscode) ]
            )
        ]
    }
    
    private var deviceBiometryType: LABiometryType {
        let context = LAContext()
        if context.canEvaluatePolicy(.deviceOwnerAuthenticationWithBiometrics, error: nil) {
            return context.biometryType
        }
        return .none
    }
    
    var titleForTouchIDState: String {
        switch deviceBiometryType {
        case .faceID:
            return Strings.authenticationFaceIDPasscodeSetting
        case .touchID:
            return Strings.authenticationTouchIDPasscodeSetting
        case .none:
            return Strings.authenticationPasscode
        @unknown default:
            return Strings.authenticationPasscode
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
