/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import Shared
import SwiftKeychainWrapper
import LocalAuthentication

// This file contains all of the settings available in the main settings screen of the app.

private var ShowDebugSettings: Bool = false
private var DebugSettingsClickCount: Int = 0

// For great debugging!
class HiddenSetting: Setting {
    unowned let settings: SettingsTableViewController

    init(settings: SettingsTableViewController) {
        self.settings = settings
        super.init(title: nil)
    }

    override var hidden: Bool {
        return !ShowDebugSettings
    }
}

class DeleteExportedDataSetting: HiddenSetting {
    override var title: NSAttributedString? {
        // Not localized for now.
        return NSAttributedString(string: "Debug: delete exported databases", attributes: [NSAttributedStringKey.foregroundColor: SettingsUX.TableViewRowTextColor])
    }

    override func onClick(_ navigationController: UINavigationController?) {
        let documentsPath = NSSearchPathForDirectoriesInDomains(.documentDirectory, .userDomainMask, true)[0]
        let fileManager = FileManager.default
        do {
            let files = try fileManager.contentsOfDirectory(atPath: documentsPath)
            for file in files {
                if file.hasPrefix("browser.") || file.hasPrefix("logins.") {
                    try fileManager.removeItemInDirectory(documentsPath, named: file)
                }
            }
        } catch {
            print("Couldn't delete exported data: \(error).")
        }
    }
}

class ExportBrowserDataSetting: HiddenSetting {
    override var title: NSAttributedString? {
        // Not localized for now.
        return NSAttributedString(string: "Debug: copy databases to app container", attributes: [NSAttributedStringKey.foregroundColor: SettingsUX.TableViewRowTextColor])
    }

    override func onClick(_ navigationController: UINavigationController?) {
        let documentsPath = NSSearchPathForDirectoriesInDomains(.documentDirectory, .userDomainMask, true)[0]
        do {
            let log = Logger.syncLogger
            try self.settings.profile.files.copyMatching(fromRelativeDirectory: "", toAbsoluteDirectory: documentsPath) { file in
                log.debug("Matcher: \(file)")
                return file.hasPrefix("browser.") || file.hasPrefix("logins.") || file.hasPrefix("metadata.")
            }
        } catch {
            print("Couldn't export browser data: \(error).")
        }
    }
}

/*
 FeatureSwitchSetting is a boolean switch for features that are enabled via a FeatureSwitch.
 These are usually features behind a partial release and not features released to the entire population.
 */
class FeatureSwitchSetting: BoolSetting {
    let featureSwitch: FeatureSwitch
    let prefs: Prefs

    init(prefs: Prefs, featureSwitch: FeatureSwitch, with title: NSAttributedString) {
        self.featureSwitch = featureSwitch
        self.prefs = prefs
        super.init(prefs: prefs, defaultValue: featureSwitch.isMember(prefs), attributedTitleText: title)
    }

    override var hidden: Bool {
        return !ShowDebugSettings
    }

    override func displayBool(_ control: UISwitch) {
        control.isOn = featureSwitch.isMember(prefs)
    }

    override func writeBool(_ control: UISwitch) {
        self.featureSwitch.setMembership(control.isOn, for: self.prefs)
    }

}

class EnableBookmarkMergingSetting: HiddenSetting {
    override var title: NSAttributedString? {
        // Not localized for now.
        return NSAttributedString(string: "Enable Bidirectional Bookmark Sync ", attributes: [NSAttributedStringKey.foregroundColor: SettingsUX.TableViewRowTextColor])
    }

    override func onClick(_ navigationController: UINavigationController?) {
        AppConstants.shouldMergeBookmarks = true
    }
}

class ForceCrashSetting: HiddenSetting {
    override var title: NSAttributedString? {
        return NSAttributedString(string: "Debug: Force Crash", attributes: [NSAttributedStringKey.foregroundColor: SettingsUX.TableViewRowTextColor])
    }

    override func onClick(_ navigationController: UINavigationController?) {
        Sentry.shared.crash()
    }
}

// Show the current version of Firefox
class VersionSetting: Setting {
    unowned let settings: SettingsTableViewController

     override var accessibilityIdentifier: String? { return "FxVersion" }

    init(settings: SettingsTableViewController) {
        self.settings = settings
        super.init(title: nil)
    }

    override var title: NSAttributedString? {
        let appVersion = Bundle.main.object(forInfoDictionaryKey: "CFBundleShortVersionString") as! String
        let buildNumber = Bundle.main.object(forInfoDictionaryKey: "CFBundleVersion") as! String
        return NSAttributedString(string: String(format: NSLocalizedString("Version %@ (%@)", comment: "Version number of Firefox shown in settings"), appVersion, buildNumber), attributes: [NSAttributedStringKey.foregroundColor: SettingsUX.TableViewRowTextColor])
    }

    override func onConfigureCell(_ cell: UITableViewCell) {
        super.onConfigureCell(cell)
        cell.selectionStyle = .none
    }

    override func onClick(_ navigationController: UINavigationController?) {
        DebugSettingsClickCount += 1
        if DebugSettingsClickCount >= 5 {
            DebugSettingsClickCount = 0
            ShowDebugSettings = !ShowDebugSettings
            settings.tableView.reloadData()
        }
    }
}

// Opens the the license page in a new tab
class LicenseAndAcknowledgementsSetting: Setting {
    override var title: NSAttributedString? {
        return NSAttributedString(string: NSLocalizedString("Licenses", comment: "Settings item that opens a tab containing the licenses. See http://mzl.la/1NSAWCG"), attributes: [NSAttributedStringKey.foregroundColor: SettingsUX.TableViewRowTextColor])
    }

    override var url: URL? {
        return URL(string: WebServer.sharedInstance.URLForResource("license", module: "about"))
    }

    override func onClick(_ navigationController: UINavigationController?) {
        setUpAndPushSettingsContentViewController(navigationController)
    }
}

// Opens about:rights page in the content view controller
class YourRightsSetting: Setting {
    override var title: NSAttributedString? {
        return NSAttributedString(string: NSLocalizedString("Your Rights", comment: "Your Rights settings section title"), attributes:
            [NSAttributedStringKey.foregroundColor: SettingsUX.TableViewRowTextColor])
    }

    override var url: URL? {
        return URL(string: "https://www.mozilla.org/about/legal/terms/firefox/")
    }

    override func onClick(_ navigationController: UINavigationController?) {
        setUpAndPushSettingsContentViewController(navigationController)
    }
}

// Opens the on-boarding screen again
class ShowIntroductionSetting: Setting {
    let profile: Profile

    override var accessibilityIdentifier: String? { return "ShowTour" }

    init(settings: SettingsTableViewController) {
        self.profile = settings.profile
        super.init(title: NSAttributedString(string: NSLocalizedString("Show Tour", comment: "Show the on-boarding screen again from the settings"), attributes: [NSAttributedStringKey.foregroundColor: SettingsUX.TableViewRowTextColor]))
    }

    override func onClick(_ navigationController: UINavigationController?) {
        navigationController?.dismiss(animated: true)
    }
}

class SendFeedbackSetting: Setting {
    override var title: NSAttributedString? {
        return NSAttributedString(string: NSLocalizedString("Send Feedback", comment: "Menu item in settings used to open input.mozilla.org where people can submit feedback"), attributes: [NSAttributedStringKey.foregroundColor: SettingsUX.TableViewRowTextColor])
    }

    override var url: URL? {
        let appVersion = Bundle.main.object(forInfoDictionaryKey: "CFBundleShortVersionString") as! String
        return URL(string: "https://input.mozilla.org/feedback/fxios/\(appVersion)")
    }

    override func onClick(_ navigationController: UINavigationController?) {
        setUpAndPushSettingsContentViewController(navigationController)
    }
}

class SendAnonymousUsageDataSetting: BoolSetting {
    init(prefs: Prefs, delegate: SettingsDelegate?) {
        let statusText = NSMutableAttributedString()
        statusText.append(NSAttributedString(string: Strings.SendUsageSettingMessage, attributes: [NSAttributedStringKey.foregroundColor: SettingsUX.TableViewHeaderTextColor]))
        statusText.append(NSAttributedString(string: " "))
        statusText.append(NSAttributedString(string: Strings.SendUsageSettingLink, attributes: [NSAttributedStringKey.foregroundColor: UIConstants.HighlightBlue]))

        super.init(
            prefs: prefs, prefKey: AppConstants.PrefSendUsageData, defaultValue: true,
            attributedTitleText: NSAttributedString(string: Strings.SendUsageSettingTitle),
            attributedStatusText: statusText,
            settingDidChange: {
                AdjustIntegration.setEnabled($0)
            }
        )
    }

    override var url: URL? {
        return SupportUtils.URLForTopic("adjust")
    }

    override func onClick(_ navigationController: UINavigationController?) {
        setUpAndPushSettingsContentViewController(navigationController)
    }
}

// Opens the the SUMO page in a new tab
class OpenSupportPageSetting: Setting {
    init(delegate: SettingsDelegate?) {
        super.init(title: NSAttributedString(string: NSLocalizedString("Help", comment: "Show the SUMO support page from the Support section in the settings. see http://mzl.la/1dmM8tZ"), attributes: [NSAttributedStringKey.foregroundColor: SettingsUX.TableViewRowTextColor]),
            delegate: delegate)
    }

    override func onClick(_ navigationController: UINavigationController?) {
        navigationController?.dismiss(animated: true) {
            if let url = URL(string: "https://support.mozilla.org/products/ios") {
                self.delegate?.settingsOpenURLInNewTab(url)
            }
        }
    }
}

// Opens the search settings pane
class SearchSetting: Setting {
    let profile: Profile

    override var accessoryType: UITableViewCellAccessoryType { return .disclosureIndicator }

    override var style: UITableViewCellStyle { return .value1 }

    override var status: NSAttributedString { return NSAttributedString(string: profile.searchEngines.defaultEngine.shortName) }

    override var accessibilityIdentifier: String? { return "Search" }

    init(settings: SettingsTableViewController) {
        self.profile = settings.profile
        super.init(title: NSAttributedString(string: NSLocalizedString("Settings.Open.Search", value: "Search", comment: "Open search section of settings"), attributes: [NSAttributedStringKey.foregroundColor: SettingsUX.TableViewRowTextColor]))
    }

    override func onClick(_ navigationController: UINavigationController?) {
        let viewController = SearchSettingsTableViewController()
        viewController.model = profile.searchEngines
        viewController.profile = profile
        navigationController?.pushViewController(viewController, animated: true)
    }
}

class LoginsSetting: Setting {
    let profile: Profile
    var tabManager: TabManager!
    weak var navigationController: UINavigationController?
    weak var settings: AppSettingsTableViewController?

    override var accessoryType: UITableViewCellAccessoryType { return .disclosureIndicator }

    override var accessibilityIdentifier: String? { return "Logins" }

    init(settings: SettingsTableViewController, delegate: SettingsDelegate?) {
        self.profile = settings.profile
        self.tabManager = settings.tabManager
        self.navigationController = settings.navigationController
        self.settings = settings as? AppSettingsTableViewController

        let loginsTitle = NSLocalizedString("Logins", comment: "Label used as an item in Settings. When touched, the user will be navigated to the Logins/Password manager.")
        super.init(title: NSAttributedString(string: loginsTitle, attributes: [NSAttributedStringKey.foregroundColor: SettingsUX.TableViewRowTextColor]),
                   delegate: delegate)
    }

    func deselectRow () {
        if let selectedRow = self.settings?.tableView.indexPathForSelectedRow {
            self.settings?.tableView.deselectRow(at: selectedRow, animated: true)
        }
    }

    override func onClick(_: UINavigationController?) {
        guard let authInfo = KeychainWrapper.sharedAppContainerKeychain.authenticationInfo() else {
            settings?.navigateToLoginsList()
            return
        }

        if authInfo.requiresValidation() {
            AppAuthenticator.presentAuthenticationUsingInfo(authInfo,
            touchIDReason: AuthenticationStrings.loginsTouchReason,
            success: {
                self.settings?.navigateToLoginsList()
            },
            cancel: {
                self.deselectRow()
            },
            fallback: {
                AppAuthenticator.presentPasscodeAuthentication(self.navigationController, delegate: self.settings)
                self.deselectRow()
            })
        } else {
            settings?.navigateToLoginsList()
        }
    }
}

class TouchIDPasscodeSetting: Setting {
    let profile: Profile
    var tabManager: TabManager!

    override var accessoryType: UITableViewCellAccessoryType { return .disclosureIndicator }

    override var accessibilityIdentifier: String? { return "TouchIDPasscode" }

    init(settings: SettingsTableViewController, delegate: SettingsDelegate? = nil) {
        self.profile = settings.profile
        self.tabManager = settings.tabManager
        let localAuthContext = LAContext()

        let title: String
        if localAuthContext.canEvaluatePolicy(.deviceOwnerAuthenticationWithBiometrics, error: nil) {
            if #available(iOS 11.0, *), localAuthContext.biometryType == .faceID {
                title = AuthenticationStrings.faceIDPasscodeSetting
            } else {
                title = AuthenticationStrings.touchIDPasscodeSetting
            }
        } else {
            title = AuthenticationStrings.passcode
        }
        super.init(title: NSAttributedString(string: title, attributes: [NSAttributedStringKey.foregroundColor: SettingsUX.TableViewRowTextColor]),
                   delegate: delegate)
    }

    override func onClick(_ navigationController: UINavigationController?) {
        let viewController = AuthenticationSettingsViewController()
        viewController.profile = profile
        navigationController?.pushViewController(viewController, animated: true)
    }
}

@available(iOS 11, *)
class ContentBlockerSetting: Setting {
    let profile: Profile
    var tabManager: TabManager!
    override var accessoryType: UITableViewCellAccessoryType { return .disclosureIndicator }
    override var accessibilityIdentifier: String? { return "TrackingProtection" }

    init(settings: SettingsTableViewController) {
        self.profile = settings.profile
        self.tabManager = settings.tabManager
        super.init(title: NSAttributedString(string: Strings.SettingsTrackingProtectionSectionName, attributes: [NSAttributedStringKey.foregroundColor: SettingsUX.TableViewRowTextColor]))
    }

    override func onClick(_ navigationController: UINavigationController?) {
        let viewController = ContentBlockerSettingViewController(prefs: profile.prefs)
        viewController.profile = profile
        viewController.tabManager = tabManager
        navigationController?.pushViewController(viewController, animated: true)
    }
}

class ClearPrivateDataSetting: Setting {
    let profile: Profile
    var tabManager: TabManager!

    override var accessoryType: UITableViewCellAccessoryType { return .disclosureIndicator }

    override var accessibilityIdentifier: String? { return "ClearPrivateData" }

    init(settings: SettingsTableViewController) {
        self.profile = settings.profile
        self.tabManager = settings.tabManager

        let clearTitle = Strings.SettingsClearPrivateDataSectionName
        super.init(title: NSAttributedString(string: clearTitle, attributes: [NSAttributedStringKey.foregroundColor: SettingsUX.TableViewRowTextColor]))
    }

    override func onClick(_ navigationController: UINavigationController?) {
        let viewController = ClearPrivateDataTableViewController()
        viewController.profile = profile
        viewController.tabManager = tabManager
        navigationController?.pushViewController(viewController, animated: true)
    }
}

class PrivacyPolicySetting: Setting {
    override var title: NSAttributedString? {
        return NSAttributedString(string: NSLocalizedString("Privacy Policy", comment: "Show Firefox Browser Privacy Policy page from the Privacy section in the settings. See https://www.mozilla.org/privacy/firefox/"), attributes: [NSAttributedStringKey.foregroundColor: SettingsUX.TableViewRowTextColor])
    }

    override var url: URL? {
        return URL(string: "https://www.mozilla.org/privacy/firefox/")
    }

    override func onClick(_ navigationController: UINavigationController?) {
        setUpAndPushSettingsContentViewController(navigationController)
    }
}

class HomePageSetting: Setting {
    let profile: Profile
    var tabManager: TabManager!

    override var accessoryType: UITableViewCellAccessoryType { return .disclosureIndicator }

    override var accessibilityIdentifier: String? { return "Homepage" }

    init(settings: SettingsTableViewController) {
        self.profile = settings.profile
        self.tabManager = settings.tabManager

        super.init(title: NSAttributedString(string: Strings.SettingsHomePageSectionName, attributes: [NSAttributedStringKey.foregroundColor: SettingsUX.TableViewRowTextColor]))
    }

    override func onClick(_ navigationController: UINavigationController?) {
        let viewController = HomePageSettingsViewController()
        viewController.profile = profile
        viewController.tabManager = tabManager
        navigationController?.pushViewController(viewController, animated: true)
    }
}

class NewTabPageSetting: Setting {
    let profile: Profile

    override var accessoryType: UITableViewCellAccessoryType { return .disclosureIndicator }

    override var accessibilityIdentifier: String? { return "NewTab" }

    init(settings: SettingsTableViewController) {
        self.profile = settings.profile

        super.init(title: NSAttributedString(string: Strings.SettingsNewTabSectionName, attributes: [NSAttributedStringKey.foregroundColor: SettingsUX.TableViewRowTextColor]))
    }

    override func onClick(_ navigationController: UINavigationController?) {
        let viewController = NewTabContentSettingsViewController()
        viewController.profile = profile
        navigationController?.pushViewController(viewController, animated: true)
    }
}

class OpenWithSetting: Setting {
    let profile: Profile

    override var accessoryType: UITableViewCellAccessoryType { return .disclosureIndicator }

    override var accessibilityIdentifier: String? { return "OpenWith.Setting" }

    init(settings: SettingsTableViewController) {
        self.profile = settings.profile

        super.init(title: NSAttributedString(string: Strings.SettingsOpenWithSectionName, attributes: [NSAttributedStringKey.foregroundColor: SettingsUX.TableViewRowTextColor]))
    }

    override func onClick(_ navigationController: UINavigationController?) {
        let viewController = OpenWithSettingsViewController(prefs: profile.prefs)
        navigationController?.pushViewController(viewController, animated: true)
    }
}
