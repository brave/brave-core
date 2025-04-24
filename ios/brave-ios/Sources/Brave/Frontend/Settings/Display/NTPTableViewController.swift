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
import Web

private struct FooterViewUX {
  static let font = UIFont.preferredFont(forTextStyle: .footnote)
  static let textColor = UIColor.braveLabel
  static let linkUnderlineStyle = 1
  static let textAlignment = NSTextAlignment.left
  static let insets = UIEdgeInsets(top: 10, left: 15, bottom: 10, right: 15)
}

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
  var linkTapped: ((URLRequest) -> Void)?

  init(rewards: BraveRewards?, linkTapped: ((URLRequest) -> Void)?) {
    self.rewards = rewards
    self.linkTapped = linkTapped

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

  private func imageTypeSelectionDescription() -> UIView {
    let footerLabel = LinkLabel().then {
      $0.font = FooterViewUX.font
      $0.textColor = FooterViewUX.textColor
      $0.linkColor = FooterViewUX.textColor
      $0.linkUnderlineStyle = FooterViewUX.linkUnderlineStyle
      $0.textAlignment = FooterViewUX.textAlignment
      $0.text =
        "\(Strings.NTP.imageTypeSelectionDescription) \(Strings.NTP.imageTypeSelectionDescriptionLearnMore)"
      $0.setURLInfo([
        Strings.NTP.imageTypeSelectionDescriptionLearnMore: "learn-more"
      ])
    }

    footerLabel.onLinkedTapped = { [unowned self] link in
      self.dismiss(animated: true) {
        switch link.absoluteString {
        case "learn-more":
          self.linkTapped?(URLRequest(url: .brave.newTabTakeoverLearnMoreLinkUrl))
        default:
          assertionFailure()
        }
      }
    }

    let footerView = UITableViewHeaderFooterView().then {
      $0.contentView.addSubview(footerLabel)
      footerLabel.snp.makeConstraints {
        $0.edges.equalToSuperview().inset(FooterViewUX.insets)
      }
      $0.isUserInteractionEnabled = true
    }
    return footerView
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
        footerView: self.imageTypeSelectionDescription(),
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
