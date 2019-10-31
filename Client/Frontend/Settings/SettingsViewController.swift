/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import Shared
import BraveShared
import Static
import SwiftKeychainWrapper
import LocalAuthentication
import SwiftyJSON
import Data
import WebKit
import BraveRewards
import BraveRewardsUI

extension TabBarVisibility: RepresentableOptionType {
    public var displayString: String {
        switch self {
        case .always: return Strings.Always_show
        case .landscapeOnly: return Strings.Show_in_landscape_only
        case .never: return Strings.Never_show
        }
    }
}

extension DataSource {
    /// Get the index path of a Row to modify it
    ///
    /// Since they are structs we cannot obtain references to them to alter them, we must directly access them
    /// from `sections[x].rows[y]`
    func indexPath(rowUUID: String, sectionUUID: String) -> IndexPath? {
        guard let section = sections.firstIndex(where: { $0.uuid == sectionUUID }),
            let row = sections[section].rows.firstIndex(where: { $0.uuid == rowUUID }) else {
                return nil
        }
        return IndexPath(row: row, section: section)
    }
}

protocol SettingsDelegate: class {
    func settingsOpenURLInNewTab(_ url: URL)
    func settingsOpenURLs(_ urls: [URL])
    func settingsDidFinish(_ settingsViewController: SettingsViewController)
}

class SettingsViewController: TableViewController {
    weak var settingsDelegate: SettingsDelegate?
    
    private let profile: Profile
    private let tabManager: TabManager
    private let rewards: BraveRewards?
    
    init(profile: Profile, tabManager: TabManager, rewards: BraveRewards? = nil) {
        self.profile = profile
        self.tabManager = tabManager
        self.rewards = rewards
        
        super.init(style: .grouped)
    }
    
    @available(*, unavailable)
    required init?(coder aDecoder: NSCoder) {
        fatalError()
    }
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        navigationItem.title = Strings.Settings
        tableView.accessibilityIdentifier = "SettingsViewController.tableView"
        dataSource.sections = sections
        
        applyTheme(theme)
    }
    
    private func displayRewardsDebugMenu() {
        guard let rewards = rewards else { return }
        let settings = QASettingsViewController(rewards: rewards)
        navigationController?.pushViewController(settings, animated: true)
    }
    
    private var theme: Theme {
        Theme.of(tabManager.selectedTab)
    }
    
    private var sections: [Section] {
        var list = [Section]()
        list.append(generalSection)
        list.append(displaySection)
        #if !NO_SYNC
            list.append(otherSettingsSection)
        #endif
        list.append(contentsOf: [privacySection,
                                 securitySection,
                                 shieldsSection,
                                 supportSection,
                                 aboutSection])
        
        if let debugSection = debugSection {
            list.append(debugSection)
        }

        return list
    }
    
    // MARK: - Sections
    
    private lazy var generalSection: Section = {
        var general = Section(
            header: .title(Strings.SettingsGeneralSectionTitle),
            rows: [
                Row(text: Strings.SearchEngines, selection: {
                    let viewController = SearchSettingsTableViewController()
                    viewController.model = self.profile.searchEngines
                    viewController.profile = self.profile
                    self.navigationController?.pushViewController(viewController, animated: true)
                }, accessory: .disclosureIndicator, cellClass: MultilineValue1Cell.self),
                BoolRow(title: Strings.Save_Logins, option: Preferences.General.saveLogins),
                BoolRow(title: Strings.Block_Popups, option: Preferences.General.blockPopups)
            ]
        )
        
        if #available(iOS 13.0, *), UIDevice.isIpad {
            general.rows.append(BoolRow(title: Strings.AlwaysRequestDesktopSite,
            option: Preferences.General.alwaysRequestDesktopSite))
        }
        
        return general
    }()
    
    private lazy var displaySection: Section = {
        var display = Section(
            header: .title(Strings.DisplaySettingsSection),
            rows: []
        )
        
        let reloadCell = { (row: Row, displayString: String) in
            if let indexPath = self.dataSource.indexPath(rowUUID: row.uuid, sectionUUID: display.uuid) {
                self.dataSource.sections[indexPath.section].rows[indexPath.row].detailText = displayString
            }
        }
        
        let themeSubtitle = Theme.DefaultTheme(rawValue: Preferences.General.themeNormalMode.value)?.displayString
        var row = Row(text: Strings.ThemesDisplayBrightness, detailText: themeSubtitle, accessory: .disclosureIndicator, cellClass: MultilineSubtitleCell.self)
        row.selection = { [unowned self] in
            let optionsViewController = OptionSelectionViewController<Theme.DefaultTheme>(
                options: Theme.DefaultTheme.normalThemesOptions,
                selectedOption: Theme.DefaultTheme(rawValue: Preferences.General.themeNormalMode.value),
                optionChanged: { [unowned self] _, option in
                    Preferences.General.themeNormalMode.value = option.rawValue
                    reloadCell(row, option.displayString)
                    self.applyTheme(self.theme)
                }
            )
            optionsViewController.headerText = Strings.ThemesDisplayBrightness
            optionsViewController.footerText = Strings.ThemesDisplayBrightnessFooter
            self.navigationController?.pushViewController(optionsViewController, animated: true)
        }
        display.rows.append(row)
        
        if UIDevice.current.userInterfaceIdiom == .pad {
            display.rows.append(
                Row(text: Strings.Show_Tabs_Bar, accessory: .switchToggle(value: Preferences.General.tabBarVisibility.value == TabBarVisibility.always.rawValue, { Preferences.General.tabBarVisibility.value = $0 ? TabBarVisibility.always.rawValue : TabBarVisibility.never.rawValue }), cellClass: MultilineValue1Cell.self)
            )
        } else {
            var row = Row(text: Strings.Show_Tabs_Bar, detailText: TabBarVisibility(rawValue: Preferences.General.tabBarVisibility.value)?.displayString, accessory: .disclosureIndicator, cellClass: MultilineSubtitleCell.self)
            row.selection = { [unowned self] in
                // Show options for tab bar visibility
                let optionsViewController = OptionSelectionViewController<TabBarVisibility>(
                    options: TabBarVisibility.allCases,
                    selectedOption: TabBarVisibility(rawValue: Preferences.General.tabBarVisibility.value),
                    optionChanged: { _, option in
                        Preferences.General.tabBarVisibility.value = option.rawValue
                        reloadCell(row, option.displayString)
                    }
                )
                optionsViewController.headerText = Strings.Show_Tabs_Bar
                self.navigationController?.pushViewController(optionsViewController, animated: true)
            }
            display.rows.append(row)
        }
        
        display.rows.append(
            BoolRow(title: Strings.Show_Bookmark_Button_In_Top_Toolbar, option: Preferences.General.showBookmarkToolbarShortcut)
        )
        
        return display
    }()
    
    private lazy var otherSettingsSection: Section = {
        // BRAVE TODO: Change it once we finalize our decision how to name the section.(#385)
        var section = Section(header: .title(Strings.OtherSettingsSection))
        #if !NO_REWARDS
        if let rewards = rewards {
            section.rows += [
                Row(text: Strings.BraveRewardsTitle, selection: { [unowned self] in
                    let rewardsVC = BraveRewardsSettingsViewController(rewards)
                    self.navigationController?.pushViewController(rewardsVC, animated: true)
                }, accessory: .disclosureIndicator),
            ]
        }
        #endif
        section.rows += [
            Row(text: Strings.Sync, selection: { [unowned self] in
                if Sync.shared.isInSyncGroup {
                    let syncSettingsVC = SyncSettingsTableViewController(style: .grouped)
                    syncSettingsVC.dismissHandler = {
                        self.navigationController?.popToRootViewController(animated: true)
                    }
                    
                    self.navigationController?.pushViewController(syncSettingsVC, animated: true)
                } else {
                    let view = SyncWelcomeViewController()
                    view.dismissHandler = {
                        view.navigationController?.popToRootViewController(animated: true)
                    }
                    self.navigationController?.pushViewController(view, animated: true)
                }
                }, accessory: .disclosureIndicator,
                   cellClass: MultilineValue1Cell.self),
            
            //Disabled until 1.13
            //BoolRow(title: Strings.Media_Auto_Plays, option: Preferences.General.mediaAutoPlays)
        ]
        return section
    }()
    
    private lazy var privacySection: Section = {
        var privacy = Section(
            header: .title(Strings.Privacy)
        )
        privacy.rows = [
            Row(text: Strings.ClearPrivateData,
                selection: { [unowned self] in
                    // Show Clear private data screen
                    let clearPrivateData = ClearPrivateDataTableViewController().then {
                        $0.profile = self.profile
                        $0.tabManager = self.tabManager
                    }
                    self.navigationController?.pushViewController(clearPrivateData, animated: true)
                },
                accessory: .disclosureIndicator,
                cellClass: MultilineValue1Cell.self
            ),
            BoolRow(title: Strings.Block_all_cookies, option: Preferences.Privacy.blockAllCookies, onValueChange: { [unowned self] in
                func toggleCookieSetting(with status: Bool) {
                    // Lock/Unlock Cookie Folder
                    let completionBlock: (Bool) -> Void = { _ in
                        let success = FileManager.default.setFolderAccess([
                            (.cookie, status),
                            (.webSiteData, status)
                            ])
                        if success {
                            Preferences.Privacy.blockAllCookies.value = status
                        } else {
                            //Revert the changes. Not handling success here to avoid a loop.
                            FileManager.default.setFolderAccess([
                                (.cookie, false),
                                (.webSiteData, false)
                                ])
                            self.toggleSwitch(on: false, section: self.privacySection, rowUUID: Preferences.Privacy.blockAllCookies.key)
                            
                            // TODO: Throw Alert to user to try again?
                            let alert = UIAlertController(title: nil, message: Strings.Block_all_cookies_failed_alert_msg, preferredStyle: .alert)
                            alert.addAction(UIAlertAction(title: Strings.OKString, style: .default))
                            self.present(alert, animated: true)
                        }
                    }
                    // Save cookie to disk before purge for unblock load.
                    status ? HTTPCookie.saveToDisk(completion: completionBlock) : completionBlock(true)
                }
                if $0 {
                    let status = $0
                    // THROW ALERT to inform user of the setting
                    let alert = UIAlertController(title: Strings.Block_all_cookies_alert_title, message: Strings.Block_all_cookies_alert_info, preferredStyle: .alert)
                    let okAction = UIAlertAction(title: Strings.Block_all_cookies_action, style: .destructive, handler: { (action) in
                        toggleCookieSetting(with: status)
                    })
                    alert.addAction(okAction)
                    
                    let cancelAction = UIAlertAction(title: Strings.CancelButtonTitle, style: .cancel, handler: { (action) in
                        self.toggleSwitch(on: false, section: self.privacySection, rowUUID: Preferences.Privacy.blockAllCookies.key)
                    })
                    alert.addAction(cancelAction)
                    self.present(alert, animated: true)
                } else {
                    toggleCookieSetting(with: $0)
                }
            })
        ]
        privacy.rows.append(
            BoolRow(
                title: Strings.Private_Browsing_Only,
                option: Preferences.Privacy.privateBrowsingOnly,
                onValueChange: {
                    Preferences.Privacy.privateBrowsingOnly.value = $0
                    
                    // Need to flush the table, hacky, but works consistenly and well
                    let superView = self.tableView.superview
                    self.tableView.removeFromSuperview()
                    DispatchQueue.main.async {
                        // Let shield toggle change propagate, otherwise theme may not be set properly
                        superView?.addSubview(self.tableView)
                        self.applyTheme(self.theme)
                    }
                }
            )
        )
        return privacy
    }()
    
    private lazy var securitySection: Section = {
        let passcodeTitle: String = {
            let localAuthContext = LAContext()
            if localAuthContext.canEvaluatePolicy(.deviceOwnerAuthenticationWithBiometrics, error: nil) {
                let title: String
                if localAuthContext.biometryType == .faceID {
                    return Strings.AuthenticationFaceIDPasscodeSetting
                } else {
                    return Strings.AuthenticationTouchIDPasscodeSetting
                }
            } else {
                return Strings.AuthenticationPasscode
            }
        }()
        
        return Section(
            header: .title(Strings.Security),
            rows: [
                Row(text: passcodeTitle, selection: { [unowned self] in
                    let passcodeSettings = PasscodeSettingsViewController()
                    self.navigationController?.pushViewController(passcodeSettings, animated: true)
                    }, accessory: .disclosureIndicator, cellClass: MultilineValue1Cell.self)
            ]
        )
    }()
    
    private lazy var shieldsSection: Section = {
        var shields = Section(
            header: .title(Strings.Brave_Shield_Defaults),
            rows: [
                BoolRow(title: Strings.Block_Ads_and_Tracking, option: Preferences.Shields.blockAdsAndTracking),
                BoolRow(title: Strings.HTTPS_Everywhere, option: Preferences.Shields.httpsEverywhere),
                BoolRow(title: Strings.Block_Phishing_and_Malware, option: Preferences.Shields.blockPhishingAndMalware),
                BoolRow(title: Strings.Block_Scripts, option: Preferences.Shields.blockScripts),
                BoolRow(title: Strings.Fingerprinting_Protection, option: Preferences.Shields.fingerprintingProtection),
            ]
        )
        if let locale = Locale.current.languageCode, let _ = ContentBlockerRegion.with(localeCode: locale) {
            shields.rows.append(BoolRow(title: Strings.Use_regional_adblock, option: Preferences.Shields.useRegionAdBlock))
        }
        return shields
    }()
    
    private lazy var supportSection: Section = {
        return Section(
            header: .title(Strings.Support),
            rows: [
                Row(text: Strings.Report_a_bug,
                    selection: { [unowned self] in
                        self.settingsDelegate?.settingsOpenURLInNewTab(BraveUX.BraveCommunityURL)
                        self.dismiss(animated: true)
                    },
                    cellClass: MultilineButtonCell.self),
                Row(text: Strings.Rate_Brave,
                    selection: { [unowned self] in
                        // Rate Brave
                        guard let writeReviewURL = URL(string: "https://itunes.apple.com/app/id1052879175?action=write-review")
                            else { return }
                        UIApplication.shared.open(writeReviewURL)
                        self.dismiss(animated: true)
                    },
                    cellClass: MultilineValue1Cell.self),
                Row(text: Strings.Privacy_Policy,
                    selection: { [unowned self] in
                        // Show privacy policy
                        let privacy = SettingsContentViewController().then { $0.url = BraveUX.BravePrivacyURL }
                        self.navigationController?.pushViewController(privacy, animated: true)
                    },
                    accessory: .disclosureIndicator, cellClass: MultilineValue1Cell.self),
                Row(text: Strings.Terms_of_Use,
                    selection: { [unowned self] in
                        // Show terms of use
                        let toc = SettingsContentViewController().then { $0.url = BraveUX.BraveTermsOfUseURL }
                        self.navigationController?.pushViewController(toc, animated: true)
                    },
                    accessory: .disclosureIndicator, cellClass: MultilineValue1Cell.self)
            ]
        )
    }()
    
    private lazy var aboutSection: Section = {
        let version = String(format: Strings.Version_template,
                             Bundle.main.object(forInfoDictionaryKey: "CFBundleShortVersionString") as? String ?? "",
                             Bundle.main.object(forInfoDictionaryKey: "CFBundleVersion") as? String ?? "")
        return Section(
            header: .title(Strings.About),
            rows: [
                Row(text: version, selection: { [unowned self] in
                    let device = UIDevice.current
                    let actionSheet = UIAlertController(title: nil, message: nil, preferredStyle: .actionSheet)
                    let iOSVersion = "\(device.systemName) \(UIDevice.current.systemVersion)"
                    
                    let deviceModel = String(format: Strings.Device_template, device.modelName, iOSVersion)
                    let copyDebugInfoAction = UIAlertAction(title: Strings.Copy_app_info_to_clipboard, style: .default) { _ in
                        UIPasteboard.general.strings = [version, deviceModel]
                    }
                    
                    actionSheet.addAction(copyDebugInfoAction)
                    actionSheet.addAction(UIAlertAction(title: Strings.CancelButtonTitle, style: .cancel, handler: nil))
                    self.navigationController?.present(actionSheet, animated: true, completion: nil)
                }, cellClass: MultilineValue1Cell.self),
                Row(text: Strings.SettingsLicenses, selection: { [unowned self] in
                    guard let url = URL(string: WebServer.sharedInstance.base) else { return }
                    
                    let licenses = SettingsContentViewController().then {
                        $0.url = url.appendingPathComponent("about").appendingPathComponent("license")
                    }
                    self.navigationController?.pushViewController(licenses, animated: true)
                    }, accessory: .disclosureIndicator)
            ]
        )
    }()
    
    private lazy var debugSection: Section? = {
        if AppConstants.BuildChannel.isPublic { return nil }
        
        return Section(
            rows: [
                Row(text: "Region: \(Locale.current.regionCode ?? "--")"),
                Row(text: "Adblock Debug", selection: { [weak self] in
                    let vc = AdblockDebugMenuTableViewController(style: .grouped)
                    self?.navigationController?.pushViewController(vc, animated: true)
                }, accessory: .disclosureIndicator, cellClass: MultilineValue1Cell.self),
                Row(text: "View URP Logs", selection: {
                    self.navigationController?.pushViewController(UrpLogsViewController(), animated: true)
                }, accessory: .disclosureIndicator, cellClass: MultilineValue1Cell.self),
                Row(text: "URP Code: \(UserReferralProgram.getReferralCode() ?? "--")"),
                Row(text: "View Rewards Debug Menu", selection: {
                    self.displayRewardsDebugMenu()
                }, accessory: .disclosureIndicator, cellClass: MultilineValue1Cell.self),
                Row(text: "Load all QA Links", selection: {
                    let url = URL(string: "https://raw.githubusercontent.com/brave/qa-resources/master/testlinks.json")!
                    let string = try? String(contentsOf: url)
                    let urls = JSON(parseJSON: string!)["links"].arrayValue.compactMap { URL(string: $0.stringValue) }
                    self.settingsDelegate?.settingsOpenURLs(urls)
                    self.dismiss(animated: true)
                }, cellClass: MultilineButtonCell.self),
                Row(text: "CRASH!!!", selection: {
                    let alert = UIAlertController(title: "Force crash?", message: nil, preferredStyle: .alert)
                    alert.addAction(UIAlertAction(title: "Crash app", style: .destructive) { _ in
                        fatalError()
                    })
                    alert.addAction(UIAlertAction(title: "Cancel", style: .cancel, handler: nil))
                    self.present(alert, animated: true, completion: nil)
                }, cellClass: MultilineButtonCell.self)
            ]
        )
    }()
    
    func toggleSwitch(on: Bool, section: Section, rowUUID: String) {
        if let sectionRow: Row = section.rows.first(where: {$0.uuid == rowUUID}) {
            if let switchView: UISwitch = sectionRow.accessory.view as? UISwitch {
                switchView.setOn(on, animated: true)
            }
        }
    }
}

extension TableViewController: Themeable {
    func applyTheme(_ theme: Theme) {
        styleChildren(theme: theme)
        tableView.reloadData()

        //  View manipulations done via `apperance()` do not impact existing UI, so need to adjust manually
        // exiting menus, so setting explicitly.
        navigationController?.navigationBar.tintColor = UINavigationBar.appearance().tintColor
        navigationController?.navigationBar.barTintColor = UINavigationBar.appearance().appearanceBarTintColor
    }
}
