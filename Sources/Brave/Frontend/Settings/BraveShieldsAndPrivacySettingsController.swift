// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Static
import Shared
import Preferences
import BraveCore
import class SwiftUI.UIHostingController
import UIKit
import BraveUI
import BraveNews
import os.log
import Data
import Combine
import Growth

class BraveShieldsAndPrivacySettingsController: TableViewController {
  let profile: Profile
  let tabManager: TabManager
  let feedDataSource: FeedDataSource
  let historyAPI: BraveHistoryAPI
  let p3aUtilities: BraveP3AUtils

  private let cookieConsentNoticesRowUUID = UUID()
  private let blockSwitchToAppNoticesRowUUID = UUID()
  private var cancellables: Set<AnyCancellable> = []

  init(
    profile: Profile,
    tabManager: TabManager,
    feedDataSource: FeedDataSource,
    historyAPI: BraveHistoryAPI,
    p3aUtilities: BraveP3AUtils
  ) {
    self.profile = profile
    self.tabManager = tabManager
    self.feedDataSource = feedDataSource
    self.historyAPI = historyAPI
    self.p3aUtilities = p3aUtilities
    super.init(style: .insetGrouped)
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }

  override func viewDidLoad() {
    super.viewDidLoad()

    title = Strings.braveShieldsAndPrivacy

    dataSource.sections = [
      shieldsSection,
      clearPrivateDataSection,
      manageWebsiteDataSection,
      otherSettingsSection,
    ]
    
    // Listen to changes on filter lists so we know we need to reload the section
    FilterListStorage.shared.$filterLists
      .sink { filterLists in
        Task { @MainActor in
          // Check if we need to update the cookie consent switch
          if let filterList = filterLists.first(where: { filterList in
            filterList.entry.componentId == FilterList.cookieConsentNoticesComponentID
          }) {
            // Toggle only if it's needed or an endless loop might be created
            self.toggleSwitchIfNeeded(
              on: filterList.isEnabled, section: self.shieldsSection,
              rowUUID: self.cookieConsentNoticesRowUUID.uuidString
            )
          }
          
          // Check if we need to update the block mobile notifications switch
          if let filterList = filterLists.first(where: { filterList in
            filterList.entry.componentId == FilterList.mobileAnnoyancesComponentID
          }) {
            // Toggle only if it's needed or an endless loop might be created
            self.toggleSwitchIfNeeded(
              on: filterList.isEnabled, section: self.otherSettingsSection,
              rowUUID: self.blockSwitchToAppNoticesRowUUID.uuidString
            )
          }
        }
      }
      .store(in: &cancellables)
    
    Preferences.Shields.blockAdsAndTracking.$value
      .sink { [weak self] _ in
        self?.recordGlobalAdBlockShieldsP3A()
      }
      .store(in: &cancellables)
    
    Preferences.Shields.fingerprintingProtection.$value
      .sink { [weak self] _ in
        self?.recordGlobalFingerprintingShieldsP3A()
      }
      .store(in: &cancellables)
  }
  
  // MARK: - P3A
  
  private func recordGlobalAdBlockShieldsP3A() {
    // Q46 What is the global ad blocking shields setting?
    enum Answer: Int, CaseIterable {
      case disabled = 0
      case standard = 1
      case aggressive = 2
    }
    let answer: Answer = Preferences.Shields.blockAdsAndTracking.value ? .standard : .disabled
    UmaHistogramEnumeration("Brave.Shields.AdBlockSetting", sample: answer)
  }
  
  private func recordGlobalFingerprintingShieldsP3A() {
    // Q47 What is the global fingerprinting shields setting?
    enum Answer: Int, CaseIterable {
      case disabled = 0
      case standard = 1
      case aggressive = 2
    }
    let answer: Answer = Preferences.Shields.fingerprintingProtection.value ? .standard : .disabled
    UmaHistogramEnumeration("Brave.Shields.FingerprintBlockSetting", sample: answer)
  }

  // MARK: - Sections

  private lazy var shieldsSection: Section = {
    var shields = Section(
      header: .title(Strings.shieldsDefaults),
      rows: [
        .boolRow(title: Strings.blockAdsAndTracking, detailText: Strings.blockAdsAndTrackingDescription, option: Preferences.Shields.blockAdsAndTracking),
        .boolRow(title: Strings.HTTPSEverywhere, detailText: Strings.HTTPSEverywhereDescription, option: Preferences.Shields.httpsEverywhere),
        .boolRow(title: Strings.blockPhishingAndMalware, option: Preferences.Shields.blockPhishingAndMalware),
        .boolRow(title: Strings.autoRedirectAMPPages, detailText: Strings.autoRedirectAMPPagesDescription, option: Preferences.Shields.autoRedirectAMPPages),
        .boolRow(title: Strings.autoRedirectTrackingURLs, detailText: Strings.autoRedirectTrackingURLsDescription, option: Preferences.Shields.autoRedirectTrackingURLs),
        .boolRow(title: Strings.blockScripts, detailText: Strings.blockScriptsDescription, option: Preferences.Shields.blockScripts),
        .boolRow(
          title: Strings.blockAllCookies, detailText: Strings.blockCookiesDescription, option: Preferences.Privacy.blockAllCookies,
          onValueChange: { [unowned self] in
            func toggleCookieSetting(with status: Bool) {
              // Lock/Unlock Cookie Folder
              let completionBlock: (Bool) -> Void = { _ in
                let success = FileManager.default.setFolderAccess([
                  (.cookie, status),
                  (.webSiteData, status),
                ])
                if success {
                  Preferences.Privacy.blockAllCookies.value = status
                } else {
                  // Revert the changes. Not handling success here to avoid a loop.
                  FileManager.default.setFolderAccess([
                    (.cookie, false),
                    (.webSiteData, false),
                  ])
                  self.toggleSwitch(on: false, section: self.shieldsSection, rowUUID: Preferences.Privacy.blockAllCookies.key)

                  // TODO: Throw Alert to user to try again?
                  let alert = UIAlertController(title: nil, message: Strings.blockAllCookiesFailedAlertMsg, preferredStyle: .alert)
                  alert.addAction(UIAlertAction(title: Strings.OKString, style: .default))
                  self.present(alert, animated: true)
                }
              }
              // Save cookie to disk before purge for unblock load.
              if status {
                HTTPCookie.saveToDisk(completion: completionBlock)
              } else {
                completionBlock(true)
              }
            }
            if $0 {
              let status = $0
              // THROW ALERT to inform user of the setting
              let alert = UIAlertController(title: Strings.blockAllCookiesAlertTitle, message: Strings.blockAllCookiesAlertInfo, preferredStyle: .alert)
              let okAction = UIAlertAction(
                title: Strings.blockAllCookiesAction, style: .destructive,
                handler: { (action) in
                  toggleCookieSetting(with: status)
                })
              alert.addAction(okAction)

              let cancelAction = UIAlertAction(
                title: Strings.cancelButtonTitle, style: .cancel,
                handler: { (action) in
                  self.toggleSwitch(on: false, section: self.shieldsSection, rowUUID: Preferences.Privacy.blockAllCookies.key)
                })
              alert.addAction(cancelAction)
              self.present(alert, animated: true)
            } else {
              toggleCookieSetting(with: $0)
            }
          }),
        
        .boolRow(
          title: Strings.fingerprintingProtection,
          detailText: Strings.fingerprintingProtectionDescription,
          option: Preferences.Shields.fingerprintingProtection
        ),
        makeCookieConsentBlockingRow(),
        Row(
          text: Strings.contentFiltering,
          detailText: Strings.contentFilteringDescription,
          selection: { [weak self] in
            let controller = UIHostingController(rootView: FilterListsView())
            self?.navigationController?.pushViewController(controller, animated: true)
          },
          accessory: .disclosureIndicator, cellClass: MultilineSubtitleCell.self, uuid: "content-filtering"
        ),
      ],
      footer: .title(Strings.shieldsDefaultsFooter)
    )
    return shields
  }()
  
  private var blockMobileAnnoyancesRow: Row {
    return .boolRow(
      uuid: self.blockSwitchToAppNoticesRowUUID,
      title: Strings.blockMobileAnnoyances,
      toggleValue: FilterListStorage.shared.isEnabled(
        for: FilterList.mobileAnnoyancesComponentID
      ),
      valueChange: { isEnabled in
        FilterListStorage.shared.enableFilterList(
          for: FilterList.mobileAnnoyancesComponentID, isEnabled: isEnabled
        )
      }, cellReuseId: "blockSwitchToAppNoticesReuseIdentifier"
    )
  }

  private lazy var toggles: [Bool] = {
    let savedToggles = Preferences.Privacy.clearPrivateDataToggles.value
    // Ensure if we ever add an option to the list of clearables we don't crash
    if savedToggles.count == clearables.count {
      return savedToggles
    }

    return self.clearables.map { $0.checked }
  }()

  private lazy var clearables: [(clearable: Clearable, checked: Bool)] = {
    var alwaysVisible: [(clearable: Clearable, checked: Bool)] =
      [
        (HistoryClearable(historyAPI: self.historyAPI), true),
        (CacheClearable(), true),
        (CookiesAndCacheClearable(), true),
        (PasswordsClearable(profile: self.profile), true),
        (DownloadsClearable(), true),
      ]

    let others: [(clearable: Clearable, checked: Bool)] =
      [
        (PlayListCacheClearable(), false),
        (PlayListDataClearable(), false),
        (RecentSearchClearable(), true),
      ]

    alwaysVisible.append((BraveNewsClearable(feedDataSource: self.feedDataSource), true))

    alwaysVisible.append(contentsOf: others)

    return alwaysVisible
  }()

  private lazy var clearPrivateDataSection: Section = {
    return Section(
      header: .title(Strings.clearPrivateData),
      rows: clearables.indices.map { idx in
        let title = self.clearables[idx].clearable.label

        return .boolRow(
          title: title, toggleValue: self.toggles[idx],
          valueChange: { [unowned self] checked in
            self.toggles[idx] = checked
            Preferences.Privacy.clearPrivateDataToggles.value = self.toggles
          }, cellReuseId: "\(title.lowercased().trimmingCharacters(in: .whitespacesAndNewlines))\(idx)")
      } + [
        Row(
          text: Strings.clearDataNow,
          selection: { [unowned self] in
            self.tappedClearPrivateData()
          }, cellClass: CenteredButtonCell.self)
      ]
    )
  }()

  private lazy var manageWebsiteDataSection: Section = {
    let privacyReportRow = Row(
      text: Strings.PrivacyHub.privacyReportsTitle,
      selection: { [unowned self] in
        let controller = UIHostingController(rootView: PrivacyReportSettingsView())
        self.navigationController?.pushViewController(controller, animated: true)
      }, accessory: .disclosureIndicator, cellClass: MultilineSubtitleCell.self)
    
    return Section(
      rows: [
        Row(
          text: Strings.manageWebsiteDataTitle,
          selection: { [unowned self] in
            var view = ManageWebsiteDataView()
            let controller = UIHostingController(rootView: view)
            // pushing SwiftUI with navigation/toolbars inside the PanModal is buggy…
            // presenting over context is also buggy (eats swipe gestures)
            self.present(controller, animated: true)
          }, accessory: .disclosureIndicator),
        privacyReportRow
      ]
    )
  }()

  private lazy var otherSettingsSection: Section = {
    let p3aRow = Row.boolRow(
      title: Strings.P3A.settingTitle,
      detailText: Strings.P3A.settingSubtitle,
      toggleValue: p3aUtilities.isP3AEnabled,
      valueChange: { [unowned self] isOn in
        p3aUtilities.isP3AEnabled = isOn
      },
      cellReuseId: "p3a"
    )
    
    var section = Section(
      header: .title(Strings.otherPrivacySettingsSection),
      rows: [
        .boolRow(
          title: Strings.privateBrowsingOnly,
          option: Preferences.Privacy.privateBrowsingOnly,
          onValueChange: { [weak self] value in
            guard let self = self else { return }
            self.pboModeToggled(value: value)
          }
        ),
        .boolRow(title: Strings.blockPopups, option: Preferences.General.blockPopups),
        blockMobileAnnoyancesRow,
        .boolRow(title: Strings.followUniversalLinks, option: Preferences.General.followUniversalLinks),
      ]
    )
    section.rows.append(
      .boolRow(
        title: Strings.googleSafeBrowsing,
        detailText: Strings.googleSafeBrowsingUsingWebKitDescription,
        option: Preferences.Shields.googleSafeBrowsing
      )
    )
    section.rows.append(p3aRow)
    
    return section
  }()

  // MARK: - Actions
  
  private func makeCookieConsentBlockingRow() -> Row {
    return .boolRow(
      uuid: self.cookieConsentNoticesRowUUID,
      title: Strings.blockCookieConsentNotices,
      toggleValue: FilterListStorage.shared.isEnabled(
        for: FilterList.cookieConsentNoticesComponentID
      ),
      valueChange: { isEnabled in
        FilterListStorage.shared.enableFilterList(
          for: FilterList.cookieConsentNoticesComponentID, isEnabled: isEnabled
        )
      }, cellReuseId: "blockCookieConsentNoticesReuseIdentifier"
    )
  }

  private func tappedClearPrivateData() {
    let alertController = UIAlertController(
      title: Strings.clearPrivateDataAlertTitle,
      message: Strings.clearPrivateDataAlertMessage,
      preferredStyle: .alert
    )
    let clearAction = UIAlertAction(title: Strings.clearPrivateDataAlertYesAction, style: .destructive) { [weak self] _ in
      guard let self = self else { return }
      Preferences.Privacy.clearPrivateDataToggles.value = self.toggles
      let spinner = SpinnerView().then {
        $0.present(on: self.view)
      }
      Task { @MainActor in
        await self.clearPrivateData(self.toggles.indices.compactMap {
          self.toggles[$0] ? self.clearables[$0].clearable : nil
        })
        spinner.dismiss()
      }
    }
    alertController.addAction(clearAction)
    alertController.addAction(.init(title: Strings.cancelButtonTitle, style: .cancel))
    self.present(alertController, animated: true, completion: nil)
  }
  
  private func toggleSwitchIfNeeded(on: Bool, section: Section, rowUUID: String) {
    guard let sectionRow = section.rows.first(where: { $0.uuid == rowUUID }) else { return }
    guard let switchView = sectionRow.accessory.view as? UISwitch else { return }
    guard switchView.isOn != on else { return }
    switchView.setOn(on, animated: true)
  }

  private func toggleSwitch(on: Bool, section: Section, rowUUID: String) {
    if let sectionRow: Row = section.rows.first(where: { $0.uuid == rowUUID }) {
      if let switchView: UISwitch = sectionRow.accessory.view as? UISwitch {
        switchView.setOn(on, animated: true)
      }
    }
  }

  private func pboModeToggled(value: Bool) {
    if value {
      let alert = UIAlertController(title: Strings.privateBrowsingOnly, message: Strings.privateBrowsingOnlyWarning, preferredStyle: .alert)
      alert.addAction(
        UIAlertAction(
          title: Strings.cancelButtonTitle, style: .cancel,
          handler: { [weak self] _ in
            guard let self = self else { return }
            DispatchQueue.main.async {
              self.toggleSwitch(on: false, section: self.otherSettingsSection, rowUUID: Preferences.Privacy.privateBrowsingOnly.key)
            }
          }))

      alert.addAction(
        UIAlertAction(
          title: Strings.OKString, style: .default,
          handler: { [weak self] _ in
            guard let self = self else { return }
            let spinner = SpinnerView().then {
              $0.present(on: self.view)
            }

            Preferences.Privacy.privateBrowsingOnly.value = value

            Task { @MainActor in
              try await Task.sleep(nanoseconds: NSEC_PER_MSEC * 100)
              let clearables: [Clearable] = [CookiesAndCacheClearable()]
              await self.clearPrivateData(clearables)
              
              // First remove all tabs so that only a blank tab exists.
              self.tabManager.removeAll()
              
              // Reset tab configurations and delete all webviews..
              self.tabManager.reset()
              
              // Restore all existing tabs by removing the blank tabs and recreating new ones..
              self.tabManager.removeAll()
              
              spinner.dismiss()
            }
          }))

      self.present(alert, animated: true, completion: nil)
    } else {
      Preferences.Privacy.privateBrowsingOnly.value = value
    }
  }

  @MainActor func clearPrivateData(_ clearables: [Clearable]) async {
    @Sendable func _clear(_ clearables: [Clearable], secondAttempt: Bool = false) async {
      await withThrowingTaskGroup(of: Void.self) { group in
        for clearable in clearables {
          group.addTask {
            try await clearable.clear()
          }
        }
        do {
          for try await _ in group { }
        } catch {
          if !secondAttempt {
            Logger.module.error("Private data NOT cleared successfully")
            try? await Task.sleep(nanoseconds: NSEC_PER_MSEC * 500)
            await _clear(clearables, secondAttempt: true)
          } else {
            Logger.module.error("Private data NOT cleared after 2 attempts")
          }
        }
      }
    }

    let clearAffectsTabs = clearables.contains { item in
      return item is CacheClearable || item is CookiesAndCacheClearable
    }

    let historyCleared = clearables.contains { $0 is HistoryClearable }

    if clearAffectsTabs {
      DispatchQueue.main.async {
        self.tabManager.allTabs.forEach({ $0.reload() })
      }
    }

    @Sendable func _toggleFolderAccessForBlockCookies(locked: Bool) {
      if Preferences.Privacy.blockAllCookies.value, FileManager.default.checkLockedStatus(folder: .cookie) != locked {
        FileManager.default.setFolderAccess([
          (.cookie, locked),
          (.webSiteData, locked),
        ])
      }
    }

    try? await Task.sleep(nanoseconds: NSEC_PER_SEC * 1)
    
    // Reset Webkit configuration to remove data from memory
    if clearAffectsTabs {
      self.tabManager.resetConfiguration()
      // Unlock the folders to allow clearing of data.
      _toggleFolderAccessForBlockCookies(locked: false)
    }
    
    await _clear(clearables)
    if clearAffectsTabs {
      self.tabManager.allTabs.forEach({ $0.reload() })
    }
    
    if historyCleared {
      self.tabManager.clearTabHistory()
      
      // Clearing Tab History should clear Recently Closed
      RecentlyClosed.removeAll()
      
      // Donate Clear Browser History for suggestions
      let clearBrowserHistoryActivity = ActivityShortcutManager.shared.createShortcutActivity(type: .clearBrowsingHistory)
      self.userActivity = clearBrowserHistoryActivity
      clearBrowserHistoryActivity.becomeCurrent()
    }
    
    _toggleFolderAccessForBlockCookies(locked: true)
  }
}
