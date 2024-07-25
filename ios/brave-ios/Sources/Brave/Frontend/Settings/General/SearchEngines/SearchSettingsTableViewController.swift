// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Shared
import UIKit
import os.log

// MARK: - SearchEnginePickerDelegate

protocol SearchEnginePickerDelegate: AnyObject {
  func searchEnginePicker(
    _ searchEnginePicker: SearchEnginePicker?,
    didSelectSearchEngine engine: OpenSearchEngine?,
    forType: DefaultEngineType?
  )
}

// MARK: - SearchSettingsTableViewController

class SearchSettingsTableViewController: UITableViewController {

  // MARK: UX

  struct UX {
    static let iconSize = CGSize(
      width: OpenSearchEngine.preferredIconSize,
      height: OpenSearchEngine.preferredIconSize
    )

    static let headerHeight: CGFloat = 44
  }

  // MARK: Constants

  struct Constants {
    static let addCustomEngineRowIdentifier = "addCustomEngineRowIdentifier"
    static let searchEngineRowIdentifier = "searchEngineRowIdentifier"
    static let showSearchSuggestionsRowIdentifier = "showSearchSuggestionsRowIdentifier"
    static let showRecentSearchesRowIdentifier = "showRecentSearchRowIdentifier"
    static let showBrowserSuggestionsRowIdentifier = "showBrowserSuggestionsRowIdentifier"
    static let quickSearchEngineRowIdentifier = "quickSearchEngineRowIdentifier"
    static let customSearchEngineRowIdentifier = "customSearchEngineRowIdentifier"
  }

  // MARK: Section

  enum Section: Int, CaseIterable {
    case current
    case customSearch
  }

  // MARK: CurrentEngineType

  enum CurrentEngineType: Int, CaseIterable {
    case standard
    case `private`
    case quick
    case searchSuggestions
    case recentSearches
    case browserSuggestions
  }

  private var searchEngines: SearchEngines
  private let profile: Profile
  private var showDeletion = false
  private var privateBrowsingManager: PrivateBrowsingManager

  private func searchPickerEngines(type: DefaultEngineType) -> [OpenSearchEngine] {
    var orderedEngines = searchEngines.orderedEngines
      .sorted { $0.shortName < $1.shortName }
      .sorted { engine, _ in engine.shortName == OpenSearchEngine.EngineNames.brave }

    if let priorityEngine = InitialSearchEngines().priorityEngine?.rawValue {
      orderedEngines =
        orderedEngines
        .sorted { engine, _ in
          engine.engineID == priorityEngine
        }
    }

    return orderedEngines
  }

  private var customSearchEngines: [OpenSearchEngine] {
    searchEngines.orderedEngines.filter { $0.isCustomEngine }
  }

  private lazy var dismissBarButton: UIBarButtonItem = {
    let doneButton = UIBarButtonItem(
      title: Strings.settingsSearchDoneButton,
      style: .done,
      target: self,
      action: #selector(close)
    )

    return doneButton
  }()

  private var isPresentedModally: Bool {
    navigationController?.viewControllers.first === self
  }

  // MARK: Lifecycle

  init(profile: Profile, privateBrowsingManager: PrivateBrowsingManager) {
    self.profile = profile
    self.privateBrowsingManager = privateBrowsingManager
    self.searchEngines = profile.searchEngines
    super.init(nibName: nil, bundle: nil)
  }

  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  override func viewDidLoad() {
    super.viewDidLoad()

    navigationItem.title = Strings.searchEngines

    tableView.do {
      $0.allowsSelectionDuringEditing = true
      $0.registerHeaderFooter(SettingsTableSectionHeaderFooterView.self)
      $0.register(
        UITableViewCell.self,
        forCellReuseIdentifier: Constants.addCustomEngineRowIdentifier
      )
      $0.register(UITableViewCell.self, forCellReuseIdentifier: Constants.searchEngineRowIdentifier)
      $0.register(
        UITableViewCell.self,
        forCellReuseIdentifier: Constants.showSearchSuggestionsRowIdentifier
      )
      $0.register(
        UITableViewCell.self,
        forCellReuseIdentifier: Constants.showRecentSearchesRowIdentifier
      )
      $0.register(
        UITableViewCell.self,
        forCellReuseIdentifier: Constants.showBrowserSuggestionsRowIdentifier
      )
      $0.register(
        UITableViewCell.self,
        forCellReuseIdentifier: Constants.quickSearchEngineRowIdentifier
      )
      $0.register(
        UITableViewCell.self,
        forCellReuseIdentifier: Constants.customSearchEngineRowIdentifier
      )
      $0.sectionHeaderTopPadding = 5
    }

    // Insert Done button if being presented outside of the Settings Nav stack
    if isPresentedModally {
      navigationItem.leftBarButtonItem = dismissBarButton
    }

    let footer = SettingsTableSectionHeaderFooterView(
      frame: CGRect(width: tableView.bounds.width, height: UX.headerHeight)
    )
    tableView.tableFooterView = footer
  }

  override func viewWillAppear(_ animated: Bool) {
    super.viewWillAppear(animated)

    updateTableEditModeVisibility()
    tableView.reloadData()
  }

  // MARK: Internal

  private func configureSearchEnginePicker(_ type: DefaultEngineType) -> SearchEnginePicker {
    return SearchEnginePicker(type: type, showCancel: false).then {
      // Order alphabetically, so that picker is always consistently ordered.
      // Every engine is a valid choice for the default engine, even the current default engine.
      $0.engines = searchPickerEngines(type: type)
      $0.delegate = self
      $0.selectedSearchEngineName = searchEngines.defaultEngine(forType: type)?.shortName
    }
  }

  private func configureSearchEngineCell(
    type: DefaultEngineType,
    engineName: String?
  ) -> UITableViewCell {
    guard let searchEngineName = engineName else { return UITableViewCell() }

    var text: String

    switch type {
    case .standard:
      text = Strings.standardTabSearch
    case .privateMode:
      text = Strings.privateTabSearch
    }

    let cell = UITableViewCell(style: .value1, reuseIdentifier: Constants.searchEngineRowIdentifier)
      .then {
        $0.accessoryType = .disclosureIndicator
        $0.editingAccessoryType = .disclosureIndicator
        $0.accessibilityLabel = text
        $0.textLabel?.text = text
        $0.accessibilityValue = searchEngineName
        $0.detailTextLabel?.text = searchEngineName
      }

    return cell
  }

  private func updateTableEditModeVisibility() {
    tableView.endEditing(true)

    navigationItem.setRightBarButton(
      customSearchEngines.isEmpty ? nil : editButtonItem,
      animated: true
    )
  }

  // MARK: TableViewDataSource - TableViewDelegate

  override func numberOfSections(in tableView: UITableView) -> Int {
    return Section.allCases.count
  }

  override func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
    if section == Section.current.rawValue {
      return CurrentEngineType.allCases.count
    } else {
      // Adding an extra row for Add Search Engine Entry
      return customSearchEngines.count + 1
    }
  }

  override func tableView(
    _ tableView: UITableView,
    heightForHeaderInSection section: Int
  ) -> CGFloat {
    return UX.headerHeight
  }

  override func tableView(
    _ tableView: UITableView,
    cellForRowAt indexPath: IndexPath
  ) -> UITableViewCell {
    var cell: UITableViewCell?
    var engine: OpenSearchEngine?

    if indexPath.section == Section.current.rawValue {
      switch indexPath.item {
      case CurrentEngineType.standard.rawValue:
        engine = searchEngines.defaultEngine(forType: .standard)
        cell = configureSearchEngineCell(type: .standard, engineName: engine?.displayName)
      case CurrentEngineType.private.rawValue:
        engine = searchEngines.defaultEngine(forType: .privateMode)
        cell = configureSearchEngineCell(type: .privateMode, engineName: engine?.displayName)
      case CurrentEngineType.quick.rawValue:
        cell = tableView.dequeueReusableCell(
          withIdentifier: Constants.quickSearchEngineRowIdentifier,
          for: indexPath
        ).then {
          $0.textLabel?.text = Strings.quickSearchEngines
          $0.accessoryType = .disclosureIndicator
          $0.editingAccessoryType = .disclosureIndicator
        }
      case CurrentEngineType.searchSuggestions.rawValue:
        let toggle = UISwitch().then {
          $0.addTarget(self, action: #selector(didToggleSearchSuggestions), for: .valueChanged)
          $0.isOn = searchEngines.shouldShowSearchSuggestions
        }

        cell = tableView.dequeueReusableCell(
          withIdentifier: Constants.showSearchSuggestionsRowIdentifier,
          for: indexPath
        ).then {
          $0.textLabel?.text = Strings.searchSettingSuggestionCellTitle
          $0.accessoryView = toggle
          $0.selectionStyle = .none
        }
      case CurrentEngineType.recentSearches.rawValue:
        let toggle = UISwitch().then {
          $0.addTarget(self, action: #selector(didToggleRecentSearches), for: .valueChanged)
          $0.isOn = searchEngines.shouldShowRecentSearches
        }

        cell = tableView.dequeueReusableCell(
          withIdentifier: Constants.showRecentSearchesRowIdentifier,
          for: indexPath
        ).then {
          $0.textLabel?.text = Strings.searchSettingRecentSearchesCellTitle
          $0.accessoryView = toggle
          $0.selectionStyle = .none
        }
      case CurrentEngineType.browserSuggestions.rawValue:
        let toggle = UISwitch().then {
          $0.addTarget(self, action: #selector(didToggleBrowserSuggestions), for: .valueChanged)
          $0.isOn = searchEngines.shouldShowBrowserSuggestions
        }

        cell = UITableViewCell(
          style: .subtitle,
          reuseIdentifier: Constants.showBrowserSuggestionsRowIdentifier
        ).then {
          $0.textLabel?.text = Strings.searchSettingBrowserSuggestionCellTitle
          $0.detailTextLabel?.numberOfLines = 0
          $0.detailTextLabel?.textColor = .secondaryBraveLabel
          $0.detailTextLabel?.text = Strings.searchSettingBrowserSuggestionCellDescription
          $0.accessoryView = toggle
          $0.selectionStyle = .none
        }
      default:
        // Should not happen.
        break
      }
    } else {
      // Add custom engine
      if indexPath.item == customSearchEngines.count {
        cell = tableView.dequeueReusableCell(
          withIdentifier: Constants.addCustomEngineRowIdentifier,
          for: indexPath
        ).then {
          $0.textLabel?.text = Strings.searchSettingAddCustomEngineCellTitle
          $0.accessoryType = .disclosureIndicator
          $0.editingAccessoryType = .disclosureIndicator
        }
      } else {
        engine = customSearchEngines[indexPath.item]

        cell = tableView.dequeueReusableCell(
          withIdentifier: Constants.customSearchEngineRowIdentifier,
          for: indexPath
        ).then {
          $0.textLabel?.text = engine?.displayName
          $0.textLabel?.adjustsFontSizeToFitWidth = true
          $0.textLabel?.minimumScaleFactor = 0.5
          $0.imageView?.image = engine?.image.createScaled(UX.iconSize)
          $0.imageView?.layer.cornerRadius = 4
          $0.imageView?.layer.cornerCurve = .continuous
          $0.imageView?.layer.masksToBounds = true
          $0.selectionStyle = .none
        }
      }
    }

    guard let tableViewCell = cell else { return UITableViewCell() }
    tableViewCell.separatorInset = .zero

    return tableViewCell
  }

  override func tableView(_ tableView: UITableView, viewForHeaderInSection section: Int) -> UIView?
  {
    let headerView = tableView.dequeueReusableHeaderFooter() as SettingsTableSectionHeaderFooterView

    let sectionTitle =
      section == Section.current.rawValue
      ? Strings.currentlyUsedSearchEngines : Strings.customSearchEngines

    headerView.titleLabel.text = sectionTitle
    return headerView
  }
}

// MARK: - Edit Table Data

extension SearchSettingsTableViewController {
  override func tableView(
    _ tableView: UITableView,
    willSelectRowAt indexPath: IndexPath
  ) -> IndexPath? {
    if indexPath.section == Section.current.rawValue
      && indexPath.item == CurrentEngineType.standard.rawValue
    {
      navigationController?.pushViewController(
        configureSearchEnginePicker(.standard),
        animated: true
      )
    } else if indexPath.section == Section.current.rawValue
      && indexPath.item == CurrentEngineType.private.rawValue
    {
      navigationController?.pushViewController(
        configureSearchEnginePicker(.privateMode),
        animated: true
      )
    } else if indexPath.section == Section.current.rawValue
      && indexPath.item == CurrentEngineType.quick.rawValue
    {
      let quickSearchEnginesViewController = SearchQuickEnginesViewController(
        profile: profile,
        isPrivateBrowsing: privateBrowsingManager.isPrivateBrowsing
      )
      navigationController?.pushViewController(quickSearchEnginesViewController, animated: true)
    } else if indexPath.section == Section.customSearch.rawValue
      && indexPath.item == customSearchEngines.count
    {
      let customEngineViewController = SearchCustomEngineViewController(
        profile: profile,
        privateBrowsingManager: privateBrowsingManager
      )
      navigationController?.pushViewController(customEngineViewController, animated: true)
    }

    return nil
  }

  // Determine whether to show delete button in edit mode
  override func tableView(
    _ tableView: UITableView,
    editingStyleForRowAt indexPath: IndexPath
  ) -> UITableViewCell.EditingStyle {
    guard indexPath.section == Section.customSearch.rawValue,
      indexPath.row != customSearchEngines.count
    else {
      return .none
    }

    return .delete
  }

  // Determine whether to indent while in edit mode for deletion
  override func tableView(
    _ tableView: UITableView,
    shouldIndentWhileEditingRowAt indexPath: IndexPath
  ) -> Bool {
    return indexPath.section == Section.customSearch.rawValue
      && indexPath.row != customSearchEngines.count
  }

  override func tableView(
    _ tableView: UITableView,
    trailingSwipeActionsConfigurationForRowAt indexPath: IndexPath
  ) -> UISwipeActionsConfiguration? {
    guard let engine = customSearchEngines[safe: indexPath.row] else {
      return nil
    }

    let deleteAction = UIContextualAction(style: .destructive, title: Strings.delete) {
      [weak self] action, view, completion in
      guard let self = self else {
        completion(false)
        return
      }

      self.deleteCustomSearchEngine(engine, using: tableView, at: indexPath) { status in
        completion(status)
      }
    }

    let editAction = UIContextualAction(style: .normal, title: Strings.edit) {
      [weak self] action, view, completion in
      guard let self = self else {
        completion(false)
        return
      }

      completion(true)

      let customEngineViewController = SearchCustomEngineViewController(
        profile: self.profile,
        privateBrowsingManager: self.privateBrowsingManager,
        engineToBeEdited: engine
      )

      navigationController?.pushViewController(customEngineViewController, animated: true)
    }

    return UISwipeActionsConfiguration(actions: [deleteAction, editAction])
  }

  private func deleteCustomSearchEngine(
    _ engine: OpenSearchEngine,
    using: UITableView,
    at indexPath: IndexPath,
    completion: @escaping (Bool) -> Void
  ) {
    func deleteCustomEngine() async {
      do {
        try await searchEngines.deleteCustomEngine(engine)
        tableView.deleteRows(at: [indexPath], with: .right)
        tableView.reloadData()
        updateTableEditModeVisibility()

        completion(true)
      } catch {
        Logger.module.error("Search Engine Error while deleting")

        completion(false)
      }
    }

    if engine == searchEngines.defaultEngine(forType: .standard) {
      let alert = UIAlertController(
        title: String(
          format: Strings.CustomSearchEngine.deleteEngineAlertTitle,
          engine.displayName
        ),
        message: Strings.CustomSearchEngine.deleteEngineAlertDescription,
        preferredStyle: .alert
      )

      alert.addAction(
        UIAlertAction(title: Strings.cancelButtonTitle, style: .cancel) { _ in
          completion(false)
        }
      )

      alert.addAction(
        UIAlertAction(title: Strings.delete, style: .destructive) { [weak self] _ in
          guard let self = self else { return }

          if let engine = self.searchEngines.defaultEngine(forType: .privateMode) {
            self.searchEngines.updateDefaultEngine(engine.shortName, forType: .standard)
          }
          Task {
            await deleteCustomEngine()
          }
        }
      )

      UIImpactFeedbackGenerator(style: .medium).vibrate()
      present(alert, animated: true, completion: nil)
    } else {
      Task {
        await deleteCustomEngine()
      }
    }
  }

  private func editCustomSearchEngine(_ engine: OpenSearchEngine) {

    let customEngineViewController = SearchCustomEngineViewController(
      profile: self.profile,
      privateBrowsingManager: self.privateBrowsingManager
    )
    navigationController?.pushViewController(customEngineViewController, animated: true)
  }

  override func tableView(_ tableView: UITableView, canEditRowAt indexPath: IndexPath) -> Bool {
    return indexPath.section == Section.customSearch.rawValue
  }

  override func setEditing(_ editing: Bool, animated: Bool) {
    super.setEditing(editing, animated: animated)

    // Hide the left bar button until edit action is finished
    navigationItem.setLeftBarButton(
      isPresentedModally && !editing ? dismissBarButton : nil,
      animated: true
    )
  }
}

// MARK: - Actions

extension SearchSettingsTableViewController {

  @objc func didToggleSearchSuggestions(_ toggle: UISwitch) {
    // Setting the value in settings dismisses any opt-in.
    searchEngines.shouldShowSearchSuggestions = toggle.isOn
    searchEngines.shouldShowSearchSuggestionsOptIn = false
  }

  @objc func didToggleRecentSearches(_ toggle: UISwitch) {
    // Setting the value in settings dismisses any opt-in.
    searchEngines.shouldShowRecentSearches = toggle.isOn
    searchEngines.shouldShowRecentSearchesOptIn = false
  }

  @objc func didToggleBrowserSuggestions(_ toggle: UISwitch) {
    // Setting the value effects all the modes private normal pbo
    searchEngines.shouldShowBrowserSuggestions = toggle.isOn
  }

  @objc func close() {
    self.dismiss(animated: true, completion: nil)
  }
}

// MARK: SearchEnginePickerDelegate

extension SearchSettingsTableViewController: SearchEnginePickerDelegate {

  func searchEnginePicker(
    _ searchEnginePicker: SearchEnginePicker?,
    didSelectSearchEngine searchEngine: OpenSearchEngine?,
    forType: DefaultEngineType?
  ) {
    if let engine = searchEngine, let type = forType {
      searchEngines.updateDefaultEngine(engine.shortName, forType: type)
      self.tableView.reloadData()
    }
    _ = navigationController?.popViewController(animated: true)
  }
}
