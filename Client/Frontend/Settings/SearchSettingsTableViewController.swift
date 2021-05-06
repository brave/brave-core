/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import Shared

private let log = Logger.browserLogger

// MARK: - SearchEnginePickerDelegate

protocol SearchEnginePickerDelegate: class {
    func searchEnginePicker(_ searchEnginePicker: SearchEnginePicker?,
                            didSelectSearchEngine engine: OpenSearchEngine?, forType: DefaultEngineType?)
}

// MARK: - SearchSettingsTableViewController

class SearchSettingsTableViewController: UITableViewController {
    
    // MARK: UX
    
    struct UX {
        static let iconSize = CGSize(
            width: OpenSearchEngine.preferredIconSize,
            height: OpenSearchEngine.preferredIconSize)
        
        static let headerHeight: CGFloat = 44
    }
    
    // MARK: Constants
    
    struct Constants {
        static let addCustomEngineRowIdentifier = "addCustomEngineRowIdentifier"
        static let searchEngineRowIdentifier = "searchEngineRowIdentifier"
        static let showSearchSuggestionsRowIdentifier = "showSearchSuggestionsRowIdentifier"
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
        case suggestions
    }
    
    private var searchEngines: SearchEngines
    private let profile: Profile
    private var showDeletion = false
    
    private var searchPickerEngines: [OpenSearchEngine] {
        let orderedEngines = searchEngines.orderedEngines.sorted { $0.shortName < $1.shortName }
        
        guard let priorityEngine = InitialSearchEngines().priorityEngine?.rawValue else {
            return orderedEngines
        }
        
        return orderedEngines.sorted { engine, _ in
            engine.engineID == priorityEngine
        }
    }
    
    private var customSearchEngines: [OpenSearchEngine] {
        searchEngines.orderedEngines.filter { $0.isCustomEngine }
    }
    
    // MARK: Lifecycle
    
    init(profile: Profile) {
        self.profile = profile
        self.searchEngines = profile.searchEngines
        super.init(nibName: nil, bundle: nil)
    }
    
    required init?(coder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }
    
    override func viewDidLoad() {
        super.viewDidLoad()

        navigationItem.title = Strings.searchSettingNavTitle

        tableView.do {
            $0.allowsSelectionDuringEditing = true
            $0.registerHeaderFooter(SettingsTableSectionHeaderFooterView.self)
            $0.register(UITableViewCell.self, forCellReuseIdentifier: Constants.addCustomEngineRowIdentifier)
            $0.register(UITableViewCell.self, forCellReuseIdentifier: Constants.searchEngineRowIdentifier)
            $0.register(UITableViewCell.self, forCellReuseIdentifier: Constants.showSearchSuggestionsRowIdentifier)
            $0.register(UITableViewCell.self, forCellReuseIdentifier: Constants.quickSearchEngineRowIdentifier)
            $0.register(UITableViewCell.self, forCellReuseIdentifier: Constants.customSearchEngineRowIdentifier)
        }

        // Insert Done button if being presented outside of the Settings Nav stack
        if navigationController?.viewControllers.first === self {
            navigationItem.leftBarButtonItem =
                UIBarButtonItem(title: Strings.settingsSearchDoneButton, style: .done, target: self, action: #selector(dismissAnimated))
        }
        
        self.navigationItem.rightBarButtonItem = editButtonItem

        let footer = SettingsTableSectionHeaderFooterView(frame: CGRect(width: tableView.bounds.width, height: UX.headerHeight))
        tableView.tableFooterView = footer
    }
    
    override func viewWillAppear(_ animated: Bool) {
        super.viewWillAppear(animated)
        
        tableView.reloadData()
    }
    
    override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
        super.traitCollectionDidChange(previousTraitCollection)
        if traitCollection.userInterfaceStyle != previousTraitCollection?.userInterfaceStyle {
            updateThemeForUserInterfaceStyleChange()
        }
    }
    
    // MARK: Internal
    
    private func configureSearchEnginePicker(_ type: DefaultEngineType) -> SearchEnginePicker {
        return SearchEnginePicker(type: type, showCancel: false).then {
            // Order alphabetically, so that picker is always consistently ordered.
            // Every engine is a valid choice for the default engine, even the current default engine.
            // In private mode only custom engines will not be shown
            $0.engines = type == .privateMode ? searchPickerEngines.filter { !$0.isCustomEngine } : searchPickerEngines
            $0.delegate = self
            $0.selectedSearchEngineName = searchEngines.defaultEngine(forType: type).shortName
        }
    }
    
    private func configureSearchEngineCell(type: DefaultEngineType, engineName: String?) -> UITableViewCell {
        guard let searchEngineName = engineName else { return UITableViewCell() }

        var text: String
        
        switch type {
        case .standard:
            text = Strings.standardTabSearch
        case .privateMode:
            text = Strings.privateTabSearch
        }
        
        let cell = UITableViewCell(style: .value1, reuseIdentifier: Constants.searchEngineRowIdentifier).then {
            $0.accessoryType = .disclosureIndicator
            $0.editingAccessoryType = .disclosureIndicator
            $0.accessibilityLabel = text
            $0.textLabel?.text = text
            $0.accessibilityValue = searchEngineName
            $0.detailTextLabel?.text = searchEngineName
        }
        
        return cell
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
    
    override func tableView(_ tableView: UITableView, heightForHeaderInSection section: Int) -> CGFloat {
        return UX.headerHeight
    }
    
    override func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
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
                    cell = tableView.dequeueReusableCell(withIdentifier: Constants.quickSearchEngineRowIdentifier, for: indexPath).then {
                        $0.textLabel?.text = Strings.quickSearchEngines
                        $0.accessoryType = .disclosureIndicator
                        $0.editingAccessoryType = .disclosureIndicator
                    }
                case CurrentEngineType.suggestions.rawValue:
                    let toggle = UISwitch().then {
                        $0.addTarget(self, action: #selector(didToggleSearchSuggestions), for: .valueChanged)
                        $0.isOn = searchEngines.shouldShowSearchSuggestions
                    }
                    
                    cell = tableView.dequeueReusableCell(withIdentifier: Constants.showSearchSuggestionsRowIdentifier, for: indexPath).then {
                        $0.textLabel?.text = Strings.searchSettingSuggestionCellTitle
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
                cell = tableView.dequeueReusableCell(withIdentifier: Constants.addCustomEngineRowIdentifier, for: indexPath).then {
                    $0.textLabel?.text = Strings.searchSettingAddCustomEngineCellTitle
                    $0.accessoryType = .disclosureIndicator
                    $0.editingAccessoryType = .disclosureIndicator
                }
            } else {
                engine = customSearchEngines[indexPath.item]
                
                cell = tableView.dequeueReusableCell(withIdentifier: Constants.customSearchEngineRowIdentifier, for: indexPath).then {
                    $0.textLabel?.text = engine?.displayName
                    $0.textLabel?.adjustsFontSizeToFitWidth = true
                    $0.textLabel?.minimumScaleFactor = 0.5
                    $0.imageView?.image = engine?.image.createScaled(UX.iconSize)
                    $0.imageView?.layer.cornerRadius = 4
                    $0.imageView?.layer.masksToBounds = true
                    $0.selectionStyle = .none
                }
            }
        }

        guard let tableViewCell = cell else { return UITableViewCell() }
        tableViewCell.separatorInset = .zero

        return tableViewCell
    }
    
    override func tableView(_ tableView: UITableView, viewForHeaderInSection section: Int) -> UIView? {
        let headerView = tableView.dequeueReusableHeaderFooter() as SettingsTableSectionHeaderFooterView
        
        let sectionTitle = section == Section.current.rawValue ?
            Strings.currentlyUsedSearchEngines : Strings.customSearchEngines
        
        headerView.titleLabel.text = sectionTitle
        return headerView
    }

    override func tableView(_ tableView: UITableView, willSelectRowAt indexPath: IndexPath) -> IndexPath? {
        if indexPath.section == Section.current.rawValue && indexPath.item == CurrentEngineType.standard.rawValue {
            navigationController?.pushViewController(configureSearchEnginePicker(.standard), animated: true)
        } else if indexPath.section == Section.current.rawValue && indexPath.item == CurrentEngineType.private.rawValue {
            navigationController?.pushViewController(configureSearchEnginePicker(.privateMode), animated: true)
        } else if indexPath.section == Section.current.rawValue && indexPath.item == CurrentEngineType.quick.rawValue {
            let quickSearchEnginesViewController = SearchQuickEnginesViewController(profile: profile)
            navigationController?.pushViewController(quickSearchEnginesViewController, animated: true)
        } else if indexPath.section == Section.customSearch.rawValue && indexPath.item == customSearchEngines.count {
            let customEngineViewController = SearchCustomEngineViewController(profile: profile)
            navigationController?.pushViewController(customEngineViewController, animated: true)
        }
        
        return nil
    }

    // Determine whether to show delete button in edit mode
    override func tableView(_ tableView: UITableView, editingStyleForRowAt indexPath: IndexPath) -> UITableViewCell.EditingStyle {
        guard indexPath.section == Section.customSearch.rawValue, indexPath.row != customSearchEngines.count else {
            return .none
        }
        
        return .delete
    }

    // Determine whether to indent while in edit mode for deletion
    override func tableView(_ tableView: UITableView, shouldIndentWhileEditingRowAt indexPath: IndexPath) -> Bool {
        return indexPath.section == Section.customSearch.rawValue && indexPath.row != customSearchEngines.count
    }

    override func tableView(_ tableView: UITableView, commit editingStyle: UITableViewCell.EditingStyle, forRowAt indexPath: IndexPath) {
        if editingStyle == .delete {
            guard let engine = customSearchEngines[safe: indexPath.row] else { return }
            
            do {
                try searchEngines.deleteCustomEngine(engine)
                tableView.deleteRows(at: [indexPath], with: .right)
            } catch {
                log.error("Search Engine Error while deleting")
            }
        }
    }
    
    override func tableView(_ tableView: UITableView, canEditRowAt indexPath: IndexPath) -> Bool {
        if let engine = customSearchEngines[safe: indexPath.row],
           engine == searchEngines.defaultEngine(forType: .standard) {
            return false
        }
        
        return true
    }
}

// MARK: - Actions

extension SearchSettingsTableViewController {
    
    @objc func didToggleSearchSuggestions(_ toggle: UISwitch) {
        // Setting the value in settings dismisses any opt-in.
        searchEngines.shouldShowSearchSuggestions = toggle.isOn
        searchEngines.shouldShowSearchSuggestionsOptIn = false
    }

    @objc func dismissAnimated() {
        self.dismiss(animated: true, completion: nil)
    }
}

// MARK: SearchEnginePickerDelegate

extension SearchSettingsTableViewController: SearchEnginePickerDelegate {
    
    func searchEnginePicker(_ searchEnginePicker: SearchEnginePicker?,
                            didSelectSearchEngine searchEngine: OpenSearchEngine?, forType: DefaultEngineType?) {
        if let engine = searchEngine, let type = forType {
            searchEngines.updateDefaultEngine(engine.shortName, forType: type)
            self.tableView.reloadData()
        }
        _ = navigationController?.popViewController(animated: true)
    }
}
