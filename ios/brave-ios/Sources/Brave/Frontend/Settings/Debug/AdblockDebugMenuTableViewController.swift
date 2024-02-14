// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import Static
import WebKit
import Shared
import Preferences
import Combine
import BraveUI

class AdblockDebugMenuTableViewController: TableViewController {
  private static var fileDateFormatter: DateFormatter {
    return DateFormatter().then {
      $0.dateStyle = .short
      $0.timeStyle = .short
      $0.timeZone = TimeZone(abbreviation: "GMT")
    }
  }
  
  private let fm = FileManager.default

  override func viewDidLoad() {
    super.viewDidLoad()

    ContentBlockerManager.shared.ruleStore.getAvailableContentRuleListIdentifiers { lists in
      let listNames = lists ?? []
      if listNames.isEmpty { return }

      self.dataSource.sections = [
        self.actionsSection,
        self.datesSection,
        self.bundledListsSection(names: listNames),
        self.downloadedResourcesSection(
          header: "Ad-Block Resources",
          footer: "Files downloaded using the AdBlockResourceDownloader",
          resources: AdblockResourceDownloader.handledResources
        ),
        self.downloadedResourcesSection(
          header: "Filter list custom URLs",
          footer: "Files downloaded using the FilterListURLResourceDownloader",
          resources: CustomFilterListStorage.shared.filterListsURLs.map({ filterListURL in
            return filterListURL.setting.resource
          })
        )
      ]
    }
  }

  private var actionsSection: Section {
    var section = Section(header: .title("Actions"))
    section.rows = [
      Row(
        text: "Recompile Content Blockers",
        selection: {
          Task { @MainActor in
            let modes = ContentBlockerManager.BlockingMode.allCases
            await AdblockResourceDownloader.shared.loadCachedAndBundledDataIfNeeded(allowedModes: Set(modes))
            self.showCompiledBlockListAlert()
          }
        }, cellClass: ButtonCell.self)
    ]

    return section
  }
  
  @MainActor private func showCompiledBlockListAlert() {
    let alert = UIAlertController(title: nil, message: "Recompiled Blockers", preferredStyle: .alert)
    alert.addAction(UIAlertAction(title: "OK", style: .default))
    present(alert, animated: true)
  }

  private var datesSection: Section {
    var section = Section(
      header: "Last time updated",
      footer: "When the lists were last time updated on the device")
    var rows = [Row]()

    let dateFormatter = DateFormatter().then {
      $0.dateStyle = .short
      $0.timeStyle = .short
    }

    var generalDateString = "-"
    if let generalDate = Preferences.Debug.lastGeneralAdblockUpdate.value {
      generalDateString = dateFormatter.string(from: generalDate)
      rows.append(.init(text: "General blocklist", detailText: generalDateString))
    }

    var regionalDateString = "-"
    if let regionalDate = Preferences.Debug.lastRegionalAdblockUpdate.value {
      regionalDateString = dateFormatter.string(from: regionalDate)
      rows.append(.init(text: "Regional blocklist", detailText: regionalDateString))
    }

    var cosmeticFilterStylesDateString = "-"
    if let stylesDate = Preferences.Debug.lastCosmeticFiltersCSSUpdate.value {
      cosmeticFilterStylesDateString = dateFormatter.string(from: stylesDate)
      rows.append(.init(text: "Cosmetic Filters (CSS)", detailText: cosmeticFilterStylesDateString))
    }

    var cosmeticFilterScripletsDateString = "-"
    if let scripletsDate = Preferences.Debug.lastCosmeticFiltersCSSUpdate.value {
      cosmeticFilterScripletsDateString = dateFormatter.string(from: scripletsDate)
      rows.append(.init(text: "Cosmetic Filters (Scriptlets)", detailText: cosmeticFilterScripletsDateString))
    }

    section.rows = rows
    return section
  }

  private func bundledListsSection(names: [String]) -> Section {
    var section = Section(
      header: "Preinstalled lists",
      footer: "Lists bundled within the iOS app.")

    var rows = [Row]()

    names.forEach {
      if let bundlePath = Bundle.module.path(forResource: $0, ofType: "json") {
        guard let jsonData = fm.contents(atPath: bundlePath),
          let json = try? JSONSerialization.jsonObject(with: jsonData, options: JSONSerialization.ReadingOptions.allowFragments) as? [[String: Any]]
        else { return }

        var text = $0 + ".json"

        // Rules with count = 1 don't have to be shown, they are static rules for cookie control
        // tracking protection and httpse.
        if json.count > 1 {
          text += ": \(json.count) rules"
        }

        let hashText = "sha1: \(jsonData.sha1.hexEncodedString)"
        rows.append(.init(text: text, detailText: hashText, cellClass: ShrinkingSubtitleCell.self))
      }

      if $0 == "block-ads",
        let bundlePath = Bundle.module.path(
          forResource: "ABPFilterParserData",
          ofType: "dat"),
        let data = fm.contents(atPath: bundlePath) {
        let hashText = "sha1: \(data.sha1.hexEncodedString)"
        rows.append(.init(text: "ABPFilterParserData.dat", detailText: hashText, cellClass: ShrinkingSubtitleCell.self))
      }
    }

    section.rows = rows
    return section
  }
  
  private func downloadedResourcesSection<Resource: DownloadResourceInterface>(
    header: Section.Extremity?, footer: Section.Extremity?, resources: [Resource]
  ) -> Section {
    func createRows(from resources: [Resource]) -> [Row] {
      resources.compactMap { createRow(from: $0) }
    }
    
    func getEtag(from resource: Resource) -> String? {
      do {
        return try resource.createdEtag()
      } catch {
        return nil
      }
    }
    
    func getFileCreation(for resource: Resource) -> String? {
      do {
        guard let date = try resource.creationDate() else { return nil }
        return Self.fileDateFormatter.string(from: date)
      } catch {
        return nil
      }
    }
    
    func createRow(from resource: Resource) -> Row? {
      guard let fileURL = resource.downloadedFileURL else {
        return nil
      }
      
      let detailText = [
        "created date: \(getFileCreation(for: resource) ?? "nil")",
        "etag: \(getEtag(from: resource) ?? "nil")",
        "folder: \(fileURL.deletingLastPathComponent().path)"
      ].joined(separator: "\n")
      
      return Row(text: fileURL.lastPathComponent, detailText: detailText, cellClass: MultilineSubtitleCell.self)
    }
    
    return Section(
      header: header, rows: createRows(from: resources),
      footer: footer
    )
  }
}

fileprivate class ShrinkingSubtitleCell: SubtitleCell {

  override init(style: UITableViewCell.CellStyle, reuseIdentifier: String?) {
    super.init(style: style, reuseIdentifier: reuseIdentifier)
    textLabel?.adjustsFontSizeToFitWidth = true
    detailTextLabel?.adjustsFontSizeToFitWidth = true
  }

  required init?(coder aDecoder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }
}
