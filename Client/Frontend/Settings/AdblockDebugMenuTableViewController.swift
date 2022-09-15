// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import Static
import WebKit
import Shared
import BraveShared
import Combine
import BraveUI

private let log = Logger.browserLogger

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
        self.downloadedResourcesSection()
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
            await ContentBlockerManager.shared.compilePendingResources()
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
      if let bundlePath = Bundle.current.path(forResource: $0, ofType: "json") {
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
        let bundlePath = Bundle.current.path(
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
  
  private func downloadedResourcesSection() -> Section {
    func createRows(from resources: [ResourceDownloader.Resource]) -> [Row] {
      resources.compactMap { createRow(from: $0) }
    }
    
    func getEtag(from resource: ResourceDownloader.Resource) -> String? {
      do {
        return try ResourceDownloader.etag(for: resource)
      } catch {
        return nil
      }
    }
    
    func getFileCreation(for resource: ResourceDownloader.Resource) -> String? {
      do {
        guard let date = try ResourceDownloader.creationDate(for: resource) else { return nil }
        return Self.fileDateFormatter.string(from: date)
      } catch {
        return nil
      }
    }
    
    func createRow(from resource: ResourceDownloader.Resource) -> Row? {
      guard let fileURL = ResourceDownloader.downloadedFileURL(for: resource) else {
        return nil
      }
      
      let detailText = [
        "created date: \(getFileCreation(for: resource) ?? "nil")",
        "etag: \(getEtag(from: resource) ?? "nil")",
        "folder: \(fileURL.deletingLastPathComponent().path)"
      ].joined(separator: "\n")
      
      return Row(text: fileURL.lastPathComponent, detailText: detailText, cellClass: MultilineSubtitleCell.self)
    }
    
    var resources = FilterListResourceDownloader.shared.filterLists.flatMap { filterList -> [ResourceDownloader.Resource] in
      return filterList.resources
    }
    
    resources.append(contentsOf: [
      .debounceRules, .genericContentBlockingBehaviors, .genericFilterRules, .generalCosmeticFilters, .generalScriptletResources
    ])
    
    return Section(
      header: "Downloaded resources",
      rows: createRows(from: resources),
      footer: "Lists downloaded from the internet at app launch using the ResourceDownloader."
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
