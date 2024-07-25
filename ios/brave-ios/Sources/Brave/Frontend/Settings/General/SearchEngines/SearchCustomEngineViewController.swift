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

class SearchCustomEngineViewController: UIViewController {

  // MARK: AddButtonType

  private enum AddButtonType {
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

  // MARK: Properties

  private var profile: Profile
  private var privateBrowsingManager: PrivateBrowsingManager

  private var urlText: String?

  private var titleText: String?

  private var host: URL? {
    didSet {
      if let host = host, oldValue != host {
        fetchSearchEngineSupportForHost(host)
      } else {
        checkManualAddExists()
      }
    }
  }

  private var openSearchReference: OpenSearchReference? {
    didSet {
      checkSupportAutoAddSearchEngine()
    }
  }

  private var engineToBeEdited: OpenSearchEngine?

  private var isAutoAddEnabled = false

  private var dataTask: URLSessionDataTask? {
    didSet {
      oldValue?.cancel()
    }
  }

  private var faviconTask: Task<Void, Error>?

  fileprivate var faviconImage: UIImage?

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

    changeAddButton(for: engineToBeEdited != nil ? .enabled : .disabled)
  }

  // MARK: Internal

  private func setup() {
    tableView.do {
      $0.register(URLInputTableViewCell.self)
      $0.register(TitleInputTableViewCell.self)
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

  private func changeAddButton(for type: AddButtonType) {
    ensureMainThread {
      switch type {
      case .enabled:
        self.navigationItem.rightBarButtonItem = UIBarButtonItem(
          title: Strings.CustomSearchEngine.customEngineAddButtonTitle,
          style: .done,
          target: self,
          action: #selector(self.checkAddEngineType)
        )
        self.spinnerView.stopAnimating()
      case .disabled:
        self.navigationItem.rightBarButtonItem = UIBarButtonItem(
          title: Strings.CustomSearchEngine.customEngineAddButtonTitle,
          style: .done,
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

  private func handleError(error: Error) {
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

    changeAddButton(for: .disabled)
    addSearchEngine(with: urlQuery, title: title)
  }

  @objc func cancel() {
    navigationController?.popViewController(animated: true)
  }
}

// MARK: - UITableViewDelegate UITableViewDataSource

extension SearchCustomEngineViewController: UITableViewDelegate, UITableViewDataSource {

  func numberOfSections(in tableView: UITableView) -> Int {
    return Section.allCases.count
  }

  func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
    return 1
  }

  func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
    switch indexPath.section {
    case Section.url.rawValue:
      let cell = tableView.dequeueReusableCell(for: indexPath) as URLInputTableViewCell
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
      let cell = tableView.dequeueReusableCell(for: indexPath) as TitleInputTableViewCell
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

// MARK: Auto Add Engine

extension SearchCustomEngineViewController {

  fileprivate func addOpenSearchEngine() {
    guard var referenceURLString = openSearchReference?.reference,
      let title = openSearchReference?.title,
      var referenceURL = URL(string: referenceURLString),
      let faviconImage = faviconImage,
      let hostURLString = host?.absoluteString
    else {
      let alert = ThirdPartySearchAlerts.failedToAddThirdPartySearch()
      present(alert, animated: true, completion: nil)
      return
    }

    while referenceURLString.hasPrefix("/") {
      referenceURLString.remove(at: referenceURLString.startIndex)
    }

    let constructedReferenceURLString = "\(hostURLString)/\(referenceURLString)"

    if referenceURL.host == nil,
      let constructedReferenceURL = URL(string: constructedReferenceURLString)
    {
      referenceURL = constructedReferenceURL
    }

    downloadOpenSearchXML(
      referenceURL,
      referenceURL: referenceURLString,
      title: title,
      iconImage: faviconImage
    )
  }

  func downloadOpenSearchXML(_ url: URL, referenceURL: String, title: String, iconImage: UIImage) {
    changeAddButton(for: .loading)
    view.endEditing(true)

    NetworkManager().downloadResource(with: url) { [weak self] response in
      guard let self = self else { return }

      switch response {
      case .success(let response):
        Task { @MainActor in
          if let openSearchEngine = await OpenSearchParser(pluginMode: true).parse(
            response.data,
            referenceURL: referenceURL,
            image: iconImage,
            isCustomEngine: true
          ) {
            self.addSearchEngine(openSearchEngine)
          } else {
            let alert = ThirdPartySearchAlerts.failedToAddThirdPartySearch()

            self.present(alert, animated: true) {
              self.changeAddButton(for: .disabled)
            }
          }
        }
      case .failure(let error):
        Logger.module.error("\(error.localizedDescription)")

        let alert = ThirdPartySearchAlerts.failedToAddThirdPartySearch()
        self.present(alert, animated: true) {
          self.changeAddButton(for: .disabled)
        }
      }
    }
  }

  func addSearchEngine(_ engine: OpenSearchEngine) {
    let alert = ThirdPartySearchAlerts.addThirdPartySearchEngine(engine) {
      [weak self] alertAction in
      guard let self = self else { return }

      if alertAction.style == .cancel {
        self.changeAddButton(for: .enabled)
        return
      }

      Task { @MainActor in
        do {
          try await self.profile.searchEngines.addSearchEngine(engine)
          self.cancel()
        } catch {
          self.handleError(error: SearchEngineError.failedToSave)

          self.changeAddButton(for: .disabled)
        }
      }
    }

    self.present(alert, animated: true, completion: {})
  }
}

// MARK: Auto Add Meta Data

extension SearchCustomEngineViewController {

  func checkSupportAutoAddSearchEngine() {
    guard let openSearchEngine = openSearchReference else {
      changeAddButton(for: .disabled)
      checkManualAddExists()

      faviconImage = nil

      return
    }

    let searchEngineExists = profile.searchEngines.orderedEngines.contains(where: {
      let nameExists = $0.shortName.lowercased() == openSearchEngine.title?.lowercased() ?? ""

      if let referenceURL = $0.referenceURL {
        return openSearchEngine.reference.contains(referenceURL) || nameExists
      }

      return nameExists
    })

    if searchEngineExists {
      changeAddButton(for: .disabled)
      checkManualAddExists()
    } else {
      changeAddButton(for: .enabled)
      isAutoAddEnabled = true
    }
  }

  func fetchSearchEngineSupportForHost(_ host: URL) {
    changeAddButton(for: .disabled)

    dataTask = URLSession.shared.dataTask(with: host) { [weak self] data, _, error in
      guard let data = data, error == nil else {
        self?.openSearchReference = nil
        return
      }

      ensureMainThread {
        self?.loadSearchEngineMetaData(from: data, url: host)
      }
    }

    dataTask?.resume()
  }

  func loadSearchEngineMetaData(from data: Data, url: URL) {
    guard let root = try? HTMLDocument(data: data as Data),
      let searchEngineDetails = fetchOpenSearchReference(document: root)
    else {
      openSearchReference = nil
      return
    }

    faviconTask?.cancel()
    faviconTask = Task { @MainActor in
      do {
        let icon = try await FaviconFetcher.loadIcon(
          url: url,
          persistent: !privateBrowsingManager.isPrivateBrowsing
        )
        self.faviconImage = icon.image ?? Favicon.defaultImage
      } catch {
        self.faviconImage = Favicon.defaultImage
      }

      self.openSearchReference = searchEngineDetails
    }
  }

  func fetchOpenSearchReference(document: HTMLDocument) -> OpenSearchReference? {
    let documentXpath = "//head//link[contains(@type, 'application/opensearchdescription+xml')]"

    for link in document.xpath(documentXpath) {
      if let referenceLink = link["href"], let title = link["title"] {
        return OpenSearchReference(reference: referenceLink, title: title)
      }
    }

    return nil
  }
}

// MARK: Manual Add Engine

extension SearchCustomEngineViewController {

  fileprivate func addSearchEngine(with urlQuery: String, title: String) {
    changeAddButton(for: .loading)

    let safeURLQuery = urlQuery.trimmingCharacters(in: .whitespacesAndNewlines)
    let safeTitle = title.trimmingCharacters(in: .whitespacesAndNewlines)

    createSearchEngine(using: safeURLQuery, name: safeTitle) { [weak self] engine, error in
      guard let self = self else { return }

      if let error = error {
        self.handleError(error: error)

        self.changeAddButton(for: .disabled)
      } else if let engine = engine {
        Task { @MainActor in
          do {
            try await self.profile.searchEngines.addSearchEngine(engine)
            self.cancel()
          } catch {
            self.handleError(error: SearchEngineError.failedToSave)

            self.changeAddButton(for: .enabled)
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
    // Check Search Query is not valid
    guard let template = getSearchTemplate(with: query),
      let urlText = template.addingPercentEncoding(withAllowedCharacters: .urlFragmentAllowed),
      let url = URL(string: urlText),
      url.isWebPage()
    else {
      completion(nil, SearchEngineError.invalidQuery)
      return
    }

    // Check Engine Exists
    guard
      profile.searchEngines.orderedEngines.filter({
        $0.shortName == name || $0.searchTemplate.contains(template)
      }).isEmpty
    else {
      completion(nil, SearchEngineError.duplicate)
      return
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

  private func checkManualAddExists() {
    guard let url = urlText, let title = titleText else {
      return
    }

    if !url.isEmpty, !title.isEmpty {
      changeAddButton(for: .enabled)
    } else {
      changeAddButton(for: .disabled)
    }
  }
}

// MARK: - UITextViewDelegate

extension SearchCustomEngineViewController: UITextViewDelegate {

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

    // The default text "https://" cant ne deleted or changed so nothing without a secure scheme can be added
    return textLengthInRange >= 8
  }

  func textViewDidChange(_ textView: UITextView) {
    changeAddButton(for: .disabled)

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

extension SearchCustomEngineViewController: UITextFieldDelegate {

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

// MARK: - SearchEngineTableViewHeader

private class SearchEngineTableViewHeader: UITableViewHeaderFooterView, TableViewReusable {

  // MARK: UX

  struct UX {
    static let addButtonInset: CGFloat = 10
  }

  // MARK: Properties

  var titleLabel = UILabel().then {
    $0.font = UIFont.preferredFont(forTextStyle: .footnote)
    $0.textColor = .secondaryBraveLabel
  }

  lazy var addEngineButton = OpenSearchEngineButton(
    title: Strings.CustomSearchEngine.customEngineAutoAddTitle,
    hidesWhenDisabled: false
  ).then {
    $0.addTarget(self, action: #selector(addEngineAuto), for: .touchUpInside)
    $0.isHidden = true
  }

  var actionHandler: (() -> Void)?

  // MARK: Lifecycle

  override init(reuseIdentifier: String?) {
    super.init(reuseIdentifier: reuseIdentifier)

    addSubview(titleLabel)
    addSubview(addEngineButton)

    setConstraints()
  }

  required init?(coder aDecoder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  // MARK: Internal

  func setConstraints() {
    titleLabel.snp.makeConstraints { make in
      make.leading.equalTo(readableContentGuide)
      make.bottom.equalToSuperview().inset(4)
    }

    addEngineButton.snp.makeConstraints { make in
      make.trailing.equalTo(readableContentGuide)
      make.centerY.equalToSuperview()
      make.height.equalTo(snp.height)
    }
  }

  // MARK: Actions

  @objc private func addEngineAuto() {
    actionHandler?()
  }
}

// MARK: URLInputTableViewCell

private class URLInputTableViewCell: UITableViewCell, TableViewReusable {

  // MARK: UX

  struct UX {
    static let textViewHeight: CGFloat = 88
    static let textViewInset: CGFloat = 16
  }

  // MARK: Properties

  var textview = UITextView(frame: .zero)

  weak var delegate: UITextViewDelegate? {
    didSet {
      textview.delegate = delegate
    }
  }
  // MARK: Lifecycle

  override init(style: UITableViewCell.CellStyle, reuseIdentifier: String?) {
    super.init(style: style, reuseIdentifier: reuseIdentifier)

    setup()
  }

  required init?(coder aDecoder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  // MARK: Internal

  private func setup() {
    textview = UITextView(
      frame: CGRect(x: 0, y: 0, width: contentView.frame.width, height: contentView.frame.height)
    ).then {
      $0.text = "https://"
      $0.backgroundColor = .clear
      $0.font = UIFont.preferredFont(forTextStyle: .body)
      $0.autocapitalizationType = .none
      $0.autocorrectionType = .no
      $0.spellCheckingType = .no
      $0.keyboardType = .URL
      $0.textColor = .braveLabel
    }

    contentView.addSubview(textview)

    textview.snp.makeConstraints({ make in
      make.leading.trailing.equalToSuperview().inset(UX.textViewInset)
      make.bottom.top.equalToSuperview()
      make.height.equalTo(UX.textViewHeight)
    })
  }
}

// MARK: TitleInputTableViewCell

private class TitleInputTableViewCell: UITableViewCell, TableViewReusable {

  // MARK: UX

  struct UX {
    static let textFieldInset: CGFloat = 16
  }

  // MARK: Properties

  var textfield: UITextField = UITextField(frame: .zero)

  weak var delegate: UITextFieldDelegate? {
    didSet {
      textfield.delegate = delegate
    }
  }

  // MARK: Lifecycle

  override init(style: UITableViewCell.CellStyle, reuseIdentifier: String?) {
    super.init(style: style, reuseIdentifier: reuseIdentifier)

    setup()
  }

  required init?(coder aDecoder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  // MARK: Internal

  private func setup() {
    textfield = UITextField(
      frame: CGRect(x: 0, y: 0, width: contentView.frame.width, height: contentView.frame.height)
    )

    contentView.addSubview(textfield)

    textfield.snp.makeConstraints({ make in
      make.leading.trailing.equalToSuperview().inset(UX.textFieldInset)
      make.bottom.top.equalToSuperview().inset(UX.textFieldInset)
    })
  }
}
