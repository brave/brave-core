/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import Shared
import Deferred

private let SectionToggles = 0
private let SectionButton = 1
private let NumberOfSections = 2
private let SectionHeaderFooterIdentifier = "SectionHeaderFooterIdentifier"
private let TogglesPrefKey = "clearprivatedata.toggles"

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
            (CookiesClearable(), true),
            (PasswordsClearable(profile: self.profile), true),
            ]
    }()
    
    fileprivate lazy var toggles: [Bool] = {
        if let savedToggles = self.profile.prefs.arrayForKey(TogglesPrefKey) as? [Bool] {
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
        let cell = UITableViewCell(style: UITableViewCellStyle.default, reuseIdentifier: nil)
        
        if indexPath.section == SectionToggles {
            cell.textLabel?.text = clearables[indexPath.item].clearable.label
            let control = UISwitch()
            control.onTintColor = UIConstants.ControlTintColor
            control.addTarget(self, action: #selector(ClearPrivateDataTableViewController.switchValueChanged(_:)), for: UIControlEvents.valueChanged)
            control.isOn = toggles[indexPath.item]
            cell.accessoryView = control
            cell.selectionStyle = .none
            control.tag = indexPath.item
        } else {
            assert(indexPath.section == SectionButton)
            cell.textLabel?.text = Strings.ClearPrivateData
            cell.textLabel?.textAlignment = NSTextAlignment.center
            cell.textLabel?.textColor = UIConstants.DestructiveRed
            cell.accessibilityTraits = UIAccessibilityTraitButton
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
            print("Clearing \(clearable.element).")
            
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
                    print("Private data NOT cleared successfully")
                    DispatchQueue.main.asyncAfter(deadline: .now() + 0.5, execute: {
                        // For some reason, a second attempt seems to always succeed
                        clearPrivateData(clearables, secondAttempt: true).upon() { _ in
                            deferred.fill(())
                        }
                    })
                    return
                }
                
                if !result.isSuccess {
                    print("Private data NOT cleared after 2 attempts")
                }
                deferred.fill(())
        }
        return deferred
    }
    
    @objc fileprivate func allWebViewsKilled() {
        gotNotificationDeathOfAllWebViews = true
        
        DispatchQueue.main.asyncAfter(deadline: .now() + 0.5, execute: { // for some reason, even after all webviews killed, an big delay is needed before the filehandles are unlocked
            var clear = [Clearable]()
            for i in 0..<self.clearables.count {
                if self.toggles[i] {
                    clear.append(self.clearables[i].clearable)
                }
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
            ClearPrivateDataTableViewController.clearPrivateData(clear).uponQueue(DispatchQueue.main) { _ in
                // TODO: add API to avoid add/remove
                self.tabManager.removeTab(self.tabManager.addTab())
            }
            //      }
        })
    }
    
    override func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
        guard indexPath.section == SectionButton else { return }
        
        tableView.deselectRow(at: indexPath, animated: false)
        
        let actionSheet = UIAlertController(title: nil, message: nil, preferredStyle: .actionSheet)
        let clearAction = UIAlertAction(title: Strings.ClearPrivateData, style: .destructive) { (_) in
            self.profile.prefs.setObject(self.toggles, forKey: TogglesPrefKey)
            self.clearButtonEnabled = false
            
            //      NotificationCenter.default.removeObserver(self)
            //      NotificationCenter.default.addObserver(self, selector: #selector(self.allWebViewsKilled), name: NSNotification.Name(rawValue: kNotificationAllWebViewsDeallocated), object: nil)
            
            self.tabManager.removeAll()
            DispatchQueue.main.asyncAfter(deadline: .now() + 0.5, execute: {
                if !self.gotNotificationDeathOfAllWebViews {
                    self.tabManager.tabs.forEach { $0.deleteWebView() }
                    self.allWebViewsKilled()
                }
            })
        }
        actionSheet.addAction(clearAction)
        actionSheet.addAction(.init(title: Strings.Cancel, style: .cancel, handler: nil))
        present(actionSheet, animated: true, completion: nil)
    }
    
    @objc func switchValueChanged(_ toggle: UISwitch) {
        toggles[toggle.tag] = toggle.isOn
        
        // Dim the clear button if no clearables are selected.
        clearButtonEnabled = toggles.contains(true)
    }
}
