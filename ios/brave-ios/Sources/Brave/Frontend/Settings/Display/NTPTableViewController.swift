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
  static let horizontalPadding: CGFloat = 15
  static let verticalPadding: CGFloat = 6
}

class NTPTableViewController: TableViewController, UITextViewDelegate {
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
  private let tabManager: TabManager?
  private let linkUrl = "https://support.brave.com/hc/en-us/articles/35182999599501"

  init(rewards: BraveRewards?, tabManager: TabManager?) {
    self.rewards = rewards
    self.tabManager = tabManager

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
    let label = UITextView()
    label.textAlignment = .left
    label.textColor = .braveLabel
    label.font = FooterViewUX.font
    label.backgroundColor = .clear
    label.isEditable = false
    label.isScrollEnabled = false
    label.isSelectable = true
    label.delegate = self
    label.textContainerInset = UIEdgeInsets(
      top: FooterViewUX.verticalPadding,
      left: FooterViewUX.horizontalPadding,
      bottom: FooterViewUX.verticalPadding,
      right: FooterViewUX.horizontalPadding
    )

    let learnMoreText = Strings.NTP.imageTypeSelectionDescriptionLearnMore
    let attributes: [NSAttributedString.Key: Any] = [
      .foregroundColor: UIColor.braveLabel,
      .font: FooterViewUX.font,
    ]

    let linkAttributes: [NSAttributedString.Key: Any] = [
      .font: FooterViewUX.font,
      .foregroundColor: UIColor.braveLabel,
      .underlineStyle: 1,
    ]
    label.linkTextAttributes = linkAttributes

    let nsLabelAttributedString = NSMutableAttributedString(
      string: Strings.NTP.imageTypeSelectionDescription + " ",
      attributes: attributes
    )
    let nsLinkAttributedString = NSMutableAttributedString(
      string: learnMoreText,
      attributes: linkAttributes
    )

    if let url = URL(string: self.linkUrl) {
      let linkTextRange = NSRange(location: 0, length: learnMoreText.count)
      nsLinkAttributedString.addAttribute(.link, value: url, range: linkTextRange)
      nsLabelAttributedString.append(nsLinkAttributedString)
      label.isUserInteractionEnabled = true
    }
    label.attributedText = nsLabelAttributedString

    return label
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

  func textView(
    _ textView: UITextView,
    shouldInteractWith url: URL,
    in characterRange: NSRange,
    interaction: UITextItemInteraction
  ) -> Bool {
    self.tabManager?.addTabAndSelect(
      URLRequest(url: URL(string: self.linkUrl)!),
      isPrivate: false
    )
    self.dismiss(animated: true)
    return false
  }
}

extension NTPTableViewController: PreferencesObserver {
  func preferencesDidChange(for key: String) {
    loadSections()
  }
}
