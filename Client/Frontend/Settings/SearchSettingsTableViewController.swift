/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import Shared

protocol SearchEnginePickerDelegate: class {
    func searchEnginePicker(_ searchEnginePicker: SearchEnginePicker?,
                            didSelectSearchEngine engine: OpenSearchEngine?, forType: DefaultEngineType?)
}

class SearchSettingsTableViewController: UITableViewController {
    fileprivate let SectionDefault = 0
    fileprivate let ItemDefaultEngine = 0
    fileprivate let ItemDefaultPrivateEngine = 1
    fileprivate let ItemDefaultSuggestions = 2
    fileprivate let NumberOfItemsInSectionDefault = 3
    fileprivate let SectionOrder = 1
    fileprivate let NumberOfSections = 2
    fileprivate let IconSize = CGSize(width: OpenSearchEngine.PreferredIconSize, height: OpenSearchEngine.PreferredIconSize)
    fileprivate let SectionHeaderIdentifier = "SectionHeaderIdentifier"
    
    fileprivate var showDeletion = false
    
    var profile: Profile?
    var tabManager: TabManager?

    var model: SearchEngines!

    override func viewDidLoad() {
        super.viewDidLoad()

        navigationItem.title = Strings.SearchSettingNavTitle

        // To allow re-ordering the list of search engines at all times.
        tableView.isEditing = true
        // So that we push the default search engine controller on selection.
        tableView.allowsSelectionDuringEditing = true

        tableView.register(SettingsTableSectionHeaderFooterView.self, forHeaderFooterViewReuseIdentifier: SectionHeaderIdentifier)

        // Insert Done button if being presented outside of the Settings Nav stack
        if !(self.navigationController is SettingsNavigationController) {
            self.navigationItem.leftBarButtonItem = UIBarButtonItem(title: Strings.SettingsSearchDoneButton, style: .done, target: self, action: #selector(self.dismissAnimated))
        }

        let footer = SettingsTableSectionHeaderFooterView(frame: CGRect(width: tableView.bounds.width, height: 44))
        footer.showBottomBorder = false
        tableView.tableFooterView = footer

        tableView.separatorColor = SettingsUX.TableViewSeparatorColor
        tableView.backgroundColor = SettingsUX.TableViewHeaderBackgroundColor
    }

    override func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
        var cell: UITableViewCell!
        var engine: OpenSearchEngine!

        if indexPath.section == SectionDefault {
            switch indexPath.item {
            case ItemDefaultEngine:
                engine = model.defaultEngine(forType: .standard)
                cell = configureSearchEngineCell(type: .standard, engineName: engine.shortName)
            case ItemDefaultPrivateEngine:
                engine = model.defaultEngine(forType: .privateMode)
                cell = configureSearchEngineCell(type: .privateMode, engineName: engine.shortName)
            case ItemDefaultSuggestions:
                cell = UITableViewCell(style: .default, reuseIdentifier: nil)
                cell.textLabel?.text = Strings.SearchSettingSuggestionCellTitle
                let toggle = UISwitch()
                toggle.onTintColor = UIConstants.ControlTintColor
                toggle.addTarget(self, action: #selector(didToggleSearchSuggestions), for: .valueChanged)
                toggle.isOn = model.shouldShowSearchSuggestions
                cell.editingAccessoryView = toggle
                cell.selectionStyle = .none
            default:
                // Should not happen.
                break
            }
        } else {
            // The default engine is not a quick search engine.
            let index = indexPath.item + 1
            engine = model.orderedEngines[index]
            
            cell = UITableViewCell(style: .default, reuseIdentifier: nil)
            cell.showsReorderControl = true
            
            let toggle = UISwitch()
            toggle.onTintColor = UIConstants.ControlTintColor
            // This is an easy way to get from the toggle control to the corresponding index.
            toggle.tag = index
            toggle.addTarget(self, action: #selector(didToggleEngine), for: .valueChanged)
            toggle.isOn = model.isEngineEnabled(engine)
            
            cell.editingAccessoryView = toggle
            cell.textLabel?.text = engine.shortName
            cell.textLabel?.adjustsFontSizeToFitWidth = true
            cell.textLabel?.minimumScaleFactor = 0.5
            cell.imageView?.image = engine.image.createScaled(IconSize)
            cell.imageView?.layer.cornerRadius = 4
            cell.imageView?.layer.masksToBounds = true
            cell.selectionStyle = .none
        }

        // So that the seperator line goes all the way to the left edge.
        cell.separatorInset = .zero

        return cell
    }
    
    private func configureSearchEngineCell(type: DefaultEngineType, engineName: String) -> UITableViewCell {
        var text: String
        
        switch type {
        case .standard:
            text = Strings.StandardTabSearch
        case .privateMode:
            text = Strings.PrivateTabSearch
        }
        
        let cell = UITableViewCell(style: UITableViewCell.CellStyle.value1, reuseIdentifier: nil)
        cell.editingAccessoryType = UITableViewCell.AccessoryType.disclosureIndicator
        cell.accessibilityLabel = text
        cell.textLabel?.text = text
        cell.accessibilityValue = engineName
        cell.detailTextLabel?.text = engineName
        
        return cell
    }

    override func numberOfSections(in tableView: UITableView) -> Int {
        return NumberOfSections
    }

    override func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
        if section == SectionDefault {
            return NumberOfItemsInSectionDefault
        } else {
            // The first engine -- the default engine -- is not shown in the quick search engine list.
            // But the option to add Custom Engine is.
            return model.orderedEngines.count - 1
        }
    }

    override func tableView(_ tableView: UITableView, willSelectRowAt indexPath: IndexPath) -> IndexPath? {
        if indexPath.section == SectionDefault && indexPath.item == ItemDefaultEngine {
            let searchEnginePicker = SearchEnginePicker(type: .standard)
            // Order alphabetically, so that picker is always consistently ordered.
            // Every engine is a valid choice for the default engine, even the current default engine.
            searchEnginePicker.engines = model.orderedEngines.sorted { $0.shortName < $1.shortName }
            searchEnginePicker.delegate = self
            searchEnginePicker.selectedSearchEngineName = model.defaultEngine(forType: .standard).shortName
            navigationController?.pushViewController(searchEnginePicker, animated: true)
        } else if indexPath.section == SectionDefault && indexPath.item == ItemDefaultPrivateEngine {
            let searchEnginePicker = SearchEnginePicker(type: .privateMode)
            // Order alphabetically, so that picker is always consistently ordered.
            // Every engine is a valid choice for the default engine, even the current default engine.
            searchEnginePicker.engines = model.orderedEngines.sorted { $0.shortName < $1.shortName }
            searchEnginePicker.delegate = self
            searchEnginePicker.selectedSearchEngineName = model.defaultEngine(forType: .privateMode).shortName
            navigationController?.pushViewController(searchEnginePicker, animated: true)
        }
        return nil
    }

    // Don't show delete button on the left.
    override func tableView(_ tableView: UITableView, editingStyleForRowAt indexPath: IndexPath) -> UITableViewCell.EditingStyle {
        if indexPath.section == SectionDefault || indexPath.item + 1 == model.orderedEngines.count {
            return UITableViewCell.EditingStyle.none
        }

        let index = indexPath.item + 1
        let engine = model.orderedEngines[index]
        return (self.showDeletion && engine.isCustomEngine) ? .delete : .none
    }

    // Don't reserve space for the delete button on the left.
    override func tableView(_ tableView: UITableView, shouldIndentWhileEditingRowAt indexPath: IndexPath) -> Bool {
        return false
    }

    // Hide a thin vertical line that iOS renders between the accessoryView and the reordering control.
    override func tableView(_ tableView: UITableView, willDisplay cell: UITableViewCell, forRowAt indexPath: IndexPath) {
        if cell.isEditing {
            for v in cell.subviews where v.frame.width == 1.0 {
                v.backgroundColor = UIColor.clear
            }
        }
    }

    override func tableView(_ tableView: UITableView, heightForHeaderInSection section: Int) -> CGFloat {
        return 44
    }

    override func tableView(_ tableView: UITableView, viewForHeaderInSection section: Int) -> UIView? {
        // swiftlint:disable:next force_cast
        let headerView = tableView.dequeueReusableHeaderFooterView(withIdentifier: SectionHeaderIdentifier) as! SettingsTableSectionHeaderFooterView
        
        let sectionTitle = section == SectionDefault ?
            Strings.CurrentlyUsedSearchEngines : Strings.QuickSearchEngines
        
        headerView.titleLabel.text = sectionTitle
        return headerView
    }

    override func tableView(_ tableView: UITableView, canMoveRowAt indexPath: IndexPath) -> Bool {
        if indexPath.section == SectionDefault || indexPath.item + 1 == model.orderedEngines.count {
            return false
        } else {
            return true
        }
    }

    override func tableView(_ tableView: UITableView, moveRowAt indexPath: IndexPath, to newIndexPath: IndexPath) {
        // The first engine (default engine) is not shown in the list, so the indices are off-by-1.
        let index = indexPath.item + 1
        let newIndex = newIndexPath.item + 1
        let engine = model.orderedEngines.remove(at: index)
        model.orderedEngines.insert(engine, at: newIndex)
        tableView.reloadData()
    }

    // Snap to first or last row of the list of engines.
    override func tableView(_ tableView: UITableView, targetIndexPathForMoveFromRowAt sourceIndexPath: IndexPath, toProposedIndexPath proposedDestinationIndexPath: IndexPath) -> IndexPath {
        // You can't drag or drop on the default engine.
        if sourceIndexPath.section == SectionDefault || proposedDestinationIndexPath.section == SectionDefault {
            return sourceIndexPath
        }

        //Can't drag/drop over "Add Custom Engine button"
        if sourceIndexPath.item + 1 == model.orderedEngines.count || proposedDestinationIndexPath.item + 1 == model.orderedEngines.count {
            return sourceIndexPath
        }

        if sourceIndexPath.section != proposedDestinationIndexPath.section {
            var row = 0
            if sourceIndexPath.section < proposedDestinationIndexPath.section {
                row = tableView.numberOfRows(inSection: sourceIndexPath.section) - 1
            }
            return IndexPath(row: row, section: sourceIndexPath.section)
        }
        return proposedDestinationIndexPath
    }

    override func tableView(_ tableView: UITableView, commit editingStyle: UITableViewCell.EditingStyle, forRowAt indexPath: IndexPath) {
        if editingStyle == .delete {
            let index = indexPath.item + 1
            let engine = model.orderedEngines[index]
            model.deleteCustomEngine(engine)
            tableView.deleteRows(at: [indexPath], with: .right)
        }
    }
}

// MARK: - Selectors
extension SearchSettingsTableViewController {
    @objc func didToggleEngine(_ toggle: UISwitch) {
        let engine = model.orderedEngines[toggle.tag] // The tag is 1-based.
        if toggle.isOn {
            model.enableEngine(engine)
        } else {
            model.disableEngine(engine)
        }
    }

    @objc func didToggleSearchSuggestions(_ toggle: UISwitch) {
        // Setting the value in settings dismisses any opt-in.
        model.shouldShowSearchSuggestions = toggle.isOn
        model.shouldShowSearchSuggestionsOptIn = false
    }

    func cancel() {
        _ = navigationController?.popViewController(animated: true)
    }

    @objc func dismissAnimated() {
        self.dismiss(animated: true, completion: nil)
    }
}

extension SearchSettingsTableViewController: SearchEnginePickerDelegate {
    func searchEnginePicker(_ searchEnginePicker: SearchEnginePicker?,
                            didSelectSearchEngine searchEngine: OpenSearchEngine?, forType: DefaultEngineType?) {
        if let engine = searchEngine, let type = forType {
            model.setDefaultEngine(engine.shortName, forType: type)
            self.tableView.reloadData()
        }
        _ = navigationController?.popViewController(animated: true)
    }
}
