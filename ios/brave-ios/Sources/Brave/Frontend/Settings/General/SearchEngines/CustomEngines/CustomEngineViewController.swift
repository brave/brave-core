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

  // MARK: DoneButtonStatus

  enum DoneButtonStatus {
    case enabled
    case disabled
    case loading
  }

  // MARK: Section

  private enum Section: Int, CaseIterable {
    case title
    case url
    case button
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

  var profile: LegacyBrowserProfile
  var onAddSucceed: (() -> Void)?

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
  var doneButtonStatus: DoneButtonStatus = .disabled {
    didSet {
      if doneButtonStatus == .disabled {
        isAutoAddEnabled = false
      }
      ensureMainThread {
        self.tableView.reloadRows(
          at: [IndexPath(row: 0, section: Section.button.rawValue)],
          with: .none
        )
      }
    }
  }

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

  private var tableView = UITableView(frame: .zero, style: .insetGrouped)

  private var searchEngineTimer: Timer?

  // MARK: Lifecycle

  init(
    profile: LegacyBrowserProfile,
    engineToBeEdited: OpenSearchEngine? = nil
  ) {
    self.profile = profile

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

    navigationItem.do {
      $0.rightBarButtonItem = UIBarButtonItem(
        barButtonSystemItem: .close,
        target: self,
        action: #selector(cancel)
      )
    }

    tableView.do {
      $0.register(CustomEngineURLInputTableViewCell.self)
      $0.register(CustomEngineTitleInputTableViewCell.self)
      $0.register(CustomEngineAddButtonCell.self)
      $0.registerHeaderFooter(SearchEngineTableViewHeader.self)
      $0.dataSource = self
      $0.delegate = self
    }

    setTheme()
    doLayout()

    doneButtonStatus = customEngineActionType == .add ? .enabled : .disabled
  }

  override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
    super.traitCollectionDidChange(previousTraitCollection)
    setTheme()
  }

  // MARK: Internal

  private func setTheme() {
    view.backgroundColor = UIColor(braveSystemName: .materialThick).withAlphaComponent(0.91)
  }

  private func doLayout() {
    view.addSubview(tableView)

    tableView.snp.makeConstraints {
      $0.edges.equalToSuperview()
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

  @objc func onDone() {
    guard doneButtonStatus != .loading else { return }
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

    doneButtonStatus = .disabled
    addSearchEngine(with: urlQuery, title: title)
  }

  @objc func cancel() {
    dismiss(animated: true)
  }
}

// MARK: - UITableViewDelegate UITableViewDataSource

extension CustomEngineViewController: UITableViewDelegate, UITableViewDataSource {

  func numberOfSections(in tableView: UITableView) -> Int {
    return Section.allCases.count
  }

  func tableView(_ tableView: UITableView, heightForRowAt indexPath: IndexPath) -> CGFloat {
    switch indexPath.section {
    case Section.url.rawValue:
      return 88
    case Section.button.rawValue:
      return 44
    default:
      return UITableView.automaticDimension
    }
  }

  func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
    return 1
  }

  func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
    switch indexPath.section {
    case Section.title.rawValue:
      let cell =
        tableView.dequeueReusableCell(for: indexPath) as CustomEngineTitleInputTableViewCell
      cell.do {
        $0.backgroundColor = UIColor(braveSystemName: .containerBackground)
        $0.delegate = self
        $0.selectionStyle = .none
      }

      if let engineToBeEdited = engineToBeEdited {
        let engineName = engineToBeEdited.shortName
        cell.textfield.text = engineName
        titleText = engineName
      }
      return cell
    case Section.url.rawValue:
      let cell = tableView.dequeueReusableCell(for: indexPath) as CustomEngineURLInputTableViewCell
      cell.do {
        $0.backgroundColor = UIColor(braveSystemName: .containerBackground)
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
      let cell = tableView.dequeueReusableCell(for: indexPath) as CustomEngineAddButtonCell
      cell.doneButton.addTarget(
        self,
        action: #selector(onDone),
        for: .touchUpInside
      )
      cell.updateButtonStatus(doneButtonStatus)
      cell.selectionStyle = .none
      cell.backgroundColor = .clear
      return cell
    }
  }

  func tableView(_ tableView: UITableView, titleForFooterInSection section: Int) -> String? {
    if section == Section.url.rawValue {
      return Strings.CustomSearchEngine.customEngineAddDesription
    }
    return nil
  }

  func tableView(
    _ tableView: UITableView,
    willDisplayFooterView view: UIView,
    forSection section: Int
  ) {
    guard let footerView = view as? UITableViewHeaderFooterView else { return }
    footerView.textLabel?.textColor = UIColor(braveSystemName: .textSecondary)
  }

  func tableView(_ tableView: UITableView, viewForHeaderInSection section: Int) -> UIView? {
    guard section != Section.button.rawValue else { return nil }

    let headerView = tableView.dequeueReusableHeaderFooter() as SearchEngineTableViewHeader
    switch section {
    case Section.title.rawValue:
      headerView.titleLabel.text = Strings.title.uppercased()
    default:
      headerView.titleLabel.text = Strings.URL.uppercased()
    }
    return headerView
  }
}

// MARK: Manual Add Engine

extension CustomEngineViewController {

  fileprivate func addSearchEngine(with urlQuery: String, title: String) {
    doneButtonStatus = .loading

    let safeURLQuery = urlQuery.trimmingCharacters(in: .whitespacesAndNewlines)
    let safeTitle = title.trimmingCharacters(in: .whitespacesAndNewlines)

    createSearchEngine(using: safeURLQuery, name: safeTitle) { [weak self] engine, error in
      guard let self = self else { return }

      if let error = error {
        self.handleError(error: error)

        doneButtonStatus = .disabled
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
            self.onAddSucceed?()
            self.cancel()
          } catch {
            self.handleError(error: SearchEngineError.failedToSave)
            self.doneButtonStatus = .enabled
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
        persistent: false
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
      doneButtonStatus = .enabled
    } else {
      doneButtonStatus = .disabled
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
    doneButtonStatus = .disabled

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
