/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import Shared
import BraveShared
import Deferred

private let SectionToggles = 0
private let SectionButton = 1
private let NumberOfSections = 2
private let SectionHeaderFooterIdentifier = "SectionHeaderFooterIdentifier"

private let log = Logger.browserLogger

private let HistoryClearableIndex = 0

class ClearPrivateDataTableViewController: UITableViewController {
    fileprivate var clearButton: UITableViewCell?
    
    var profile: Profile!
    var tabManager: TabManager!
    
    fileprivate var gotNotificationDeathOfAllWebViews = false
    
    fileprivate typealias DefaultCheckedState = Bool
    
    fileprivate lazy var clearables: [(clearable: Clearable, checked: DefaultCheckedState)] = {
        return [
            (HistoryClearable(), true),
            (CacheClearable(), true),
            (CookiesAndCacheClearable(), true),
            (PasswordsClearable(profile: self.profile), true),
            ]
    }()
    
    fileprivate lazy var toggles: [Bool] = {
        let savedToggles = Preferences.Privacy.clearPrivateDataToggles.value
        // Ensure if we ever add an option to the list of clearables we don't crash
        if savedToggles.count == clearables.count {
            return savedToggles
        }
        
        return self.clearables.map { $0.checked }
    }()
    
    fileprivate var clearButtonEnabled = true {
        didSet {
            clearButton?.textLabel?.textColor = clearButtonEnabled ? UIConstants.DestructiveRed : UIColor.lightGray
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
        
        title = Strings.ClearPrivateData
        
        tableView.register(SettingsTableSectionHeaderFooterView.self, forHeaderFooterViewReuseIdentifier: SectionHeaderFooterIdentifier)
        
        tableView.separatorColor = UIConstants.TableViewSeparatorColor
        tableView.backgroundColor = UIConstants.TableViewHeaderBackgroundColor
    }
    
    override func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
        let cell = UITableViewCell(style: .default, reuseIdentifier: nil)
        
        if indexPath.section == SectionToggles {
            cell.textLabel?.text = clearables[indexPath.item].clearable.label
            let control = UISwitch()
            control.onTintColor = UIConstants.ControlTintColor
            control.addTarget(self, action: #selector(switchValueChanged(_:)), for: .valueChanged)
            control.isOn = toggles[indexPath.item]
            cell.accessoryView = control
            cell.selectionStyle = .none
            control.tag = indexPath.item
        } else {
            assert(indexPath.section == SectionButton)
            cell.textLabel?.text = Strings.ClearPrivateData
            cell.textLabel?.textAlignment = .center
            cell.textLabel?.textColor = UIConstants.DestructiveRed
            cell.accessibilityTraits = UIAccessibilityTraits.button
            cell.accessibilityIdentifier = "ClearPrivateData"
            clearButton = cell
        }
        
        return cell
    }
    
    override func numberOfSections(in tableView: UITableView) -> Int {
        return NumberOfSections
    }
    
    override func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
        if section == SectionToggles {
            return clearables.count
        }
        
        assert(section == SectionButton)
        return 1
    }
    
    override func tableView(_ tableView: UITableView, shouldHighlightRowAt indexPath: IndexPath) -> Bool {
        guard indexPath.section == SectionButton else { return false }
        
        // Highlight the button only if it's enabled.
        return clearButtonEnabled
    }
    
    static func clearPrivateData(_ clearables: [Clearable], secondAttempt: Bool = false) -> Deferred<Void> {
        let deferred = Deferred<Void>()
        
        clearables.enumerated().map { clearable in
            log.info("Clearing \(clearable.element).")
            
            let res = Success()
            succeed().upon() { _ in // move off main thread
                clearable.element.clear().upon() { result in
                    res.fill(result)
                }
            }
            return res
            }
            .allSucceed()
            .upon { result in
                if !result.isSuccess && !secondAttempt {
                    log.error("Private data NOT cleared successfully")
                    DispatchQueue.main.asyncAfter(deadline: .now() + 0.5, execute: {
                        // For some reason, a second attempt seems to always succeed
                        clearPrivateData(clearables, secondAttempt: true).upon() { _ in
                            deferred.fill(())
                        }
                    })
                    return
                }
                
                if !result.isSuccess {
                    log.error("Private data NOT cleared after 2 attempts")
                }
                deferred.fill(())
        }
        return deferred
    }
    
    @objc fileprivate func allWebViewsKilled() {
        gotNotificationDeathOfAllWebViews = true
        
        DispatchQueue.main.asyncAfter(deadline: .now() + 0.5, execute: { // for some reason, even after all webviews killed, an big delay is needed before the filehandles are unlocked
            var clear = [Clearable]()
            for i in 0..<self.clearables.count where i < self.toggles.count && self.toggles[i] {
                clear.append(self.clearables[i].clearable)
            }
            
            // TODO: Bring back PrivateBrowsing
            /*
             if PrivateBrowsing.singleton.isOn {
             PrivateBrowsing.singleton.exit().upon {
             ClearPrivateDataTableViewController.clearPrivateData(clear).upon {
             DispatchQueue.main.asyncAfter(deadline: .now() + 0.1, execute: {
             PrivateBrowsing.singleton.enter()
             getApp().tabManager.addTabAndSelect()
             })
             }
             }
             } else {*/
            
            // Reset Webkit configuration to remove data from memory
            if clear.contains(where: { $0 is CookiesAndCacheClearable || $0 is CacheClearable }) {
                self.tabManager.resetConfiguration()
                // Unlock the folders to allow clearing of data.
                if Preferences.Privacy.blockAllCookies.value {
                    FileManager.default.setFolderAccess([
                        (.cookie, false),
                        (.webSiteData, false)
                        ])
                }
            }
            ClearPrivateDataTableViewController.clearPrivateData(clear).uponQueue(DispatchQueue.main) { _ in
                // TODO: add API to avoid add/remove
                //Lock the local storage back if mismatch.
                if Preferences.Privacy.blockAllCookies.value, !FileManager.default.checkLockedStatus(folder: .cookie) {
                    FileManager.default.setFolderAccess([
                        (.cookie, true),
                        (.webSiteData, true)
                        ])
                }
                self.tabManager.removeTab(self.tabManager.addTab())
            }
        })
    }
    
    override func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
        guard indexPath.section == SectionButton else { return }
        
        tableView.deselectRow(at: indexPath, animated: false)
        
        let actionSheet = UIAlertController(title: nil, message: nil, preferredStyle: .actionSheet)
        let clearAction = UIAlertAction(title: Strings.ClearPrivateData, style: .destructive) { (_) in
            Preferences.Privacy.clearPrivateDataToggles.value = self.toggles
            self.clearButtonEnabled = false
            self.tabManager.removeAll()
            DispatchQueue.main.asyncAfter(deadline: .now() + 0.5, execute: {
                if !self.gotNotificationDeathOfAllWebViews {
                    self.allWebViewsKilled()
                }
            })
        }
        actionSheet.addAction(clearAction)
        actionSheet.addAction(.init(title: Strings.CancelButtonTitle, style: .cancel, handler: nil))
        present(actionSheet, animated: true, completion: nil)
    }
    
    @objc func switchValueChanged(_ toggle: UISwitch) {
        toggles[toggle.tag] = toggle.isOn
        
        // Dim the clear button if no clearables are selected.
        clearButtonEnabled = toggles.contains(true)
    }
}
