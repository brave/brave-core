// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import Shared
import BraveShared
import Storage
import Data

private let log = Logger.browserLogger

class LoginListViewController: LoginAuthViewController {
    
    // MARK: UX
    
    struct UX {
        static let headerHeight: CGFloat = 44
    }
    
    // MARK: Constants
    
    struct Constants {
        static let saveLoginsRowIdentifier = "saveLoginsRowIdentifier"
    }
    
    // MARK: Section
    
    enum Section: Int, CaseIterable {
        case options
        case savedLogins
    }
    
    weak var settingsDelegate: SettingsDelegate?
    
    // MARK: Private
    
    private let profile: Profile
    private var loginEntries = [Login]()
    private var isFetchingLoginEntries = false
    private var searchLoginTimer: Timer?

    private let searchController = UISearchController(searchResultsController: nil)
    private let emptyLoginView = LoginEmptyView()
    
    // MARK: Lifecycle
    
    init(profile: Profile) {
        self.profile = profile
        super.init(requiresAuthentication: true)
    }
    
    required init?(coder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }
    
    override func viewDidLoad() {
        super.viewDidLoad()

        applyTheme()
        
        // Insert Done button if being presented outside of the Settings Navigation stack
        if navigationController?.viewControllers.first === self {
            navigationItem.leftBarButtonItem =
                UIBarButtonItem(title: Strings.settingsSearchDoneButton, style: .done, target: self, action: #selector(dismissAnimated))
        }

        navigationItem.do {
            $0.searchController = searchController
            $0.hidesSearchBarWhenScrolling = false
            $0.rightBarButtonItem = editButtonItem
            $0.rightBarButtonItem?.isEnabled = !self.loginEntries.isEmpty
        }
        definesPresentationContext = true

        tableView.tableFooterView = SettingsTableSectionHeaderFooterView(
            frame: CGRect(width: tableView.bounds.width, height: UX.headerHeight))
    }
    
    override func viewWillAppear(_ animated: Bool) {
        super.viewWillAppear(animated)
        
        fetchLoginInfo()
    }
    
    override func viewWillDisappear(_ animated: Bool) {
        super.viewDidDisappear(animated)
        
        tableView.endEditing(true)
    }
    
    // MARK: Internal
    
    private func applyTheme() {
        navigationItem.title = Strings.Login.loginListNavigationTitle

        tableView.do {
            $0.accessibilityIdentifier = Strings.Login.loginListNavigationTitle
            $0.allowsSelectionDuringEditing = true
            $0.registerHeaderFooter(SettingsTableSectionHeaderFooterView.self)
            $0.register(UITableViewCell.self, forCellReuseIdentifier: Constants.saveLoginsRowIdentifier)
            $0.register(TwoLineTableViewCell.self)
            #if swift(>=5.5)
            if #available(iOS 15.0, *) {
                $0.sectionHeaderTopPadding = 0
            }
            #endif
        }
        
        searchController.do {
            $0.searchBar.autocapitalizationType = .none
            $0.searchResultsUpdater = self
            $0.obscuresBackgroundDuringPresentation = false
            $0.searchBar.placeholder = Strings.Login.loginListSearchBarPlaceHolderTitle
            $0.delegate = self
            $0.hidesNavigationBarDuringPresentation = true
        }
        
        navigationController?.view.backgroundColor = .secondaryBraveBackground
    }
    
    private func fetchLoginInfo(_ searchQuery: String? = nil) {
        guard !isFetchingLoginEntries else {
            return
        }
        
        isFetchingLoginEntries = true
        
        if let query = searchQuery {
            profile.logins.getLoginsForQuery(query) >>== { [weak self] results in
                self?.reloadEntries(results: results)
            }
        } else {
            profile.logins.getAllLogins() >>== { [weak self] results in
                self?.reloadEntries(results: results)
            }
        }
    }
    
    private func reloadEntries(results: Cursor<Login>) {
        loginEntries = results.asArray()
        DispatchQueue.main.async {
            self.tableView.reloadData()
            self.isFetchingLoginEntries = false
            self.navigationItem.rightBarButtonItem?.isEnabled = !self.loginEntries.isEmpty
        }
    }
}

// MARK: TableViewDataSource - TableViewDelegate

extension LoginListViewController {
    
    override func numberOfSections(in tableView: UITableView) -> Int {
        tableView.backgroundView = loginEntries.isEmpty ? emptyLoginView : nil

        return Section.allCases.count
    }
    
    override func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
        if section == Section.options.rawValue {
            return 1
        } else {
            return loginEntries.count
        }
    }
    
    override func tableView(_ tableView: UITableView, heightForRowAt indexPath: IndexPath) -> CGFloat {
        if indexPath.section == Section.options.rawValue, searchController.isActive || tableView.isEditing {
            return 0
        }
        return UITableView.automaticDimension
    }
    
    override func tableView(_ tableView: UITableView, heightForHeaderInSection section: Int) -> CGFloat {
        if section == Section.savedLogins.rawValue {
            return loginEntries.isEmpty ? .zero : UX.headerHeight
        }
        
        return .zero
    }
    
    override func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
        if indexPath.section == Section.options.rawValue {
            let toggle = UISwitch().then {
                $0.addTarget(self, action: #selector(didToggleSaveLogins), for: .valueChanged)
                $0.isOn = Preferences.General.saveLogins.value
            }
            
            let cell = tableView.dequeueReusableCell(withIdentifier: Constants.saveLoginsRowIdentifier, for: indexPath).then {
                $0.textLabel?.text = Strings.saveLogins
                $0.separatorInset = .zero
                $0.accessoryView = toggle
                $0.selectionStyle = .none
            }
            
            return cell
        } else {
            guard let loginInfo = loginEntries[safe: indexPath.item] else {
                return UITableViewCell()
            }
            
            let cell = tableView.dequeueReusableCell(for: indexPath) as TwoLineTableViewCell
            
            cell.do {
                $0.selectionStyle = .none
                $0.accessoryType = .disclosureIndicator

                $0.setLines(loginInfo.hostname, detailText: loginInfo.username)
                $0.imageView?.contentMode = .scaleAspectFit
                $0.imageView?.image = FaviconFetcher.defaultFaviconImage
                $0.imageView?.layer.borderColor = BraveUX.faviconBorderColor.cgColor
                $0.imageView?.layer.borderWidth = BraveUX.faviconBorderWidth
                $0.imageView?.layer.cornerRadius = 6
                $0.imageView?.layer.cornerCurve = .continuous
                $0.imageView?.layer.masksToBounds = true
            }
                      
            if let loginHostnameURL = URL(string: loginInfo.hostname) {
                let domain = Domain.getOrCreate(forUrl: loginHostnameURL, persistent: true)
                
                cell.imageView?.loadFavicon(
                    for: loginHostnameURL,
                    domain: domain,
                    fallbackMonogramCharacter: loginHostnameURL.baseDomain?.first,
                    shouldClearMonogramFavIcon: false,
                    cachedOnly: true)
            } else {
                cell.imageView?.clearMonogramFavicon()
                cell.imageView?.image = FaviconFetcher.defaultFaviconImage
            }
            
            return cell
        }
    }
    
    override func tableView(_ tableView: UITableView, viewForHeaderInSection section: Int) -> UIView? {
        let headerView = tableView.dequeueReusableHeaderFooter() as SettingsTableSectionHeaderFooterView
        headerView.titleLabel.text = Strings.Login.loginListSavedLoginsHeaderTitle.uppercased()
        
        return headerView
    }

    override func tableView(_ tableView: UITableView, willSelectRowAt indexPath: IndexPath) -> IndexPath? {
        if indexPath.section == Section.savedLogins.rawValue, let loginEntry = loginEntries[safe: indexPath.row] {
            let loginDetailsViewController = LoginInfoViewController(profile: profile, loginEntry: loginEntry)
            loginDetailsViewController.settingsDelegate = settingsDelegate
            navigationController?.pushViewController(loginDetailsViewController, animated: true)
            
            return indexPath
        }
        
        return nil
    }
    
    override func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
        if indexPath.section == Section.savedLogins.rawValue {
            fetchLoginInfo()
            searchController.isActive = false
        }
    }

    // Determine whether to show delete button in edit mode
    override func tableView(_ tableView: UITableView, editingStyleForRowAt indexPath: IndexPath) -> UITableViewCell.EditingStyle {
        guard indexPath.section == Section.savedLogins.rawValue else {
            return .none
        }
        
        return .delete
    }

    // Determine whether to indent while in edit mode for deletion
    override func tableView(_ tableView: UITableView, shouldIndentWhileEditingRowAt indexPath: IndexPath) -> Bool {
        return indexPath.section == Section.savedLogins.rawValue
    }

    override func tableView(_ tableView: UITableView, commit editingStyle: UITableViewCell.EditingStyle, forRowAt indexPath: IndexPath) {
        if editingStyle == .delete {
            guard let loginItem = loginEntries[safe: indexPath.row] else { return }
            
            showDeleteLoginWarning(with: loginItem)
        }
    }
    
    override func tableView(_ tableView: UITableView, canEditRowAt indexPath: IndexPath) -> Bool {
        return indexPath.section == Section.savedLogins.rawValue
    }
    
    private func showDeleteLoginWarning(with loginItem: Login) {
        let alert = UIAlertController(
            title: Strings.deleteLoginAlertTitle,
            message: Strings.Login.loginEntryDeleteAlertMessage,
            preferredStyle: .alert)
        
        alert.addAction(UIAlertAction(title: Strings.deleteLoginButtonTitle, style: .destructive, handler: { [weak self] _ in
            let success = self?.profile.logins.removeLoginByGUID(loginItem.guid)
            
            success?.upon { result in
                if result.isSuccess {
                    self?.fetchLoginInfo()
                } else {
                    log.error("Error while deleting a login entry")
                }
            }
        }))
        
        alert.addAction(UIAlertAction(title: Strings.cancelButtonTitle, style: .cancel, handler: nil))
        present(alert, animated: true, completion: nil)
    }
}

// MARK: - Actions

extension LoginListViewController {
    
    @objc func didToggleSaveLogins(_ toggle: UISwitch) {
        Preferences.General.saveLogins.value = toggle.isOn
    }

    @objc func dismissAnimated() {
        self.dismiss(animated: true, completion: nil)
    }
}

// MARK: UISearchResultUpdating

extension LoginListViewController: UISearchResultsUpdating {
    
    func updateSearchResults(for searchController: UISearchController) {
        guard let query = searchController.searchBar.text else { return }

        if searchLoginTimer != nil {
            searchLoginTimer?.invalidate()
            searchLoginTimer = nil
        }
        
        searchLoginTimer =
            Timer.scheduledTimer(timeInterval: 0.1, target: self, selector: #selector(fetchSearchResults(timer:)), userInfo: query, repeats: false)
    }
    
    @objc private func fetchSearchResults(timer: Timer) {
        guard let query = timer.userInfo as? String else {
            return
        }
        
        fetchLoginInfo(query)
    }
}

// MARK: UISearchControllerDelegate

extension LoginListViewController: UISearchControllerDelegate {
    
    func willPresentSearchController(_ searchController: UISearchController) {
        tableView.setEditing(false, animated: true)
        tableView.reloadData()
    }
    
    func willDismissSearchController(_ searchController: UISearchController) {
        tableView.reloadData()
    }
}
