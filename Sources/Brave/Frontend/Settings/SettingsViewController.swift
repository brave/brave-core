/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import Shared
import BraveShared
import Preferences
import Static
import LocalAuthentication
import SwiftyJSON
import Data
import WebKit
import BraveCore
import SwiftUI
import BraveWallet
import BraveUI
import BraveVPN
import BraveNews
import Growth
import NetworkExtension

extension TabBarVisibility: RepresentableOptionType {
  public var displayString: String {
    switch self {
    case .always: return Strings.alwaysShow
    case .landscapeOnly: return Strings.showInLandscapeOnly
    case .never: return Strings.neverShow
    }
  }
}

extension Preferences.AutoCloseTabsOption: RepresentableOptionType {
  public var displayString: String {
    switch self {
    case .manually: return Strings.Settings.autocloseTabsManualOption
    case .oneDay: return Strings.Settings.autocloseTabsOneDayOption
    case .oneWeek: return Strings.Settings.autocloseTabsOneWeekOption
    case .oneMonth: return Strings.Settings.autocloseTabsOneMonthOption
    }
  }
}

protocol SettingsDelegate: AnyObject {
  func settingsOpenURLInNewTab(_ url: URL)
  func settingsOpenURLs(_ urls: [URL], loadImmediately: Bool)
}

class SettingsViewController: TableViewController {
  weak var settingsDelegate: SettingsDelegate?

  private let profile: Profile
  private let tabManager: TabManager
  private let rewards: BraveRewards?
  private let feedDataSource: FeedDataSource
  private let historyAPI: BraveHistoryAPI
  private let passwordAPI: BravePasswordAPI
  private let syncAPI: BraveSyncAPI
  private let syncProfileServices: BraveSyncProfileServiceIOS
  private let p3aUtilities: BraveP3AUtils
  private let attributionManager: AttributionManager
  private let keyringStore: KeyringStore?
  private let cryptoStore: CryptoStore?
  private let windowProtection: WindowProtection?
  private let ipfsAPI: IpfsAPI

  private let featureSectionUUID: UUID = .init()
  private let walletRowUUID: UUID = .init()

  init(
    profile: Profile,
    tabManager: TabManager,
    feedDataSource: FeedDataSource,
    rewards: BraveRewards? = nil,
    windowProtection: WindowProtection?,
    braveCore: BraveCoreMain,
    attributionManager: AttributionManager,
    keyringStore: KeyringStore? = nil,
    cryptoStore: CryptoStore? = nil
  ) {
    self.profile = profile
    self.tabManager = tabManager
    self.feedDataSource = feedDataSource
    self.rewards = rewards
    self.windowProtection = windowProtection
    self.historyAPI = braveCore.historyAPI
    self.passwordAPI = braveCore.passwordAPI
    self.syncAPI = braveCore.syncAPI
    self.syncProfileServices = braveCore.syncProfileService
    self.p3aUtilities = braveCore.p3aUtils
    self.attributionManager = attributionManager
    self.keyringStore = keyringStore
    self.cryptoStore = cryptoStore
    self.ipfsAPI = braveCore.ipfsAPI

    super.init(style: .insetGrouped)
  }
  
  deinit {
    keyringStore?.tearDown()
    cryptoStore?.tearDown()
    
    NotificationCenter.default.removeObserver(self)
  }

  @available(*, unavailable)
  required init?(coder aDecoder: NSCoder) {
    fatalError()
  }

  override func viewDidLoad() {
    super.viewDidLoad()

    navigationItem.title = Strings.settings
    tableView.accessibilityIdentifier = "SettingsViewController.tableView"
    setUpSections()

    tableView.separatorInset = UIEdgeInsets(top: 0, left: 16, bottom: 0, right: 0)

    view.backgroundColor = .braveGroupedBackground
    view.tintColor = .braveBlurpleTint
    navigationController?.view.backgroundColor = .braveGroupedBackground
    
    NotificationCenter.default.addObserver(
      self,
      selector: #selector(vpnConfigChanged(notification:)),
      name: .NEVPNStatusDidChange, object: nil)
  }

  private func displayRewardsDebugMenu() {
    guard let rewards = rewards else { return }
    let settings = RewardsDebugSettingsViewController(rewards: rewards)
    navigationController?.pushViewController(settings, animated: true)
  }

  private func displayBraveNewsDebugMenu() {
    let settings = UIHostingController(rootView: BraveNewsDebugSettingsView(dataSource: feedDataSource))
    navigationController?.pushViewController(settings, animated: true)
  }

  private func displayBraveSearchDebugMenu() {
    let hostingController =
      UIHostingController(rootView: BraveSearchDebugMenu(logging: BraveSearchLogEntry.shared))

    navigationController?.pushViewController(hostingController, animated: true)
  }
  
  /// The function for refreshing VPN status for menu
  /// - Parameter notification: NEVPNStatusDidChange
  @objc private func vpnConfigChanged(notification: NSNotification) {
    guard let connection = notification.object as? NEVPNConnection else { return }

    if connection.status == .connected || connection.status == .disconnected {
      setUpSections()
      tableView.reloadData()
    }
  }

  // Do not use `sections` directly to access sections/rows. Use DataSource.sections instead.
  private func makeSections() -> [Static.Section] {
    var list = [
      defaultBrowserSection,
      makeFeaturesSection(),
      generalSection,
      displaySection,
      tabsSection,
      securitySection,
      supportSection,
      aboutSection,
    ]

    let shouldShowVPNSection = { () -> Bool in
      if !VPNProductInfo.isComplete || Preferences.VPN.vpnSettingHeaderWasDismissed.value {
        return false
      }

      switch BraveVPN.vpnState {
      case .notPurchased, .expired:
        return true
      case .purchased:
        return false
      }
    }()

    if shouldShowVPNSection {
      list.insert(enableBraveVPNSection, at: 0)
    }

    if let debugSection = debugSection {
      list.append(debugSection)
    }

    return list
  }

  // MARK: - Sections

  private lazy var enableBraveVPNSection: Static.Section = {
    let header = EnableVPNSettingHeader()
    header.enableVPNTapped = { [weak self] in
      self?.enableVPNTapped()
    }

    header.dismissHeaderTapped = { [weak self] in
      self?.dismissVPNHeaderTapped()
    }

    let calculatedSize = header.systemLayoutSizeFitting(
      CGSize(width: navigationController?.navigationBar.frame.width ?? 0, height: 300),
      withHorizontalFittingPriority: .required,
      verticalFittingPriority: .fittingSizeLevel
    )

    header.bounds = CGRect(size: calculatedSize)

    return Static.Section(header: .view(header))
  }()

  private lazy var defaultBrowserSection: Static.Section = {
    var section = Static.Section(
      rows: [
        .init(
          text: Strings.setDefaultBrowserSettingsCell,
          selection: { [unowned self] in
            guard let settingsUrl = URL(string: UIApplication.openSettingsURLString) else {
              return
            }
            UIApplication.shared.open(settingsUrl)
          }, cellClass: MultilineButtonCell.self)
      ]
    )

    return section
  }()

  private func makeFeaturesSection() -> Static.Section {
    weak var spinner: SpinnerView?
    
    var section = Static.Section(
      header: .title(Strings.features),
      rows: [
        Row(
          text: Strings.braveShieldsAndPrivacy,
          selection: { [unowned self] in
            let controller = UIHostingController(rootView: AdvancedShieldsSettingsView(
              profile: self.profile,
              tabManager: self.tabManager,
              feedDataSource: self.feedDataSource,
              historyAPI: self.historyAPI,
              p3aUtilities: self.p3aUtilities,
              clearDataCallback: { [weak self] isLoading, isHistoryCleared in
                guard let view = self?.navigationController?.view, view.window != nil else {
                  assertionFailure()
                  return
                }
                
                if isLoading, spinner == nil {
                  let newSpinner = SpinnerView()
                  newSpinner.present(on: view)
                  spinner = newSpinner
                } else {
                  spinner?.dismiss()
                  spinner = nil
                }
                
                if isHistoryCleared {
                  // Donate Clear Browser History for suggestions
                  let clearBrowserHistoryActivity = ActivityShortcutManager.shared.createShortcutActivity(type: .clearBrowsingHistory)
                  self?.userActivity = clearBrowserHistoryActivity
                  clearBrowserHistoryActivity.becomeCurrent()
                }
              }
            ))
            
            controller.rootView.openURLAction = { [unowned self] url in
              self.settingsDelegate?.settingsOpenURLInNewTab(url)
              self.dismiss(animated: true)
            }
            
            self.navigationController?.pushViewController(controller, animated: true)
          },
          image: UIImage(braveSystemNamed: "leo.shield.done"),
          accessory: .disclosureIndicator
        )
      ],
      uuid: featureSectionUUID.uuidString
    )

    if BraveRewards.isAvailable, let rewards = rewards {
      section.rows += [
        Row(
          text: Strings.braveRewardsTitle,
          selection: { [unowned self] in
            let rewardsVC = BraveRewardsSettingsViewController(rewards)
            rewardsVC.walletTransferLearnMoreTapped = { [weak self] in
              guard let self = self else { return }
              self.dismiss(animated: true) {
                self.presentingViewController?.dismiss(animated: true) {
                  self.settingsDelegate?.settingsOpenURLInNewTab(.brave.rewardsOniOS)
                }
              }
            }
            self.navigationController?.pushViewController(rewardsVC, animated: true)
          }, image: UIImage(braveSystemNamed: "leo.product.bat-outline"), accessory: .disclosureIndicator)
      ]
    }

    section.rows.append(
      Row(
        text: Strings.BraveNews.braveNews,
        selection: { [unowned self] in
          let controller = NewsSettingsViewController(dataSource: self.feedDataSource, openURL: { [weak self] url in
            guard let self else { return }
            self.dismiss(animated: true)
            self.settingsDelegate?.settingsOpenURLs([url], loadImmediately: true)
          })
          controller.viewDidDisappear = {
            if Preferences.Review.braveNewsCriteriaPassed.value {
              AppReviewManager.shared.isRevisedReviewRequired = true
              Preferences.Review.braveNewsCriteriaPassed.value = false
            }
          }
          self.navigationController?.pushViewController(controller, animated: true)
        }, image: UIImage(braveSystemNamed: "leo.product.brave-news"), accessory: .disclosureIndicator)
    )

    vpnRow = vpnSettingsRow()
    if let vpnRow = vpnRow {
      section.rows.append(vpnRow)
    }

    section.rows.append(
      Row(
        text: Strings.PlayList.playListTitle,
        selection: { [unowned self] in
          let playlistSettings = PlaylistSettingsViewController()
          self.navigationController?.pushViewController(playlistSettings, animated: true)
        }, image: UIImage(braveSystemNamed: "leo.product.playlist"), accessory: .disclosureIndicator)
    )

    return section
  }

  private lazy var generalSection: Static.Section = {
    var general = Static.Section(
      header: .title(Strings.settingsGeneralSectionTitle),
      rows: [
        Row(
          text: Strings.searchEngines,
          selection: { [unowned self] in
            let viewController = SearchSettingsTableViewController(profile: self.profile, privateBrowsingManager: tabManager.privateBrowsingManager)
            self.navigationController?.pushViewController(viewController, animated: true)
          }, image: UIImage(braveSystemNamed: "leo.search"), accessory: .disclosureIndicator, cellClass: MultilineValue1Cell.self),
        Row(
          text: Strings.sync,
          selection: { [unowned self] in
            if syncAPI.isInSyncGroup {
              if !DeviceInfo.hasConnectivity() {
                self.present(SyncAlerts.noConnection, animated: true)
                return
              }
              
              let syncSettingsViewController = SyncSettingsTableViewController(
                syncAPI: syncAPI,
                syncProfileService:
                  syncProfileServices,
                tabManager: tabManager,
                windowProtection: windowProtection)  

              self.navigationController?
                .pushViewController(syncSettingsViewController, animated: true)
            } else {
              let syncWelcomeViewController = SyncWelcomeViewController(
                syncAPI: syncAPI,
                syncProfileServices: syncProfileServices,
                tabManager: tabManager,
                windowProtection: windowProtection)
              
              self.navigationController?.pushViewController(syncWelcomeViewController, animated: true)
            }
          }, image: UIImage(braveSystemNamed: "leo.sync"), accessory: .disclosureIndicator,
          cellClass: MultilineValue1Cell.self),
        .boolRow(title: Strings.bookmarksLastVisitedFolderTitle, option: Preferences.General.showLastVisitedBookmarksFolder, image: UIImage(braveSystemNamed: "leo.folder.open")),
        Row(
          text: Strings.Shortcuts.shortcutSettingsTitle,
          selection: { [unowned self] in
            self.navigationController?.pushViewController(ShortcutSettingsViewController(), animated: true)
          }, image: UIImage(braveSystemNamed: "leo.siri.shorcut"), accessory: .disclosureIndicator, cellClass: MultilineValue1Cell.self),
      ]
    )

    if UIDevice.isIpad {
      general.rows.append(
        .boolRow(
          title: Strings.alwaysRequestDesktopSite,
          option: Preferences.UserAgent.alwaysRequestDesktopSite,
          image: UIImage(braveSystemNamed: "leo.window.cursor"))
      )
    }

    general.rows.append(contentsOf: [
      .boolRow(
        title: Strings.enablePullToRefresh,
        option: Preferences.General.enablePullToRefresh,
        image: UIImage(braveSystemNamed: "leo.browser.refresh")),
      .boolRow(title: Strings.blockPopups, option: Preferences.General.blockPopups,
               image: UIImage(braveSystemNamed: "leo.shield.block")),
    ])

    let websiteRedirectsRow = Row(
      text: Strings.urlRedirectsSettings,
      selection: { [unowned self] in
        let controller = UIHostingController(rootView: WebsiteRedirectsSettingsView())
        self.navigationController?.pushViewController(controller, animated: true)
      }, image: UIImage(braveSystemNamed: "leo.swap.horizontal"), accessory: .disclosureIndicator, cellClass: MultilineSubtitleCell.self)
    general.rows.append(websiteRedirectsRow)

    return general
  }()

  private lazy var tabsSection: Static.Section = {
    var tabs = Static.Section(header: .title(Strings.tabsSettingsSectionTitle), rows: [])

    if UIDevice.current.userInterfaceIdiom == .phone {
      tabs.rows.append(
        Row(cellClass: LocationViewPositionPickerCell.self)
      )
    }

    if UIDevice.current.userInterfaceIdiom == .pad {
      tabs.rows.append(
        Row(
          text: Strings.showTabsBar,
          image: UIImage(braveSystemNamed: "leo.window.tab"),
          accessory: .switchToggle(value: Preferences.General.tabBarVisibility.value != TabBarVisibility.never.rawValue, {
            Preferences.General.tabBarVisibility.value = $0 ? TabBarVisibility.always.rawValue : TabBarVisibility.never.rawValue
          }),
          cellClass: MultilineValue1Cell.self
        )
      )
    } else {
      var row = Row(text: Strings.showTabsBar, detailText: TabBarVisibility(rawValue: Preferences.General.tabBarVisibility.value)?.displayString, image: UIImage(braveSystemNamed: "leo.window.tab"), accessory: .disclosureIndicator, cellClass: MultilineValue1Cell.self)
      row.selection = { [unowned self] in
        // Show options for tab bar visibility
        let optionsViewController = OptionSelectionViewController<TabBarVisibility>(
          options: TabBarVisibility.allCases,
          selectedOption: TabBarVisibility(rawValue: Preferences.General.tabBarVisibility.value),
          optionChanged: { _, option in
            Preferences.General.tabBarVisibility.value = option.rawValue
            self.dataSource.reloadCell(row: row, section: tabs, displayText: option.displayString)
          }
        )
        optionsViewController.headerText = Strings.showTabsBar
        optionsViewController.navigationItem.title = Strings.showTabsBar
        self.navigationController?.pushViewController(optionsViewController, animated: true)
      }
      tabs.rows.append(row)
    }

    let autoCloseSetting =
    Preferences
      .AutoCloseTabsOption(rawValue: Preferences.General.autocloseTabs.value)?.displayString
    var autoCloseTabsRow =
    Row(
      text: Strings.Settings.autocloseTabsSetting,
      detailText: autoCloseSetting, image: UIImage(braveSystemNamed: "leo.window.tabs"),
      accessory: .disclosureIndicator,
      cellClass: MultilineSubtitleCell.self)
    autoCloseTabsRow.selection = { [unowned self] in
      let optionsViewController = OptionSelectionViewController<Preferences.AutoCloseTabsOption>(
        options: Preferences.AutoCloseTabsOption.allCases,
        selectedOption:
          Preferences.AutoCloseTabsOption(rawValue: Preferences.General.autocloseTabs.value),
        optionChanged: { _, option in
          Preferences.General.autocloseTabs.value = option.rawValue
          self.dataSource.reloadCell(row: autoCloseTabsRow, section: tabs, displayText: option.displayString)
        }
      )
      optionsViewController.headerText = Strings.Settings.autocloseTabsSetting
      optionsViewController.footerText = Strings.Settings.autocloseTabsSettingFooter
      optionsViewController.navigationItem.title = Strings.Settings.autocloseTabsSetting
      self.navigationController?.pushViewController(optionsViewController, animated: true)
    }

    tabs.rows.append(autoCloseTabsRow)
    
    if !Preferences.Privacy.privateBrowsingOnly.value {
      let privateTabsRow = Row(
        text: Strings.TabsSettings.privateTabsSettingsTitle,
        selection: { [unowned self] in
          let vc = UIHostingController(rootView: PrivateTabsView(tabManager: tabManager, askForAuthentication: self.askForLocalAuthentication))
          self.navigationController?.pushViewController(vc, animated: true)
        },
        image: UIImage(braveSystemNamed: "leo.product.private-window"),
        accessory: .disclosureIndicator)
      
      tabs.rows.append(privateTabsRow)
    }

    return tabs
  }()

  private lazy var displaySection: Static.Section = {
    var display = Static.Section(
      header: .title(Strings.displaySettingsSection),
      rows: []
    )
    
    display.rows.append(.init(
      text: Strings.Settings.mediaRootSetting,
      selection: { [unowned self] in
        let vc = UIHostingController(rootView: MediaSettingsView())
        self.navigationController?.pushViewController(vc, animated: true)
      },
      image: UIImage(braveSystemNamed: "leo.media.player"),
      accessory: .disclosureIndicator,
      cellClass: MultilineValue1Cell.self
    ))

    let themeSubtitle = DefaultTheme(rawValue: Preferences.General.themeNormalMode.value)?.displayString
    var row = Row(text: Strings.themesDisplayBrightness, detailText: themeSubtitle, image: UIImage(braveSystemNamed: "leo.appearance"), accessory: .disclosureIndicator, cellClass: MultilineSubtitleCell.self)
    row.selection = { [unowned self] in
      let optionsViewController = OptionSelectionViewController<DefaultTheme>(
        options: DefaultTheme.normalThemesOptions,
        selectedOption: DefaultTheme(rawValue: Preferences.General.themeNormalMode.value),
        optionChanged: { [unowned self] _, option in
          Preferences.General.themeNormalMode.value = option.rawValue
          self.dataSource.reloadCell(row: row, section: display, displayText: option.displayString)
        }
      )
      optionsViewController.headerText = Strings.themesDisplayBrightness
      optionsViewController.footerText = Strings.themesDisplayBrightnessFooter
      optionsViewController.navigationItem.title = Strings.themesDisplayBrightness
      
      let nightModeSection = Section(
        header: .title(Strings.NightMode.sectionTitle.uppercased()),
        rows: [
          .boolRow(
            title: Strings.NightMode.settingsTitle,
            detailText: Strings.NightMode.settingsDescription,
            option: Preferences.General.nightModeEnabled,
            onValueChange: { [unowned self] enabled in
              NightModeScriptHandler.setNightMode(tabManager: tabManager, enabled: enabled)
            },
            image: UIImage(braveSystemNamed: "leo.theme.dark"))
        ],
        footer: .title(Strings.NightMode.sectionDescription)
      )

      optionsViewController.dataSource.sections.append(nightModeSection)
      self.navigationController?.pushViewController(optionsViewController, animated: true)
    }
    display.rows.append(row)
    display.rows.append(Row(
      text: Strings.NTP.settingsTitle,
      selection: { [unowned self] in
        self.navigationController?.pushViewController(NTPTableViewController(), animated: true)
      },
      image: UIImage(braveSystemNamed: "leo.window.tab-new"),
      accessory: .disclosureIndicator,
      cellClass: MultilineValue1Cell.self
    ))

    // We do NOT persistently save page-zoom settings in Private Browsing
    if !tabManager.privateBrowsingManager.isPrivateBrowsing {
      display.rows.append(
        Row(text: Strings.PageZoom.settingsTitle,
            selection: { [weak self] in
              let controller = PageZoomSettingsController()
              controller.navigationItem.title = Strings.PageZoom.settingsTitle
              self?.navigationController?.pushViewController(controller, animated: true)
            },
            image: UIImage(braveSystemNamed: "leo.font.size"),
            accessory: .disclosureIndicator,
            cellClass: MultilineValue1Cell.self)
      )
    }

    display.rows.append(contentsOf: [
      .boolRow(
        title: Strings.showBookmarkButtonInTopToolbar,
        option: Preferences.General.showBookmarkToolbarShortcut,
        image: UIImage(braveSystemNamed: "leo.product.bookmarks")),
      .boolRow(
        title: Strings.hideRewardsIcon,
        option: Preferences.Rewards.hideRewardsIcon,
        image: UIImage(braveSystemNamed: "leo.product.bat-outline")),
    ])

    return display
  }()

  private var vpnRow: Row?

  private func vpnSettingsRow() -> Row {

    let (text, color) = { () -> (String, UIColor) in
      if Preferences.VPN.vpnReceiptStatus.value == BraveVPN.ReceiptResponse.Status.retryPeriod.rawValue {
        return (Strings.VPN.updateActionCellTitle, .braveErrorLabel)
      }
      
      switch BraveVPN.vpnState {
      case .notPurchased:
        return ("", UIColor.black)
      case .purchased(let enabled):
        if enabled {
          return (Strings.VPN.settingsVPNEnabled, .braveSuccessLabel)
        } else {
          return (Strings.VPN.settingsVPNDisabled, .braveErrorLabel)
        }
      case .expired:
        return (Strings.VPN.settingsVPNExpired, .braveErrorLabel)
      }
    }()

    return Row(
      text: Strings.VPN.vpnName, detailText: text,
      selection: { [unowned self] in

        let vc = { () -> UIViewController? in
          switch BraveVPN.vpnState {
          case .notPurchased, .expired:
            return BraveVPN.vpnState.enableVPNDestinationVC
          case .purchased:
            let vc = BraveVPNSettingsViewController(iapObserver: BraveVPN.iapObserver)
            vc.openURL = { [unowned self] url in
              self.settingsDelegate?.settingsOpenURLInNewTab(url)
              self.dismiss(animated: true)
            }

            return vc
          }
        }()

        guard let vcToShow = vc else { return }
        
        if VPNProductInfo.isComplete {
          self.navigationController?.pushViewController(vcToShow, animated: true)
        } else {
          let alert = UIAlertController(
            title: Strings.VPN.errorCantGetPricesTitle,
            message: Strings.VPN.errorCantGetPricesBody,
            preferredStyle: .alert)

          alert.addAction(UIAlertAction(title: Strings.OKString, style: .default, handler: nil))
          self.present(alert, animated: true, completion: nil)
        }
      },
      image: Preferences.VPN.vpnReceiptStatus.value == BraveVPN.ReceiptResponse.Status.retryPeriod.rawValue
        ? UIImage(braveSystemNamed: "leo.warning.triangle-filled")?
            .withRenderingMode(.alwaysOriginal)
            .withTintColor(.braveErrorLabel)
        : UIImage(braveSystemNamed: "leo.product.vpn"),
      accessory: .disclosureIndicator,
      cellClass: ColoredDetailCell.self, context: [ColoredDetailCell.colorKey: color], uuid: "vpnrow")
  }

  private lazy var securitySection: Static.Section = {
    return Section(
      header: .title(Strings.security),
      rows: [
        Row(
          text: Strings.Privacy.browserLock,
          detailText: Strings.Privacy.browserLockDescription,
          image: UIImage(braveSystemNamed: "leo.biometric.login"),
          accessory: .view(SwitchAccessoryView(initialValue: Preferences.Privacy.lockWithPasscode.value, valueChange: { isOn in
            if isOn {
              Preferences.Privacy.lockWithPasscode.value = isOn
            } else {
              self.askForLocalAuthentication { [weak self] success, error in
                if success {
                  Preferences.Privacy.lockWithPasscode.value = isOn
                }
              }
            }
          })),
          cellClass: MultilineSubtitleCell.self,
          uuid: Preferences.Privacy.lockWithPasscode.key
        ),
        Row(
          text: Strings.Login.loginListNavigationTitle,
          selection: { [unowned self] in
            let loginsPasswordsViewController = LoginListViewController(
              passwordAPI: self.passwordAPI,
              windowProtection: self.windowProtection)
            loginsPasswordsViewController.settingsDelegate = self.settingsDelegate
            self.navigationController?.pushViewController(loginsPasswordsViewController, animated: true)
          }, image: UIImage(braveSystemNamed: "leo.outside"), accessory: .disclosureIndicator)
      ]
    )
  }()

  private lazy var supportSection: Static.Section = {
    return Static.Section(
      header: .title(Strings.support),
      rows: [
        Row(
          text: Strings.reportABug,
          selection: { [unowned self] in
            self.settingsDelegate?.settingsOpenURLInNewTab(.brave.community)
            self.dismiss(animated: true)
          },
          image: UIImage(braveSystemNamed: "leo.bug"),
          cellClass: MultilineValue1Cell.self),
        Row(
          text: Strings.rateBrave,
          selection: { [unowned self] in
            // Rate Brave
            guard let writeReviewURL = URL(string: "https://itunes.apple.com/app/id1052879175?action=write-review")
            else { return }
            UIApplication.shared.open(writeReviewURL)
            self.dismiss(animated: true)
          },
          image: UIImage(braveSystemNamed: "leo.message.bubble-smile"),
          cellClass: MultilineValue1Cell.self),
      ]
    )
  }()

  private lazy var aboutSection: Static.Section = {
    let version = String(
      format: Strings.versionTemplate,
      Bundle.main.object(forInfoDictionaryKey: "CFBundleShortVersionString") as? String ?? "",
      Bundle.main.object(forInfoDictionaryKey: "CFBundleVersion") as? String ?? "")
    let coreVersion = "BraveCore \(BraveCoreVersionInfo.braveCoreVersion) (\(BraveCoreVersionInfo.chromiumVersion))"
    return Static.Section(
      header: .title(Strings.about),
      rows: [
        Row(
          text: version,
          selection: { [unowned self] in
            let device = UIDevice.current
            let actionSheet = UIAlertController(title: version, message: coreVersion, preferredStyle: .actionSheet)
            actionSheet.popoverPresentationController?.sourceView = self.view
            actionSheet.popoverPresentationController?.sourceRect = self.view.bounds
            let iOSVersion = "\(device.systemName) \(UIDevice.current.systemVersion)"

            let deviceModel = String(format: Strings.deviceTemplate, device.modelName, iOSVersion)
            let copyDebugInfoAction = UIAlertAction(title: Strings.copyAppInfoToClipboard, style: .default) { _ in
              UIPasteboard.general.strings = [version, coreVersion, deviceModel]
            }
            
            let copyAppInfoAction = UIAlertAction(title: Strings.copyAppSizeInfoToClipboard, style: .default) { _ in
              UIPasteboard.general.setSecureString(AppStorageDebugComposer.compose(),
                                                   expirationDate: Date().addingTimeInterval(2.minutes))
            }

            actionSheet.addAction(copyDebugInfoAction)
            actionSheet.addAction(copyAppInfoAction)
            actionSheet.addAction(UIAlertAction(title: Strings.cancelButtonTitle, style: .cancel, handler: nil))
            self.navigationController?.present(actionSheet, animated: true, completion: nil)
          }, cellClass: MultilineValue1Cell.self),
        Row(
          text: Strings.privacyPolicy,
          selection: { [unowned self] in
            // Show privacy policy
            let privacy = SettingsContentViewController().then { $0.url = .brave.privacy }
            privacy.navigationItem.title = Strings.privacyPolicy
            self.navigationController?.pushViewController(privacy, animated: true)
          },
          accessory: .disclosureIndicator, cellClass: MultilineValue1Cell.self),
        Row(
          text: Strings.termsOfUse,
          selection: { [unowned self] in
            // Show terms of use
            let toc = SettingsContentViewController().then { $0.url = .brave.termsOfUse }
            toc.navigationItem.title = Strings.termsOfUse
            self.navigationController?.pushViewController(toc, animated: true)
          },
          accessory: .disclosureIndicator, cellClass: MultilineValue1Cell.self),
        Row(
          text: Strings.settingsLicenses,
          selection: { [unowned self] in
            let licenses = SettingsContentViewController().then {
              $0.url = URL(string: "\(InternalURL.baseUrl)/\(AboutLicenseHandler.path)")
            }
            self.navigationController?.pushViewController(licenses, animated: true)
          }, accessory: .disclosureIndicator),
      ]
    )
  }()

  private lazy var debugSection: Static.Section? = {
    if AppConstants.buildChannel.isPublic { return nil }

    return Static.Section(
      rows: [
        Row(text: "Region: \(Locale.current.regionCode ?? "--")"),
        Row(
          text: "Sandbox Inspector",
          selection: { [unowned self] in
            let vc = UIHostingController(rootView: SandboxInspectorView())
            self.navigationController?.pushViewController(vc, animated: true)
          }, accessory: .disclosureIndicator),
        Row(
          text: "Adblock Debug",
          selection: { [unowned self] in
            let vc = AdblockDebugMenuTableViewController(style: .grouped)
            self.navigationController?.pushViewController(vc, animated: true)
          }, accessory: .disclosureIndicator, cellClass: MultilineValue1Cell.self),
        Row(
          text: "View URP Logs",
          selection: { [unowned self] in
            self.navigationController?.pushViewController(UrpLogsViewController(), animated: true)
          }, accessory: .disclosureIndicator, cellClass: MultilineValue1Cell.self),
        Row(text: "URP Code: \(UserReferralProgram.getReferralCode() ?? "--")"),
        Row(
          text: "BraveCore Switches",
          selection: { [unowned self] in
            let controller = UIHostingController(rootView: BraveCoreDebugSwitchesView())
            self.navigationController?.pushViewController(controller, animated: true)
          }, accessory: .disclosureIndicator, cellClass: MultilineSubtitleCell.self),
        Row(
          text: "View Rewards Debug Menu",
          selection: { [unowned self] in
            self.displayRewardsDebugMenu()
          }, accessory: .disclosureIndicator, cellClass: MultilineValue1Cell.self),
        Row(
          text: "View Brave News Debug Menu",
          selection: { [unowned self] in
            self.displayBraveNewsDebugMenu()
          }, accessory: .disclosureIndicator, cellClass: MultilineValue1Cell.self),
        Row(
          text: "View Brave Search Debug Menu",
          selection: { [unowned self] in
            self.displayBraveSearchDebugMenu()
          }, accessory: .disclosureIndicator, cellClass: MultilineValue1Cell.self),
        Row(
          text: "View Chromium Local State",
          selection: { [unowned self] in
            let controller = UIHostingController(rootView: LocalStateInspectorView())
            self.navigationController?.pushViewController(controller, animated: true)
          }, accessory: .disclosureIndicator, cellClass: MultilineValue1Cell.self
        ),
        Row(
          text: "View Brave Histogram (p3a) Logs",
          selection: { [unowned self] in
            let histogramsController = ChromeWebViewController(privateBrowsing: false).then {
              $0.title = "Histograms (p3a)"
              $0.loadURL("brave://histograms")
            }
            if #available(iOS 16.0, *) {
              let webView = histogramsController.webView
              webView.isFindInteractionEnabled = true
              histogramsController.navigationItem.rightBarButtonItem = UIBarButtonItem(
                systemItem: .search,
                primaryAction: .init { [weak webView] _ in
                  guard let findInteraction = webView?.findInteraction,
                        !findInteraction.isFindNavigatorVisible else {
                    return
                  }
                  findInteraction.searchText = ""
                  findInteraction.presentFindNavigator(showingReplace: false)
                }
              )
            }
            self.navigationController?.pushViewController(histogramsController, animated: true)
          }, accessory: .disclosureIndicator, cellClass: MultilineValue1Cell.self),
        Row(
          text: "VPN Logs",
          selection: { [unowned self] in
            self.navigationController?.pushViewController(VPNLogsViewController(), animated: true)
          }, accessory: .disclosureIndicator, cellClass: MultilineValue1Cell.self),
        Row(
          text: "Brave Talk Logs",
          selection: { [unowned self] in
            let controller = UIHostingController(rootView: BraveTalkLogsView())
            self.navigationController?.pushViewController(controller, animated: true)
          }, accessory: .disclosureIndicator, cellClass: MultilineValue1Cell.self),
        Row(
          text: "Retention Preferences Debug Menu",
          selection: { [unowned self] in
            self.navigationController?.pushViewController(
              RetentionPreferencesDebugMenuViewController(p3aUtilities: p3aUtilities, attributionManager: attributionManager),
              animated: true)
          }, accessory: .disclosureIndicator, cellClass: MultilineValue1Cell.self),
        Row(
          text: "Load all QA Links",
          selection: { [unowned self] in
            let url = URL(string: "https://raw.githubusercontent.com/brave/qa-resources/master/testlinks.json")!
            let string = try? String(contentsOf: url)
            let urls = JSON(parseJSON: string!)["links"].arrayValue.compactMap { URL(string: $0.stringValue) }
            self.settingsDelegate?.settingsOpenURLs(urls, loadImmediately: false)
            self.dismiss(animated: true)
          }, cellClass: MultilineButtonCell.self),
        Row(
          text: "Create 1000 Tabs",
          selection: { [unowned self] in
            let urls = (0..<1000).map { URL(string: "https://search.brave.com/search?q=\($0)")! }
            self.settingsDelegate?.settingsOpenURLs(urls, loadImmediately: false)
            self.dismiss(animated: true)
          },
          cellClass: ButtonCell.self
        ),
        Row(
          text: "CRASH!!!",
          selection: { [unowned self] in
            let alert = UIAlertController(title: "Force crash?", message: nil, preferredStyle: .alert)
            alert.addAction(
              UIAlertAction(title: "Crash app", style: .destructive) { _ in
                fatalError()
              })
            alert.addAction(UIAlertAction(title: "Cancel", style: .cancel, handler: nil))
            self.present(alert, animated: true, completion: nil)
          }, cellClass: MultilineButtonCell.self),
      ]
    )
  }()

  private func setUpSections() {
    var copyOfSections = self.makeSections()
    
    if let featureSectionIndex = copyOfSections.firstIndex(where: {
      $0.uuid == self.featureSectionUUID.uuidString
    }) {
      let walletRowIndex = copyOfSections[featureSectionIndex].rows.firstIndex(where: {
        $0.uuid == self.walletRowUUID.uuidString
      })
      
      if walletRowIndex == nil {
        let settingsStore = cryptoStore?.settingsStore
        copyOfSections[featureSectionIndex].rows.append(
          Row(
            text: Strings.Wallet.web3,
            selection: { [unowned self] in
              // iOS17 memory leak issue #8160
              keyringStore?.setupObservers()
              cryptoStore?.setupObservers()
              let web3SettingsView = Web3SettingsView(
                settingsStore: settingsStore,
                networkStore: cryptoStore?.networkStore,
                keyringStore: keyringStore
              ).environment(\.openURL, .init(handler: { [weak self] url in
                guard let self = self else { return .discarded }
                (self.presentingViewController ?? self).dismiss(animated: true) { [self] in
                  self.settingsDelegate?.settingsOpenURLInNewTab(url)
                }
                return .handled
              }))
              let vc = UIHostingController(rootView: web3SettingsView)
              self.navigationController?.pushViewController(vc, animated: true)
            },
            image: UIImage(braveSystemNamed: "leo.product.brave-wallet"),
            accessory: .disclosureIndicator,
            uuid: self.walletRowUUID.uuidString)
        )
      } else if let index = walletRowIndex {
        copyOfSections.remove(at: index)
      }
    }
    self.dataSource.sections = copyOfSections
  }

  // MARK: - Actions

  private func enableVPNTapped() {
    let state = BraveVPN.vpnState

    switch state {
    case .notPurchased, .expired:
      guard let vc = state.enableVPNDestinationVC else { return }
      navigationController?.pushViewController(vc, animated: true)
    case .purchased:
      BraveVPN.reconnect()
      dismiss(animated: true)
    }
  }

  private func dismissVPNHeaderTapped() {
    if dataSource.sections.isEmpty { return }
    dataSource.sections[0] = Static.Section()
    Preferences.VPN.vpnSettingHeaderWasDismissed.value = true
  }
}
