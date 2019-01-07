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

extension TabBarVisibility: RepresentableOptionType {
    public var displayString: String {
        switch self {
        case .always: return Strings.Always_show
        case .landscapeOnly: return Strings.Show_in_landscape_only
        case .never: return Strings.Never_show
        }
    }
}

/// Just creates a switch toggle `Row` which updates a `Preferences.Option<Bool>`
private func BoolRow(title: String, option: Preferences.Option<Bool>) -> Row {
    return Row(
        text: title,
        accessory: .switchToggle(
            value: option.value, { option.value = $0 }
        ),
        uuid: option.key
    )
}

extension DataSource {
    /// Get the index path of a Row to modify it
    ///
    /// Since they are structs we cannot obtain references to them to alter them, we must directly access them
    /// from `sections[x].rows[y]`
    func indexPath(rowUUID: String, sectionUUID: String) -> IndexPath? {
        guard let section = sections.index(where: { $0.uuid == sectionUUID }),
            let row = sections[section].rows.index(where: { $0.uuid == rowUUID }) else {
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
    
    init(profile: Profile, tabManager: TabManager) {
        self.profile = profile
        self.tabManager = tabManager
        
        super.init(style: .grouped)
        
        UITableViewCell.appearance().tintColor = BraveUX.BraveOrange
    }
    
    @available(*, unavailable)
    required init?(coder aDecoder: NSCoder) {
        fatalError()
    }
    
    override func viewDidLoad() {
        
        navigationItem.title = Strings.Settings
        navigationItem.rightBarButtonItem = UIBarButtonItem(title: Strings.Done, style: .done, target: self, action: #selector(tappedDone))
        navigationItem.rightBarButtonItem?.accessibilityIdentifier = "SettingsViewController.navigationItem.rightBarButtonItem"
        navigationItem.rightBarButtonItem?.tintColor = BraveUX.BraveOrange
        
        tableView.accessibilityIdentifier = "SettingsViewController.tableView"
        tableView.separatorColor = UIConstants.TableViewSeparatorColor
        tableView.backgroundColor = UIConstants.TableViewHeaderBackgroundColor

        dataSource.sections = sections
    }
    
    private var sections: [Section] {
        var list = [Section]()
        list.append(generalSection)
        #if !NO_SYNC
            list.append(syncSection)
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
    
    @objc private func tappedDone() {
        settingsDelegate?.settingsDidFinish(self)
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
                }, accessory: .disclosureIndicator),
                BoolRow(title: Strings.Save_Logins, option: Preferences.General.saveLogins),
                BoolRow(title: Strings.Block_Popups, option: Preferences.General.blockPopups),
            ]
        )
        
        if UIDevice.current.userInterfaceIdiom == .pad {
            general.rows.append(
                Row(text: Strings.Show_Tabs_Bar, accessory: .switchToggle(value: Preferences.General.tabBarVisibility.value == TabBarVisibility.always.rawValue, { Preferences.General.tabBarVisibility.value = $0 ? TabBarVisibility.always.rawValue : TabBarVisibility.never.rawValue }))
            )
        } else {
            var row = Row(text: Strings.Show_Tabs_Bar, detailText: TabBarVisibility(rawValue: Preferences.General.tabBarVisibility.value)?.displayString, accessory: .disclosureIndicator)
            row.selection = { [unowned self] in
                // Show options for tab bar visibility
                let optionsViewController = OptionSelectionViewController<TabBarVisibility>(
                    options: TabBarVisibility.allCases,
                    selectedOption: TabBarVisibility(rawValue: Preferences.General.tabBarVisibility.value),
                    optionChanged: { [unowned self] _, option in
                        Preferences.General.tabBarVisibility.value = option.rawValue
                        
                        if let indexPath = self.dataSource.indexPath(rowUUID: row.uuid, sectionUUID: general.uuid) {
                            self.dataSource.sections[indexPath.section].rows[indexPath.row].detailText = option.displayString
                        }
                    }
                )
                optionsViewController.headerText = Strings.Show_Tabs_Bar
                self.navigationController?.pushViewController(optionsViewController, animated: true)
            }
            general.rows.append(row)
        }
        
        return general
    }()
    
    private lazy var syncSection: Section = {
        
        return Section(
            // BRAVE TODO: Change it once we finalize our decision how to name the section.(#385)
            header: .title("Other Settings"),
            rows: [
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
                }, accessory: .disclosureIndicator)
            ]
        )
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
                accessory: .disclosureIndicator
            )
        ]
        privacy.rows.append(BoolRow(title: Strings.Private_Browsing_Only, option: Preferences.Privacy.privateBrowsingOnly))
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
                    }, accessory: .disclosureIndicator)
            ]
        )
    }()
    
    private lazy var shieldsSection: Section = {
        return Section(
            header: .title(Strings.Brave_Shield_Defaults),
            rows: [
                BoolRow(title: Strings.Block_Ads_and_Tracking, option: Preferences.Shields.blockAdsAndTracking),
                BoolRow(title: Strings.HTTPS_Everywhere, option: Preferences.Shields.httpsEverywhere),
                BoolRow(title: Strings.Block_Phishing_and_Malware, option: Preferences.Shields.blockPhishingAndMalware),
                BoolRow(title: Strings.Block_Scripts, option: Preferences.Shields.blockScripts),
                BoolRow(title: Strings.Fingerprinting_Protection, option: Preferences.Shields.fingerprintingProtection),
            ]
        )
        // TODO: Add regional adblock
        // shields.rows.append(BasicBoolRow(title: Strings.Use_regional_adblock, option: Preferences.Shields.useRegionAdBlock))
    }()
    
    private lazy var supportSection: Section = {
        return Section(
            header: .title(Strings.Support),
            rows: [
                Row(text: Strings.Report_a_bug,
                    selection: { [unowned self] in
                        // Report a bug
                        self.settingsDelegate?.settingsOpenURLInNewTab(BraveUX.BraveCommunityURL)
                        self.dismiss(animated: true)
                    },
                    cellClass: ButtonCell.self),
                Row(text: Strings.Privacy_Policy,
                    selection: { [unowned self] in
                        // Show privacy policy
                        let privacy = SettingsContentViewController().then { $0.url = BraveUX.BravePrivacyURL }
                        self.navigationController?.pushViewController(privacy, animated: true)
                    },
                    accessory: .disclosureIndicator),
                Row(text: Strings.Terms_of_Use,
                    selection: { [unowned self] in
                        // Show terms of use
                        let toc = SettingsContentViewController().then { $0.url = BraveUX.BraveTermsOfUseURL }
                        self.navigationController?.pushViewController(toc, animated: true)
                    },
                    accessory: .disclosureIndicator)
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
                })
            ]
        )
    }()
    
    private lazy var debugSection: Section? = {
        if AppConstants.BuildChannel.isRelease { return nil }
        
        return Section(
            rows: [
                Row(text: "Region: \(Locale.current.regionCode ?? "--")"),
                Row(text: "Recompile Content Blockers", selection: { [weak self] in
                    BlocklistName.allLists.forEach { $0.fileVersionPref?.value = nil }
                    ContentBlockerHelper.compileLists().upon {
                        let alert = UIAlertController(title: nil, message: "Recompiled Blockers", preferredStyle: .alert)
                        alert.addAction(UIAlertAction(title: "OK", style: .default))
                        self?.present(alert, animated: true)
                    }
                }, cellClass: ButtonCell.self),
                Row(text: "View URP Logs", selection: {
                    self.navigationController?.pushViewController(UrpLogsViewController(), animated: true)
                }, accessory: .disclosureIndicator),
                Row(text: "URP Code: \(UserReferralProgram.getReferralCode() ?? "--")"),
                Row(text: "Load all QA Links", selection: {
                    let url = URL(string: "https://raw.githubusercontent.com/brave/qa-resources/master/testlinks.json")!
                    let string = try? String(contentsOf: url)
                    let urls = JSON(parseJSON: string!)["links"].arrayValue.compactMap { URL(string: $0.stringValue) }
                    self.settingsDelegate?.settingsOpenURLs(urls)
                    self.dismiss(animated: true)
                }, cellClass: ButtonCell.self),
                Row(text: "CRASH!!!", selection: {
                    let alert = UIAlertController(title: "Force crash?", message: nil, preferredStyle: .alert)
                    alert.addAction(UIAlertAction(title: "Crash app", style: .destructive) { _ in
                        fatalError()
                    })
                    alert.addAction(UIAlertAction(title: "Cancel", style: .cancel, handler: nil))
                    self.present(alert, animated: true, completion: nil)
                }, cellClass: ButtonCell.self)
            ]
        )
    }()
}
