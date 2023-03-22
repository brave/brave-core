// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import Shared
import BraveShared
import Storage
import Data
import BraveCore
import Favicon

class LoginListViewController: LoginAuthViewController {

  // MARK: UX

  private struct UX {
    static let headerHeight: CGFloat = 44
  }

  // MARK: Constants

  private struct Constants {
    static let saveLoginsRowIdentifier = "saveLoginsRowIdentifier"
  }

  weak var settingsDelegate: SettingsDelegate?

  // MARK: Private

  private let passwordAPI: BravePasswordAPI
  private let dataSource: LoginListDataSource
  private let windowProtection: WindowProtection?

  private var passwordStoreListener: PasswordStoreListener?
  private var searchLoginTimer: Timer?
  private let searchController = UISearchController(searchResultsController: nil)
  private let emptyStateOverlayView = EmptyStateOverlayView(
    overlayDetails: EmptyOverlayStateDetails(title: Strings.Login.loginListEmptyScreenTitle))

  // MARK: Lifecycle

  init(passwordAPI: BravePasswordAPI, windowProtection: WindowProtection?) {
    self.windowProtection = windowProtection
    self.passwordAPI = passwordAPI
    dataSource = LoginListDataSource(with: passwordAPI)
    
    super.init(windowProtection: windowProtection, requiresAuthentication: true)

    // Adding the Password store observer in constructor to watch credentials changes
    passwordStoreListener = passwordAPI.add(
      PasswordStoreStateObserver { [weak self] _ in
        guard let self = self, !self.dataSource.isCredentialsBeingSearched else {
          return
        }

        DispatchQueue.main.async {
          self.fetchLoginInfo()
        }
      })
  }

  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  deinit {
    // Remove the password store observer
    if let observer = passwordStoreListener {
      passwordAPI.removeObserver(observer)
    }
  }

  override func viewDidLoad() {
    super.viewDidLoad()

    applyTheme()

    // Insert Done button if being presented outside of the Settings Navigation stack
    if navigationController?.viewControllers.first === self {
      navigationItem.leftBarButtonItem = UIBarButtonItem(
        title: Strings.settingsSearchDoneButton,
        style: .done,
        target: self,
        action: #selector(dismissAnimated))
    }

    navigationItem.do {
      $0.searchController = searchController
      $0.hidesSearchBarWhenScrolling = false
      $0.rightBarButtonItem = editButtonItem
      $0.rightBarButtonItem?.isEnabled = !self.dataSource.credentialList.isEmpty
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
      $0.sectionHeaderTopPadding = 0
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
}

// MARK: TableViewDataSource - TableViewDelegate

extension LoginListViewController {

  override func numberOfSections(in tableView: UITableView) -> Int {
    tableView.backgroundView = dataSource.isDataSourceEmpty ? emptyStateOverlayView : nil
    return dataSource.fetchNumberOfSections()
  }

  override func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
    return dataSource.fetchNumberOfRowsInSection(section: section)
  }

  override func tableView(_ tableView: UITableView, heightForRowAt indexPath: IndexPath) -> CGFloat {
    return UITableView.automaticDimension
  }

  override func tableView(_ tableView: UITableView, heightForHeaderInSection section: Int) -> CGFloat {
    // Save Login Toggle
    if section == 0, !dataSource.isCredentialsBeingSearched {
      return 0
    }
    
    return UX.headerHeight
  }

  override func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
    func createSaveToggleCell() -> UITableViewCell {
      let toggle = UISwitch().then {
        $0.addTarget(self, action: #selector(didToggleSaveLogins), for: .valueChanged)
        $0.isOn = Preferences.General.saveLogins.value
      }
      
      let cell = tableView.dequeueReusableCell(withIdentifier: Constants.saveLoginsRowIdentifier, for: indexPath).then {
        $0.textLabel?.text = Strings.saveLogins
        $0.separatorInset = .zero
        $0.accessoryView = searchController.isActive ? nil : toggle
        $0.selectionStyle = .none
      }
      
      return cell
    }
    
    func createCredentialFormCell(passwordForm: PasswordForm?) -> TwoLineTableViewCell {
      guard let loginInfo = passwordForm else {
        return TwoLineTableViewCell()
      }
      
      let cell = tableView.dequeueReusableCell(for: indexPath) as TwoLineTableViewCell

      cell.do {
        $0.selectionStyle = .none
        $0.accessoryType = .disclosureIndicator

        $0.setLines(
          loginInfo.displayURLString,
          detailText: loginInfo.usernameValue)
        $0.imageView?.contentMode = .scaleAspectFit
        $0.imageView?.image = Favicon.defaultImage
        $0.imageView?.layer.borderColor = BraveUX.faviconBorderColor.cgColor
        $0.imageView?.layer.borderWidth = BraveUX.faviconBorderWidth
        $0.imageView?.layer.cornerRadius = 6
        $0.imageView?.layer.cornerCurve = .continuous
        $0.imageView?.layer.masksToBounds = true

        if let signOnRealmURL = URL(string: loginInfo.signOnRealm) {
          $0.imageView?.loadFavicon(for: signOnRealmURL,
                                    monogramFallbackCharacter: signOnRealmURL.baseDomain?.first)
        } else {
          $0.imageView?.clearMonogramFavicon()
          $0.imageView?.image = Favicon.defaultImage
        }
      }
      
      return cell
    }
    
    if let form = dataSource.fetchPasswordFormFor(indexPath: indexPath) {
      return createCredentialFormCell(passwordForm: form)
    }
    
    if !dataSource.isCredentialsBeingSearched, indexPath.section == 0 {
      return createSaveToggleCell()
    }
    
    return UITableViewCell()
  }
  
  override func tableView(_ tableView: UITableView, viewForHeaderInSection section: Int) -> UIView? {
    let headerView = tableView.dequeueReusableHeaderFooter() as SettingsTableSectionHeaderFooterView
    
    let savedLoginHeaderText = Strings.Login.loginListSavedLoginsHeaderTitle.uppercased()
    let neverSavedHeaderText = Strings.Login.loginListNeverSavedHeaderTitle.uppercased()
    
    var titleHeaderText = ""
    
    if dataSource.isCredentialsBeingSearched {
      switch section {
      case 0:
        titleHeaderText = dataSource.credentialList.isEmpty ? neverSavedHeaderText : savedLoginHeaderText
      case 1:
        titleHeaderText = dataSource.blockedList.isEmpty ? "" : neverSavedHeaderText
      default:
        titleHeaderText = ""
      }
    } else {
      switch section {
      case 1:
        titleHeaderText = dataSource.credentialList.isEmpty ? neverSavedHeaderText : savedLoginHeaderText
      case 2:
        titleHeaderText = dataSource.blockedList.isEmpty ? "" : neverSavedHeaderText
      default:
        titleHeaderText = ""
      }
    }
    
    headerView.titleLabel.text = titleHeaderText
    
    return headerView
  }

  override func tableView(_ tableView: UITableView, willSelectRowAt indexPath: IndexPath) -> IndexPath? {
    func showInformationController(for form: PasswordForm) {
      let loginDetailsViewController = LoginInfoViewController(
        passwordAPI: passwordAPI,
        credentials: form,
        windowProtection: windowProtection)
      loginDetailsViewController.settingsDelegate = settingsDelegate
      navigationController?.pushViewController(loginDetailsViewController, animated: true)
    }
    
    if tableView.isEditing {
      return nil
    }
    
    if let form = dataSource.fetchPasswordFormFor(indexPath: indexPath) {
      showInformationController(for: form)
      return indexPath
    }

    return nil
  }

  override func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
    if indexPath.section == 0, !dataSource.isCredentialsBeingSearched {
     return
    }
    
    searchController.isActive = false
      
    tableView.isEditing = false
    setEditing(false, animated: false)
      
    fetchLoginInfo()
  }

  // Determine whether to show delete button in edit mode
  override func tableView(_ tableView: UITableView, editingStyleForRowAt indexPath: IndexPath) -> UITableViewCell.EditingStyle {
    if indexPath.section == 0, !dataSource.isCredentialsBeingSearched {
      return .none
    }

    return .delete
  }

  // Determine whether to indent while in edit mode for deletion
  override func tableView(_ tableView: UITableView, shouldIndentWhileEditingRowAt indexPath: IndexPath) -> Bool {
    return !(indexPath.section == 0 && !dataSource.isCredentialsBeingSearched)
  }

  override func tableView(_ tableView: UITableView, commit editingStyle: UITableViewCell.EditingStyle, forRowAt indexPath: IndexPath) {
    if editingStyle == .delete {
      if let form = dataSource.fetchPasswordFormFor(indexPath: indexPath) {
        showDeleteLoginWarning(with: form)
      }
    }
  }

  override func tableView(_ tableView: UITableView, canEditRowAt indexPath: IndexPath) -> Bool {
    return !(indexPath.section == 0 && !dataSource.isCredentialsBeingSearched)
  }

  private func showDeleteLoginWarning(with credential: PasswordForm) {
    let alert = UIAlertController(
      title: Strings.deleteLoginAlertTitle,
      message: Strings.Login.loginEntryDeleteAlertMessage,
      preferredStyle: .alert)

    alert.addAction(
      UIAlertAction(
        title: Strings.deleteLoginButtonTitle, style: .destructive,
        handler: { [weak self] _ in
          guard let self = self else { return }

          self.tableView.isEditing = false
          self.setEditing(false, animated: false)
          self.passwordAPI.removeLogin(credential)
          self.fetchLoginInfo()
        }))

    alert.addAction(UIAlertAction(title: Strings.cancelButtonTitle, style: .cancel, handler: nil))
    present(alert, animated: true, completion: nil)
  }
  
  private func fetchLoginInfo(_ searchQuery: String? = nil) {
    dataSource.fetchLoginInfo(searchQuery) { [weak self] editEnabled in
      self?.tableView.reloadData()
      self?.navigationItem.rightBarButtonItem?.isEnabled = editEnabled
    }
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
    dataSource.isCredentialsBeingSearched = true

    tableView.setEditing(false, animated: true)
    tableView.reloadData()
  }

  func willDismissSearchController(_ searchController: UISearchController) {
    dataSource.isCredentialsBeingSearched = false

    tableView.reloadData()
  }
}
