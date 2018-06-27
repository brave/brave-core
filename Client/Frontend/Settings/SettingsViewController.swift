/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import Shared
import BraveShared
import Static

extension TabBarVisibility: RepresentableOptionType {
  public var displayString: String {
    switch self {
    case .always: return Strings.Always_show
    case .landscapeOnly: return Strings.Show_in_landscape_only
    case .never: return Strings.Never_show
    }
  }
}

extension HTTPCookie.AcceptPolicy: RepresentableOptionType {
  
  public var displayString: String {
    switch self {
    case .always: return Strings.Block_all_cookies
    case .onlyFromMainDocumentDomain: return Strings.Block_3rd_party_cookies
    case .never: return Strings.Dont_block_cookies
    }
  }
}

/// Just creates a switch toggle `Row` which updates a `Preferences.Option<Bool>`
private func BasicBoolRow(title: String, option: Preferences.Option<Bool>) -> Row {
  return Row(
    text: title,
    accessory: .switchToggle(
      value: option.value,
      { option.value = $0 }
    )
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
}

class SettingsViewController: TableViewController {
  
  weak var settingsDelegate: SettingsDelegate?
  
  private var settings: [Section] = []
  
  private let profile: Profile
  private let tabManager: TabManager
  
  init(profile: Profile, tabManager: TabManager) {
    self.profile = profile
    self.tabManager = tabManager
    
    super.init(style: .grouped)
    
    UITableViewCell.appearance(whenContainedInInstancesOf: [SettingsViewController.self]).tintColor = BraveUX.BraveOrange
  }
  
  @available(*, unavailable)
  required init?(coder aDecoder: NSCoder) {
    fatalError()
  }
  
  override func viewDidLoad() {
    
    navigationItem.title = Strings.Settings
    navigationItem.rightBarButtonItem = UIBarButtonItem(title: Strings.Done, style: .done, target: self, action: #selector(tappedDone))
    navigationItem.rightBarButtonItem?.accessibilityIdentifier = "SettingsViewController.navigationItem.leftBarButtonItem"
    navigationItem.rightBarButtonItem?.tintColor = BraveUX.BraveOrange
    
    tableView.accessibilityIdentifier = "SettingsViewController.tableView"
    tableView.separatorColor = UIConstants.TableViewSeparatorColor
    tableView.backgroundColor = UIConstants.TableViewHeaderBackgroundColor
    
    var general = Section(
      header: .title(Strings.SettingsGeneralSectionTitle),
      rows: [
        Row(text: Strings.DefaultSearchEngine, detailText: Preferences.Search.orderedEngines.value.first, selection: {
          // Show default engines
        }, accessory: .disclosureIndicator),
        BasicBoolRow(title: Strings.SaveLogin, option: Preferences.saveLogins),
        BasicBoolRow(title: Strings.Block_Popups, option: Preferences.blockPopups),
      ]
    )
    
    if UIDevice.current.userInterfaceIdiom == .pad {
      general.rows.append(
        Row(text: Strings.Show_Tabs_Bar, accessory: .switchToggle(value: Preferences.tabBarVisibility.value == TabBarVisibility.always.rawValue, { Preferences.tabBarVisibility.value = $0 ? TabBarVisibility.always.rawValue : TabBarVisibility.never.rawValue }))
      )
    } else {
      let uuid = "general.tab-bar-visiblity"
      general.rows.append(
        Row(text: Strings.Show_Tabs_Bar, detailText: TabBarVisibility(rawValue: Preferences.tabBarVisibility.value)?.displayString, selection: { [unowned self] in
          // Show options for tab bar visibility
          let optionsViewController = OptionSelectionViewController<TabBarVisibility>(
            options: TabBarVisibility.allCases,
            selectedOption: TabBarVisibility(rawValue: Preferences.tabBarVisibility.value),
            optionChanged: { [unowned self] _, option in
              Preferences.tabBarVisibility.value = option.rawValue

              if let indexPath = self.dataSource.indexPath(rowUUID: uuid, sectionUUID: general.uuid) {
                self.dataSource.sections[indexPath.section].rows[indexPath.row].detailText = option.displayString
              }
            }
          )
          optionsViewController.headerText = Strings.Show_Tabs_Bar
          self.navigationController?.pushViewController(optionsViewController, animated: true)
        }, accessory: .disclosureIndicator, uuid: uuid)
      )
    }
    
    let cookieControlUuid = "privacy.cookie-control"
    var privacy = Section(
      header: .title(Strings.Privacy)
    )
    privacy.rows = [
      Row(text: Strings.ClearPrivateData, selection: { [unowned self] in
        // Show Clear private data screen
        let clearPrivateData = ClearPrivateDataTableViewController().then {
          $0.profile = self.profile
          $0.tabManager = self.tabManager
        }
        self.navigationController?.pushViewController(clearPrivateData, animated: true)
      }, accessory: .disclosureIndicator),
      Row(text: Strings.Cookie_Control, detailText: HTTPCookie.AcceptPolicy(rawValue:  Preferences.Privacy.cookieAcceptPolicy.value)?.displayString, selection: {
        // Show Options for cookie control
        let optionsViewController = OptionSelectionViewController<HTTPCookie.AcceptPolicy>(
          options: [.onlyFromMainDocumentDomain, .always, .never],
          selectedOption: HTTPCookie.AcceptPolicy(rawValue:  Preferences.Privacy.cookieAcceptPolicy.value),
          optionChanged: { [unowned self] _, option in
            Preferences.Privacy.cookieAcceptPolicy.value = option.rawValue
            
            if let indexPath = self.dataSource.indexPath(rowUUID: cookieControlUuid, sectionUUID: privacy.uuid) {
              self.dataSource.sections[indexPath.section].rows[indexPath.row].detailText = option.displayString
            }
          }
        )
        optionsViewController.headerText = Strings.Cookie_Control
        self.navigationController?.pushViewController(optionsViewController, animated: true)
      }, accessory: .disclosureIndicator, uuid: cookieControlUuid),
      BasicBoolRow(title: Strings.Private_Browsing_Only, option: Preferences.Privacy.privateBrowsingOnly)
    ]
    
    let security = Section(
      header: .title(Strings.Security),
      rows: [
        Row(text: Strings.Browser_Lock, accessory: .switchToggle(value: Preferences.Security.browserLockEnabled.value, { Preferences.Security.browserLockEnabled.value = $0 })),
        Row(text: Strings.Change_Pin, selection: {
          // Show pin selector
        }, accessory: .disclosureIndicator)
      ]
    )
    
    let shields = Section(
      header: .title(Strings.Brave_Shield_Defaults),
      rows: [
        BasicBoolRow(title: Strings.Block_Ads_and_Tracking, option: Preferences.Shields.blockAdsAndTracking),
        BasicBoolRow(title: Strings.HTTPS_Everywhere, option: Preferences.Shields.httpsEverywhere),
        BasicBoolRow(title: Strings.Block_Phishing_and_Malware, option: Preferences.Shields.blockPhishingAndMalware),
        BasicBoolRow(title: Strings.Block_Scripts, option: Preferences.Shields.blockScripts),
        BasicBoolRow(title: Strings.Fingerprinting_Protection, option: Preferences.Shields.fingerprintingProtection),
      ]
    )
    
    // TODO: Add regional adblock
    // shields.rows.append(BasicBoolRow(title: Strings.Use_regional_adblock, option: Preferences.Shields.useRegionAdBlock))
    
    let support = Section(
      header: .title(Strings.Support),
      rows: [
        BasicBoolRow(title: Strings.Opt_in_to_telemetry, option: Preferences.Support.sendsCrashReportsAndMetrics),
        Row(text: Strings.Report_a_bug, selection: { [unowned self] in
          // Report a bug
          self.settingsDelegate?.settingsOpenURLInNewTab(BraveUX.BraveCommunityURL)
          self.dismiss(animated: true)
        }, cellClass: ButtonCell.self),
        Row(text: Strings.Privacy_Policy, selection: { [unowned self] in
          // Show privacy policy
          let privacy = SettingsContentViewController().then { $0.url = BraveUX.BravePrivacyURL }
          self.navigationController?.pushViewController(privacy, animated: true)
        }, accessory: .disclosureIndicator),
        Row(text: Strings.Terms_of_Use, selection: { [unowned self] in
          // Show terms of use
          let toc = SettingsContentViewController().then { $0.url = BraveUX.BraveTermsOfUseURL }
          self.navigationController?.pushViewController(toc, animated: true)
        }, accessory: .disclosureIndicator)
      ]
    )
    
    let version = String(format: Strings.Version_template, Bundle.main.object(forInfoDictionaryKey: "CFBundleShortVersionString") as? String ?? "", Bundle.main.object(forInfoDictionaryKey: "CFBundleVersion") as? String ?? "")
    
    let about = Section(
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
          actionSheet.addAction(UIAlertAction(title: Strings.Cancel, style: .cancel, handler: nil))
          self.navigationController?.present(actionSheet, animated: true, completion: nil)
        })
      ]
    )
    
    dataSource.sections = [
      general,
      privacy,
      security,
      shields,
      support,
      about
    ]
  }
  
  @objc private func tappedDone() {
    dismiss(animated: true)
  }
}
