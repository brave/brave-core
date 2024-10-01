// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveUI
import Data
import Favicon
import Foundation
import Fuzi
import Shared
import SnapKit
import Static
import Storage
import WebKit
import os.log

class CustomEngineViewController: UIViewController {

  // MARK: AddButtonType

  enum AddButtonType {
    case enabled
    case disabled
    case loading
  }

  // MARK: Section

  private enum Section: Int, CaseIterable {
    case url
    case title
  }

  // MARK: Constants

  struct Constants {
    static let titleEntryMaxCharacterCount = 50
  }

  // MARK: CustomEngineType

  enum CustomEngineActionType {
    case add
    case edit
  }

  // MARK: Properties

  var profile: Profile
  var privateBrowsingManager: PrivateBrowsingManager

  private var urlText: String?
  private var titleText: String?

  var host: URL? {
    didSet {
      if let host = host, oldValue != host, customEngineActionType == .add {
        fetchSearchEngineSupportForHost(host)
      } else {
        checkManualAddExists()
      }
    }
  }

  var openSearchReference: OpenSearchReference? {
    didSet {
      checkSupportAutoAddSearchEngine()
    }
  }

  private var engineToBeEdited: OpenSearchEngine?
  var customEngineActionType: CustomEngineActionType = .add

  var isAutoAddEnabled = false

  var dataTask: URLSessionDataTask? {
    didSet {
      oldValue?.cancel()
    }
  }

  var faviconTask: Task<Void, Error>?
  var faviconImage: UIImage?

  private lazy var spinnerView = UIActivityIndicatorView(style: .medium).then {
    $0.hidesWhenStopped = true
  }

  private var tableView = UITableView(frame: .zero, style: .grouped)

  private var searchEngineTimer: Timer?

  // MARK: Lifecycle

  init(
    profile: Profile,
    privateBrowsingManager: PrivateBrowsingManager,
    engineToBeEdited: OpenSearchEngine? = nil
  ) {
    self.profile = profile
    self.privateBrowsingManager = privateBrowsingManager

    self.engineToBeEdited = engineToBeEdited
    if engineToBeEdited != nil {
      self.customEngineActionType = .edit
    }

    super.init(nibName: nil, bundle: nil)
  }

  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  override func viewDidLoad() {
    super.viewDidLoad()

    title = Strings.searchSettingAddCustomEngineCellTitle

    setup()
    doLayout()

    changeAddEditButton(for: customEngineActionType == .add ? .enabled : .disabled)
  }

  // MARK: Internal

  private func setup() {
    tableView.do {
      $0.register(CustomEngineURLInputTableViewCell.self)
      $0.register(CustomEngineTitleInputTableViewCell.self)
      $0.registerHeaderFooter(SearchEngineTableViewHeader.self)
      $0.dataSource = self
      $0.delegate = self
    }
  }

  private func doLayout() {
    view.addSubview(tableView)

    tableView.snp.makeConstraints { make in
      make.edges.equalTo(self.view)
    }
  }

  func changeAddEditButton(for type: AddButtonType) {
    ensureMainThread {
      switch type {
      case .enabled:
        self.navigationItem.rightBarButtonItem = UIBarButtonItem(
          barButtonSystemItem: self.customEngineActionType == .add ? .done : .edit,
          target: self,
          action: #selector(self.checkAddEngineType)
        )
        self.spinnerView.stopAnimating()
      case .disabled:
        self.navigationItem.rightBarButtonItem = UIBarButtonItem(
          barButtonSystemItem: self.customEngineActionType == .add ? .done : .edit,
          target: self,
          action: #selector(self.checkAddEngineType)
        )
        self.navigationItem.rightBarButtonItem?.isEnabled = false
        self.spinnerView.stopAnimating()
        self.isAutoAddEnabled = false
      case .loading:
        self.navigationItem.rightBarButtonItem = UIBarButtonItem(customView: self.spinnerView)
        self.spinnerView.startAnimating()
      }
    }
  }

  func handleError(error: Error) {
    let alert: UIAlertController

    if let searchError = error as? SearchEngineError {
      switch searchError {
      case .duplicate:
        alert = ThirdPartySearchAlerts.duplicateCustomEngine()
      case .invalidQuery:
        alert = ThirdPartySearchAlerts.incorrectCustomEngineForm()
      case .failedToSave:
        alert = ThirdPartySearchAlerts.failedToAddThirdPartySearch()
      case .missingInformation:
        alert = ThirdPartySearchAlerts.missingInfoToAddThirdPartySearch()
      case .invalidURL:
        alert = ThirdPartySearchAlerts.insecureURLEntryThirdPartySearch()
      }
    } else {
      alert = ThirdPartySearchAlerts.failedToAddThirdPartySearch()
    }

    Logger.module.error("\(error.localizedDescription)")
    present(alert, animated: true, completion: nil)
  }

  // MARK: Actions

  @objc func checkAddEngineType() {
    if isAutoAddEnabled {
      addOpenSearchEngine()
    } else {
      addCustomSearchEngine()
    }
  }

  func addCustomSearchEngine() {
    view.endEditing(true)

    guard let title = titleText,
      let urlQuery = urlText,
      !title.isEmpty,
      !urlQuery.isEmpty
    else {
      present(
        ThirdPartySearchAlerts.missingInfoToAddThirdPartySearch(),
        animated: true,
        completion: nil
      )
      return
    }

    changeAddEditButton(for: .disabled)
    addSearchEngine(with: urlQuery, title: title)
  }

  @objc func cancel() {
    navigationController?.popViewController(animated: true)
  }
}

// MARK: - UITableViewDelegate UITableViewDataSource

extension CustomEngineViewController: UITableViewDelegate, UITableViewDataSource {

  func numberOfSections(in tableView: UITableView) -> Int {
    return Section.allCases.count
  }

  func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
    return 1
  }

  func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
    switch indexPath.section {
    case Section.url.rawValue:
      let cell = tableView.dequeueReusableCell(for: indexPath) as CustomEngineURLInputTableViewCell
      cell.do {
        $0.delegate = self
        $0.selectionStyle = .none
      }

      if let engineToBeEdited = engineToBeEdited {
        let searchQuery = getSearchQuery(from: engineToBeEdited.searchTemplate)
        cell.textview.text = searchQuery
        urlText = searchQuery
      }
      return cell
    default:
      let cell =
        tableView.dequeueReusableCell(for: indexPath) as CustomEngineTitleInputTableViewCell
      cell.do {
        $0.delegate = self
        $0.selectionStyle = .none
      }

      if let engineToBeEdited = engineToBeEdited {
        let engineName = engineToBeEdited.shortName
        cell.textfield.text = engineName
        titleText = engineName
      }
      return cell
    }
  }

  func tableView(_ tableView: UITableView, titleForFooterInSection section: Int) -> String? {
    guard section == Section.url.rawValue else { return nil }

    return Strings.CustomSearchEngine.customEngineAddDesription
  }

  func tableView(_ tableView: UITableView, viewForHeaderInSection section: Int) -> UIView? {
    let headerView = tableView.dequeueReusableHeaderFooter() as SearchEngineTableViewHeader

    switch section {
    case Section.url.rawValue:
      headerView.titleLabel.text = Strings.URL.uppercased()
    default:
      headerView.titleLabel.text = Strings.title.uppercased()
    }

    return headerView
  }
}

// MARK: Manual Add Engine

extension CustomEngineViewController {

  fileprivate func addSearchEngine(with urlQuery: String, title: String) {
    changeAddEditButton(for: .loading)

    let safeURLQuery = urlQuery.trimmingCharacters(in: .whitespacesAndNewlines)
    let safeTitle = title.trimmingCharacters(in: .whitespacesAndNewlines)

    createSearchEngine(using: safeURLQuery, name: safeTitle) { [weak self] engine, error in
      guard let self = self else { return }

      if let error = error {
        self.handleError(error: error)

        self.changeAddEditButton(for: .disabled)
      } else if let engine = engine {
        Task { @MainActor in
          do {
            switch self.customEngineActionType {
            case .add:
              try await self.profile.searchEngines.addSearchEngine(engine)
            case .edit:
              if let engineToBeEdited = self.engineToBeEdited {
                try await self.profile.searchEngines.editSearchEngine(
                  engineToBeEdited,
                  with: engine
                )
              }
            }
            self.cancel()
          } catch {
            self.handleError(error: SearchEngineError.failedToSave)
            self.changeAddEditButton(for: .enabled)
          }
        }
      }

    }
  }

  private func createSearchEngine(
    using query: String,
    name: String,
    completion: @escaping ((OpenSearchEngine?, SearchEngineError?) -> Void)
  ) {
    // Ensure the host is setup even if the user does not edit it
    setupHost()

    // Check Search Query is not valid
    guard let template = getSearchTemplate(with: query),
      let urlText = template.addingPercentEncoding(withAllowedCharacters: .urlFragmentAllowed),
      let url = URL(string: urlText),
      url.isWebPage()
    else {
      completion(nil, SearchEngineError.invalidQuery)
      return
    }

    // Check Engine Exists only for add mode
    if customEngineActionType == .add {
      guard
        profile.searchEngines.orderedEngines.filter({
          $0.shortName == name || $0.searchTemplate.contains(template)
        }).isEmpty
      else {
        completion(nil, SearchEngineError.duplicate)
        return
      }
    }

    var engineImage = Favicon.defaultImage

    guard let hostUrl = host else {
      let engine = OpenSearchEngine(
        shortName: name,
        image: engineImage,
        searchTemplate: template,
        isCustomEngine: true
      )

      completion(engine, nil)
      return
    }

    faviconTask?.cancel()
    faviconTask = Task { @MainActor in
      let favicon = try? await FaviconFetcher.loadIcon(
        url: hostUrl,
        persistent: !privateBrowsingManager.isPrivateBrowsing
      )
      if let image = favicon?.image {
        engineImage = image
      }

      let engine = OpenSearchEngine(
        shortName: name,
        image: engineImage,
        searchTemplate: template,
        isCustomEngine: true
      )
      completion(engine, nil)
    }
  }

  private func getSearchTemplate(with query: String) -> String? {
    let searchTermPlaceholder = "%s"
    let searchTemplatePlaceholder = "{searchTerms}"

    if query.contains(searchTermPlaceholder) {
      return query.replacingOccurrences(of: searchTermPlaceholder, with: searchTemplatePlaceholder)
    }

    return nil
  }

  private func getSearchQuery(from template: String) -> String? {
    let searchTermPlaceholder = "%s"
    let searchTemplatePlaceholder = "{searchTerms}"

    if template.contains(searchTemplatePlaceholder) {
      return template.replacingOccurrences(
        of: searchTemplatePlaceholder,
        with: searchTermPlaceholder
      )
    }

    return nil
  }

  func checkManualAddExists() {
    guard let url = urlText, let title = titleText else {
      return
    }

    if !url.isEmpty, !title.isEmpty {
      changeAddEditButton(for: .enabled)
    } else {
      changeAddEditButton(for: .disabled)
    }
  }
}

// MARK: - UITextViewDelegate

extension CustomEngineViewController: UITextViewDelegate {

  func textView(
    _ textView: UITextView,
    shouldChangeTextIn range: NSRange,
    replacementText text: String
  ) -> Bool {
    guard text.rangeOfCharacter(from: .newlines) == nil else {
      textView.resignFirstResponder()
      return false
    }

    let textLengthInRange = textView.text.count + (text.count - range.length)

    // The default text "https://" cant be deleted or changed so nothing without a secure scheme can be added
    return textLengthInRange >= 8
  }

  func textViewDidChange(_ textView: UITextView) {
    changeAddEditButton(for: .disabled)

    // The withSecureUrlScheme is used in order to force user to use secure url scheme
    // Instead of checking paste-board with every character entry, the textView text is analyzed
    // and according to what prefix copied or entered, text is altered to start with https://
    // this logic block repeating https:// and http:// schemes
    let textEntered = textView.text.withSecureUrlScheme

    textView.text = textEntered
    urlText = textEntered

    if searchEngineTimer != nil {
      searchEngineTimer?.invalidate()
      searchEngineTimer = nil
    }

    // Reschedule the search engine fetch: in 0.25 second, call the setupHost method on the new textview content
    searchEngineTimer =
      Timer.scheduledTimer(
        timeInterval: 0.25,
        target: self,
        selector: #selector(setupHost),
        userInfo: nil,
        repeats: false
      )
  }

  func textViewDidEndEditing(_ textView: UITextView) {
    urlText = textView.text
  }

  @objc
  private func setupHost() {
    if let text = urlText?.trimmingCharacters(in: .whitespacesAndNewlines),
      let encodedText = text.addingPercentEncoding(withAllowedCharacters: .urlFragmentAllowed),
      let url = URL(string: encodedText),
      url.host != nil,
      url.isWebPage()
    {
      if let scheme = url.scheme, let host = url.host {
        self.host = URL(string: "\(scheme)://\(host)")
      }
    }
  }
}

// MARK: - UITextFieldDelegate

extension CustomEngineViewController: UITextFieldDelegate {

  func textField(
    _ textField: UITextField,
    shouldChangeCharactersIn range: NSRange,
    replacementString string: String
  ) -> Bool {
    guard let text = textField.text, text.rangeOfCharacter(from: .newlines) == nil else {
      return false
    }

    let currentString = text as NSString
    let newString: NSString = currentString.replacingCharacters(in: range, with: string) as NSString

    let shouldChangeCharacters = newString.length <= Constants.titleEntryMaxCharacterCount

    if shouldChangeCharacters {
      titleText = newString as String
      checkManualAddExists()
    }

    return shouldChangeCharacters
  }

  func textFieldDidEndEditing(_ textField: UITextField) {
    titleText = textField.text
  }

  func textFieldShouldReturn(_ textField: UITextField) -> Bool {
    textField.endEditing(true)

    return true
  }
}
