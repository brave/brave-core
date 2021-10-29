// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Storage
import Shared
import SwiftKeychainWrapper

private let log = Logger.browserLogger

class LoginInfoViewController: LoginAuthViewController {
    
    // MARK: UX
    
    struct UX {
        static let informationRowHeight: CGFloat = 58
        static let createdRowHeight: CGFloat = 66
        static let standardItemHeight: CGFloat = 44
    }
    
    // MARK: Section
    
    enum Section: Int, CaseIterable {
        case information
        case createdDate
        case delete
    }
    
    // MARK: ItemType
    
    enum InfoItem: Int, CaseIterable {
        case websiteItem
        case usernameItem
        case passwordItem
    }
    
    weak var settingsDelegate: SettingsDelegate?

    // MARK: Private
    
    private let profile: Profile
    private weak var websiteField: UITextField?
    private weak var usernameField: UITextField?
    private weak var passwordField: UITextField?
    
    private var loginEntry: Login {
        didSet {
            tableView.reloadData()
        }
    }
    private var isEditingFieldData: Bool = false {
        didSet {
            if isEditingFieldData != oldValue {
                tableView.reloadData()
            }
        }
    }
    
    private var formattedCreationDate: String {
        let date = Date(timeIntervalSince1970: TimeInterval(loginEntry.timeCreated / 1_000_000))
        let dateFormatter = DateFormatter().then {
            $0.locale = .current
            $0.dateFormat = "EEEE, MMM d, yyyy"
        }

        return dateFormatter.string(from: date)
    }
    
    // MARK: Lifecycle

    init(profile: Profile, loginEntry: Login, windowProtection: WindowProtection?) {
        self.loginEntry = loginEntry
        self.profile = profile
        super.init(windowProtection: windowProtection)
    }

    required init?(coder aDecoder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }

    override func viewDidLoad() {
        super.viewDidLoad()

        navigationItem.do {
            $0.title = URL(string: loginEntry.hostname)?.baseDomain ?? ""
            $0.rightBarButtonItem = UIBarButtonItem(barButtonSystemItem: .edit, target: self, action: #selector(edit))
        }

        tableView.do {
            $0.accessibilityIdentifier = Strings.Login.loginInfoDetailsHeaderTitle
            $0.register(CenteredButtonCell.self)
            $0.register(LoginInfoTableViewCell.self)
            $0.registerHeaderFooter(SettingsTableSectionHeaderFooterView.self)
            $0.tableFooterView = SettingsTableSectionHeaderFooterView(
                frame: CGRect(width: tableView.bounds.width, height: 1.0))
            $0.estimatedRowHeight = 44.0
            #if swift(>=5.5)
            if #available(iOS 15.0, *) {
                $0.sectionHeaderTopPadding = 0
            }
            #endif
        }
    }
}

// MARK: TableViewDataSource - TableViewDelegate

extension LoginInfoViewController {
    
    override func tableView(_ tableView: UITableView, viewForHeaderInSection section: Int) -> UIView? {
        let headerView = tableView.dequeueReusableHeaderFooter() as SettingsTableSectionHeaderFooterView
        headerView.titleLabel.text = Strings.Login.loginInfoDetailsHeaderTitle.uppercased()
        return headerView
    }
    
    override func tableView(_ tableView: UITableView, heightForHeaderInSection section: Int) -> CGFloat {
        switch section {
            case Section.delete.rawValue:
                // This is added because of the undesired separator over the Delete Row.
                // An easy way of keeping separator over the created row but not underneath it
                return 1
            case Section.information.rawValue:
                return UX.standardItemHeight
            default:
                return 0
        }
    }
    
    override func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
        switch indexPath.section {
            case Section.information.rawValue:
                switch indexPath.row {
                    case InfoItem.websiteItem.rawValue:
                        let cell = tableView.dequeueReusableCell(for: indexPath) as LoginInfoTableViewCell
                        
                        cell.do {
                            $0.delegate = self
                            $0.highlightedLabel.text = Strings.Login.loginInfoDetailsWebsiteFieldTitle
                            $0.descriptionTextField.text = loginEntry.hostname
                            $0.isEditingFieldData = false
                            $0.tag = InfoItem.websiteItem.rawValue
                        }
                        
                        websiteField = cell.descriptionTextField
                        websiteField?.accessibilityIdentifier = "websiteField"
                        
                        cell.contentView.alpha = isEditingFieldData ? 0.5 : 1.0
                        
                        return cell
                    case InfoItem.usernameItem.rawValue:
                        let cell = tableView.dequeueReusableCell(for: indexPath) as LoginInfoTableViewCell
                      
                        cell.do {
                            $0.delegate = self
                            $0.highlightedLabel.text = Strings.Login.loginInfoDetailsUsernameFieldTitle
                            $0.descriptionTextField.text = loginEntry.username
                            $0.descriptionTextField.keyboardType = .emailAddress
                            $0.descriptionTextField.returnKeyType = .next
                            $0.isEditingFieldData = isEditingFieldData
                            $0.contentView.alpha = 1.0
                            $0.tag = InfoItem.usernameItem.rawValue
                        }
                        
                        usernameField = cell.descriptionTextField
                        usernameField?.accessibilityIdentifier = "usernameField"
                        
                        return cell
                    case InfoItem.passwordItem.rawValue:
                        let cell = tableView.dequeueReusableCell(for: indexPath) as LoginInfoTableViewCell
                      
                        cell.do {
                            $0.delegate = self
                            $0.highlightedLabel.text = Strings.Login.loginInfoDetailsPasswordFieldTitle
                            $0.descriptionTextField.text = loginEntry.password
                            $0.descriptionTextField.returnKeyType = .done
                            $0.displayDescriptionAsPassword = true
                            $0.isEditingFieldData = isEditingFieldData
                            $0.contentView.alpha = 1.0
                            $0.tag = InfoItem.passwordItem.rawValue
                        }
                        
                        passwordField = cell.descriptionTextField
                        passwordField?.accessibilityIdentifier = "passwordField"
                        
                        return cell
                    default:
                        assertionFailure("No cell available for index path: \(indexPath)")
                }
            case Section.createdDate.rawValue:
                let cell = tableView.dequeueReusableCell(for: indexPath) as CenteredButtonCell
            
                cell.do {
                    $0.textLabel?.text = String(format: Strings.Login.loginInfoCreatedHeaderTitle, formattedCreationDate)
                    $0.tintColor = .secondaryBraveLabel
                    $0.selectionStyle = .none
                    $0.backgroundColor = .secondaryBraveBackground
                }
                return cell
            case Section.delete.rawValue:
                let cell = tableView.dequeueReusableCell(for: indexPath) as CenteredButtonCell
                cell.do {
                    $0.textLabel?.text = Strings.delete
                    $0.tintColor = .braveOrange
                }
                return cell
            default:
                assertionFailure("No cell available for index path: \(indexPath)")
        }
        
        return UITableViewCell()
    }
    
    override func numberOfSections(in tableView: UITableView) -> Int {
        return Section.allCases.count
    }

    override func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
        switch section {
            case Section.information.rawValue:
                return InfoItem.allCases.count
            default:
                return 1
        }
    }

    override func tableView(_ tableView: UITableView, heightForRowAt indexPath: IndexPath) -> CGFloat {
        switch indexPath.section {
            case Section.information.rawValue:
                return UX.informationRowHeight
            case Section.createdDate.rawValue:
                return UX.createdRowHeight
            case Section.delete.rawValue:
                return UX.standardItemHeight
            default:
                return UITableView.automaticDimension
        }
    }

    override func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
        switch indexPath.section {
            case Section.delete.rawValue:
                deleteLogin()
            default:
                if !isEditingFieldData {
                    showActionMenu(for: indexPath)
                }
        }
        
        tableView.deselectRow(at: indexPath, animated: true)
    }
}

// MARK: Actions

extension LoginInfoViewController {
    
    @objc private func edit() {
        isEditingFieldData = true
        
        cellForItem(InfoItem.usernameItem)?.descriptionTextField.becomeFirstResponder()

        navigationItem.rightBarButtonItem =
            UIBarButtonItem(barButtonSystemItem: .done, target: self, action: #selector(done))
    }
    
    private func showActionMenu(for indexPath: IndexPath) {
        if indexPath.section == Section.createdDate.rawValue || indexPath.section == Section.delete.rawValue {
            return
        }

        guard let cell = tableView.cellForRow(at: indexPath) as? LoginInfoTableViewCell else {
            return
        }
        cell.becomeFirstResponder()
        
        UIMenuController.shared.showMenu(from: tableView, rect: cell.frame)
    }
    
    @objc private func done() {
        updateLoginInfo { [weak self] in
            guard let self = self else { return }

            self.isEditingFieldData = false
            self.tableView.reloadData()
            self.navigationItem.rightBarButtonItem =
                UIBarButtonItem(barButtonSystemItem: .edit, target: self, action: #selector(self.edit))
        }
    }
    
    private func updateLoginInfo(completion: @escaping () -> Void) {
        guard let username = usernameField?.text, let password = passwordField?.text,
              username != loginEntry.username || password != loginEntry.password else {
            completion()
            return
        }
                
        loginEntry.update(password: password, username: username)
        
        profile.logins.updateLoginByGUID(loginEntry.guid, new: loginEntry, significant: true).upon { result in
            DispatchQueue.main.async {
                if !result.isSuccess {
                    log.error("Error while updating a login entry. Error Reason: \(result.failureValue ?? "")")
                }
                completion()
            }
        }
    }
    
    private func deleteLogin() {
        let alert = UIAlertController(
            title: Strings.deleteLoginAlertTitle,
            message: Strings.Login.loginEntryDeleteAlertMessage,
            preferredStyle: .alert)
        
        alert.addAction(UIAlertAction(title: Strings.deleteLoginButtonTitle, style: .destructive, handler: { [unowned self] _ in
            self.profile.logins.removeLoginByGUID(self.loginEntry.guid).upon { result in
                DispatchQueue.main.async {
                    if result.isSuccess {
                        self.navigationController?.popViewController(animated: true)
                    } else {
                        log.error("Error while deleting a login entry")
                    }
                }
            }
        }))
        
        alert.addAction(UIAlertAction(title: Strings.cancelButtonTitle, style: .cancel, handler: nil))
        present(alert, animated: true, completion: nil)
    }
    
    private func cellForItem(_ infoItem: InfoItem) -> LoginInfoTableViewCell? {
        guard let cell = tableView.cellForRow(at: IndexPath(row: infoItem.rawValue, section: 0)) as? LoginInfoTableViewCell else {
            return nil
        }
    
        return cell
    }
}

// MARK: LoginInfoTableViewCellDelegate

extension LoginInfoViewController: LoginInfoTableViewCellDelegate {
    func shouldReturnAfterEditingTextField(_ cell: LoginInfoTableViewCell) -> Bool {
        switch cell.tag {
            case InfoItem.usernameItem.rawValue:
                cellForItem(InfoItem.passwordItem)?.descriptionTextField.becomeFirstResponder()
                return false
            case InfoItem.passwordItem.rawValue:
                done()
                return true
            default:
                return true
        }
    }
    
    func canPerform(action: Selector, for cell: LoginInfoTableViewCell) -> Bool {
        switch cell.tag {
            case InfoItem.websiteItem.rawValue:
                return action == MenuHelper.selectorCopy || action == MenuHelper.selectorOpenAndFill
        case InfoItem.usernameItem.rawValue:
            return action == MenuHelper.selectorCopy
        case InfoItem.passwordItem.rawValue:
            let hideRevealPasswordOption = cell.descriptionTextField.isSecureTextEntry
                ? (action == MenuHelper.selectorReveal)
                : (action == MenuHelper.selectorHide)
            return action == MenuHelper.selectorCopy || hideRevealPasswordOption
        default:
            return false
        }
    }
    
    func didSelectOpenAndFill(_ cell: LoginInfoTableViewCell) {
        guard let url = (loginEntry.formSubmitURL?.asURL ?? loginEntry.hostname.asURL) else {
            return
        }
    
        settingsDelegate?.settingsOpenURLInNewTab(url)
        dismiss(animated: true, completion: nil)
    }
    
    func didSelectReveal(_ cell: LoginInfoTableViewCell) -> Bool {
        askForAuthentication()
    }
    
    func textFieldDidEndEditing(_ cell: LoginInfoTableViewCell) {
        if cell.tag == InfoItem.passwordItem.rawValue {
            cell.displayDescriptionAsPassword = true
        }
    }
}

