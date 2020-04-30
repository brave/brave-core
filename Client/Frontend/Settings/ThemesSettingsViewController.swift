// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Static
import BraveShared
import Shared
import BraveRewards

class ThemesSettingsViewController: UITableViewController {
    
    var themeApplied: (() -> Void)?
    var themeChanged: ((_ themeId: String?) -> Void)?
    
    private enum Sections: Int, CaseIterable { case themes, appearance }
    
    override func viewDidLoad() {
        tableView.dataSource = self
        tableView.delegate = self
        tableView.register(UITableViewCell.self, forCellReuseIdentifier: "default")
        
        title = Strings.themesSettings
    }
    
    override func numberOfSections(in tableView: UITableView) -> Int {
        Sections.allCases.count
    }
    
    override func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
        let customThemesCount = customThemeIDs.count + 1
        let defaultThemesCount = Theme.DefaultTheme.normalThemesOptions.count
        
        return section == Sections.themes.rawValue ? customThemesCount : defaultThemesCount
    }
    
    override func tableView(_ tableView: UITableView, titleForHeaderInSection section: Int) -> String? {
        section == Sections.themes.rawValue ? Strings.themesSettings : Strings.themesDisplayBrightness
    }
    
    override func tableView(_ tableView: UITableView, titleForFooterInSection section: Int) -> String? {
        section == Sections.appearance.rawValue ? Strings.themesDisplayBrightnessFooter : nil
    }
    
    override func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
        tableView.deselectRow(at: indexPath, animated: true)
        
        if tableView.cellForRow(at: indexPath)?.accessoryType == .checkmark {
            // Don't trigger the theme update logic when selecting currently chosen theme.
            return
        }
        
        switch Sections(rawValue: indexPath.section) {
        case .themes:
            // Keep old value to know what resource to remove.
            if indexPath.row == 0 {
                // nil -> use default theme.
                Preferences.NewTabPage.selectedCustomTheme.reset()
                themeChanged?(nil)
                navigationController?.popViewController(animated: true)
                break
            }
            
            guard let customTheme = customThemeIDs[safe: indexPath.row - 1] else {
                assertionFailure()
                return
            }
            
            Preferences.NewTabPage.selectedCustomTheme.value = customTheme
            themeChanged?(customTheme)
            navigationController?.popViewController(animated: true)
            
        case .appearance:
            guard let theme = Theme.DefaultTheme.normalThemesOptions[safe: indexPath.row] else {
                assertionFailure()
                return
            }
            
            if Preferences.General.themeNormalMode.value != theme.key {
                Preferences.General.themeNormalMode.value = theme.key
                themeApplied?()
                navigationController?.popViewController(animated: true)
            }
        case .none:
            break
        }
        
        tableView.reloadData()
        
    }
    
    private var customThemeIDs: [String] {
        Preferences.NewTabPage.installedCustomThemes.value
    }
    
    private func themeName(for id: String) -> String? {
        let themeResource: NTPDownloader.ResourceType = .superReferral(code: id)
        
        guard let metadata = FileManager.default
            .urls(for: .applicationSupportDirectory, in: .userDomainMask).first?
            .appendingPathComponent(themeResource.saveTopFolderName)
            .appendingPathComponent(id)
            .appendingPathComponent(themeResource.resourceName) else { return nil }
        
        guard let data = try? Data(contentsOf: metadata) else { return nil }
        let json = try? JSONDecoder().decode(CustomTheme.self, from: data)
        
        return json?.themeName
    }
    
    override func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
        let cell = tableView.dequeueReusableCell(withIdentifier: "default", for: indexPath)
        
        switch Sections(rawValue: indexPath.section) {
        case .themes:
            if indexPath.row == 0 {
                cell.textLabel?.text = Strings.defaultThemeName
                if Preferences.NewTabPage.selectedCustomTheme.value == nil {
                    cell.accessoryType = .checkmark
                }
                break
            }
            
            guard let customTheme = customThemeIDs[safe: indexPath.row - 1] else {
                assertionFailure()
                return UITableViewCell()
            }
            
            cell.textLabel?.text = themeName(for: customTheme)
            if Preferences.NewTabPage.selectedCustomTheme.value == customTheme {
                cell.accessoryType = .checkmark
            }
            
        case .appearance:
            guard let theme = Theme.DefaultTheme.normalThemesOptions[safe: indexPath.row] else {
                assertionFailure()
                return UITableViewCell()
            }
            
            cell.textLabel?.text = theme.displayString
            
            if theme.key == Preferences.General.themeNormalMode.value {
                cell.accessoryType = .checkmark
            }
        case .none:
            assertionFailure()
            return UITableViewCell()
        }
        
        return cell
    }
}
