// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Static
import BraveShared
import Preferences
import Shared
import BraveCore
import BraveUI

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
      case .defaultImages: return "\(Strings.NTP.settingsDefaultImagesOnly)"
      case .sponsored: return Strings.NTP.settingsSponsoredImagesSelection
      case .superReferrer(let referrer): return referrer
      }
    }
  }

  init() {
    super.init(style: .insetGrouped)
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
    var imageSection = Section(
      header: .title(Strings.NTP.settingsBackgroundImages.uppercased()),
      rows: [
        .boolRow(
          title: Strings.NTP.settingsBackgroundImages,
          option: Preferences.NewTabPage.backgroundImages)
      ]
    )

    if Preferences.NewTabPage.backgroundImages.value {
      imageSection.rows.append(backgroundImagesSetting(section: imageSection))
    }
    
    let widgetSection = Section(
      header: .title(Strings.Widgets.widgetTitle.uppercased()),
      rows: [
        .boolRow(
          title: Strings.PrivacyHub.privacyReportsTitle,
          option: Preferences.NewTabPage.showNewTabPrivacyHub),
        .boolRow(
          title: Strings.Widgets.favoritesWidgetTitle,
          option: Preferences.NewTabPage.showNewTabFavourites)
      ]
    )

    dataSource.sections = [imageSection, widgetSection]
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
      cellClass: Value1Cell.self)

    row.selection = { [unowned self] in
      // Show options for tab bar visibility
      let optionsViewController = OptionSelectionViewController<BackgroundImageType>(
        headerText: Strings.NTP.settingsBackgroundImageSubMenu,
        footerText: Strings.NTP.imageTypeSelectionDescription,
        style: .insetGrouped,
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
