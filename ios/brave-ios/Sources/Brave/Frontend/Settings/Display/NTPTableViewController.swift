// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShared
import BraveUI
import Foundation
import Preferences
import Shared
import Static

class NTPTableViewController: TableViewController {
  enum BackgroundType: RepresentableOptionType {

    case defaultImages
    case sponsoredImages
    case sponsoredImagesAndVideos(String)
    case superReferrer(String)

    var key: String {
      displayString
    }

    public var displayString: String {
      switch self {
      case .defaultImages: return "\(Strings.NTP.settingsDefaultImagesOnly)"
      case .sponsoredImages: return Strings.NTP.settingsSponsoredImagesSelection
      case .sponsoredImagesAndVideos(let displayString): return displayString
      case .superReferrer(let referrer): return referrer
      }
    }
  }

  private let rewards: BraveRewards?

  init(_ rewards: BraveRewards?) {
    self.rewards = rewards

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
          option: Preferences.NewTabPage.backgroundImages
        )
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
          option: Preferences.NewTabPage.showNewTabPrivacyHub
        ),
        .boolRow(
          title: Strings.Widgets.favoritesWidgetTitle,
          option: Preferences.NewTabPage.showNewTabFavourites
        ),
      ]
    )

    dataSource.sections = [imageSection, widgetSection]
  }

  private func selectedItem() -> BackgroundType {
    if let referrer = Preferences.NewTabPage.selectedCustomTheme.value {
      return .superReferrer(referrer)
    }

    switch Preferences.NewTabPage.backgroundMediaType {
    case .defaultImages:
      return BackgroundType.defaultImages
    case .sponsoredImages:
      return BackgroundType.sponsoredImages
    case .sponsoredImagesAndVideos:
      return rewards?.ads.shouldShowSponsoredImagesAndVideosSetting() == true
        ? BackgroundType.sponsoredImagesAndVideos(
          Strings.NTP.settingsSponsoredImagesAndVideosSelection
        )
        : BackgroundType.sponsoredImagesAndVideos(Strings.NTP.settingsSponsoredImagesSelection)
    }
  }

  private func backgroundImageOptions() -> [BackgroundType] {
    var available: [BackgroundType] = [.defaultImages]
    if rewards?.ads.shouldShowSponsoredImagesAndVideosSetting() == true {
      available += [
        .sponsoredImages,
        .sponsoredImagesAndVideos(Strings.NTP.settingsSponsoredImagesAndVideosSelection),
      ]
    } else {
      available += [.sponsoredImagesAndVideos(Strings.NTP.settingsSponsoredImagesSelection)]
    }

    available += Preferences.NewTabPage.installedCustomThemes.value.map {
      .superReferrer($0)
    }
    return available
  }

  private func backgroundImagesSetting(section: Section) -> Row {
    var row = Row(
      text: Strings.NTP.settingsBackgroundImageSubMenu,
      detailText: selectedItem().displayString,
      accessory: .disclosureIndicator,
      cellClass: Value1Cell.self
    )

    row.selection = { [unowned self] in
      // Show options for tab bar visibility
      let optionsViewController = OptionSelectionViewController<BackgroundType>(
        headerText: Strings.NTP.settingsBackgroundImageSubMenu,
        footerText: Strings.NTP.imageTypeSelectionDescription,
        style: .insetGrouped,
        options: self.backgroundImageOptions(),
        selectedOption: self.selectedItem(),
        optionChanged: { _, option in
          // Should turn this off whenever possible to prevent unnecessary resource downloading
          switch option {
          case .defaultImages:
            Preferences.NewTabPage.backgroundMediaType = .defaultImages
            Preferences.NewTabPage.selectedCustomTheme.value = nil
          case .sponsoredImages:
            Preferences.NewTabPage.backgroundMediaType = .sponsoredImages
            Preferences.NewTabPage.selectedCustomTheme.value = nil
          case .sponsoredImagesAndVideos:
            Preferences.NewTabPage.backgroundMediaType = .sponsoredImagesAndVideos
            Preferences.NewTabPage.selectedCustomTheme.value = nil
          case .superReferrer(let referrer):
            Preferences.NewTabPage.selectedCustomTheme.value = referrer
            Preferences.NewTabPage.backgroundMediaType = .sponsoredImagesAndVideos
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
