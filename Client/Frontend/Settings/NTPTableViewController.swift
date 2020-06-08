// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Static
import BraveShared
import Shared
import BraveRewards

class NTPTableViewController: TableViewController {
    enum BackgroundImageType: RepresentableOptionType {
        
        case defaultImages
        case sponsored
        case superReferrer(String)
        
        var key: String {
            displayString
        }
        
        public var displayString: String {
            switch self {
            case .defaultImages: return "(\(Strings.NTP.settingsDefaultImagesOnly))"
            case .sponsored: return Strings.NTP.settingsSponsoredImagesSelection
            case .superReferrer(let referrer): return referrer
            }
        }
    }
    
    init() {
        super.init(style: .grouped)
    }
    
    @available(*, unavailable)
    required init?(coder aDecoder: NSCoder) {
        fatalError()
    }
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        // Hides unnecessary empty rows
        tableView.tableFooterView = UIView()
        
        navigationItem.title = Strings.NTP.settingsTitle
        tableView.accessibilityIdentifier = "NewTabPageSettings.tableView"
        loadSections()
        
        Preferences.NewTabPage.backgroundImages.observe(from: self)
    }
    
    private func loadSections() {
        var section = Section(rows: [Row.boolRow(title: Strings.NTP.settingsBackgroundImages,
                                                 option: Preferences.NewTabPage.backgroundImages)])
        
        if Preferences.NewTabPage.backgroundImages.value {
            section.rows.append(backgroundImagesSetting(section: section))
        }
        
        dataSource.sections = [section]
    }
    
    private func selectedItem() -> BackgroundImageType {
        if let referrer = Preferences.NewTabPage.selectedCustomTheme.value {
            return .superReferrer(referrer)
        }
        
        return Preferences.NewTabPage.backgroundSponsoredImages.value ? .sponsored : .defaultImages
    }
    
    private lazy var backgroundImageOptions: [BackgroundImageType] = {
        var available: [BackgroundImageType] = [.defaultImages, .sponsored]
        available += Preferences.NewTabPage.installedCustomThemes.value.map {
            .superReferrer($0)
        }
        return available
    }()
    
    private func backgroundImagesSetting(section: Section) -> Row {
        var row = Row(
            text: Strings.NTP.settingsBackgroundImageSubMenu,
            detailText: selectedItem().displayString,
            accessory: .disclosureIndicator,
            cellClass: MultilineSubtitleCell.self)
        
        row.selection = { [unowned self] in
            // Show options for tab bar visibility
            let optionsViewController = OptionSelectionViewController<BackgroundImageType>(
                options: self.backgroundImageOptions,
                selectedOption: self.selectedItem(),
                optionChanged: { _, option in
                    // Should turn this off whenever possible to prevent unnecessary resource downloading
                    Preferences.NewTabPage.backgroundSponsoredImages.value = option == .sponsored
                    
                    if case .superReferrer(let referrer) = option {
                        Preferences.NewTabPage.selectedCustomTheme.value = referrer
                    } else {
                        Preferences.NewTabPage.selectedCustomTheme.value = nil
                    }
                    
                    self.dataSource.reloadCell(row: row, section: section, displayText: option.displayString)
                }
            )
            optionsViewController.navigationItem.title = Strings.NTP.settingsBackgroundImageSubMenu
            self.navigationController?.pushViewController(optionsViewController, animated: true)
        }
        return row
    }
}

extension NTPTableViewController: PreferencesObserver {
    func preferencesDidChange(for key: String) {
        loadSections()
    }
}
