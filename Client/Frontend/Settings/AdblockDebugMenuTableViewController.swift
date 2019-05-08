// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import Static
import WebKit
import Deferred
import Shared
import BraveShared

private let log = Logger.browserLogger

class AdblockDebugMenuTableViewController: TableViewController {

    override func viewDidLoad() {
        super.viewDidLoad()
        
        listSection.uponQueue(.main) { list in
            self.dataSource.sections = [self.actionsSection, list]
        }
    }

    var actionsSection: Section {
        var section = Section(header: .title("Actions"))
        section.rows = [
            Row(text: "Recompile Content Blockers", selection: { [weak self] in
                BlocklistName.allLists.forEach { $0.fileVersionPref?.value = nil }
                ContentBlockerHelper.compileBundledLists().upon { _ in
                    let alert = UIAlertController(title: nil, message: "Recompiled Blockers", preferredStyle: .alert)
                    alert.addAction(UIAlertAction(title: "OK", style: .default))
                    self?.present(alert, animated: true)
                }
                }, cellClass: ButtonCell.self)
        ]
        
        return section
    }
    
    var listSection: Deferred<Section> {
        let completion = Deferred<Section>()
        
        let footerNote = """
            Lists without rule count below them are single rule lists to do a certain action \
            on a website(block cookies, images, upgrade to https etc.)
            """
        
        var section = Section(header: .title("Available lists"), footer: .title(footerNote))
        guard let store = WKContentRuleListStore.default() else { return completion }
        
        var rows = [Row]()
        
        func bundleOrDocumentsData(for name: String) -> Data? {
            let fm = FileManager.default
            
            // Search in bundle
            if let bundlePath = Bundle.main.path(forResource: name, ofType: "json") {
                return fm.contents(atPath: bundlePath)
            }
            
            // Search in documents directory otherwise
            let folderName = AdblockResourceDownloader.folderName
            guard let folderUrl = fm.getOrCreateFolder(name: folderName) else { return nil }
            
            let fileUrl = folderUrl.appendingPathComponent(name + ".json")
            return fm.contents(atPath: fileUrl.path)
        }
        
        func getEtag(name: String) -> String? {
            let fm = FileManager.default
            
            guard let folderUrl = fm.getOrCreateFolder(name: AdblockResourceDownloader.folderName) else {
                return nil
            }
            let etagUrl = folderUrl.appendingPathComponent(name + ".json.etag")
            guard let data = fm.contents(atPath: etagUrl.path) else { return nil }
            return String(data: data, encoding: .utf8)
        }
        
        store.getAvailableContentRuleListIdentifiers { list in
            list?.forEach {
                var row = Row(text: $0, cellClass: ShrinkingSubtitleCell.self)
                
                if let data = bundleOrDocumentsData(for: $0) {
                    do {
                        let json = try JSONSerialization.jsonObject(with: data, options: JSONSerialization.ReadingOptions.allowFragments) as? [[String: Any]]
                        
                        // Rules with count 1 don't have to be shown, they are static rules for cookie control
                        // tracking protection and httpse.
                        if let count = json?.count, count != 1 {
                            var detailText = "Rules count: \(count)"
                            
                            if let etag = getEtag(name: $0) {
                                detailText += ", etag: \(etag)"
                            }
                            
                            row.detailText = detailText
                        }
                    } catch {
                        log.error(error)
                        row.detailText = "Failed to get rule count for: \($0)"
                    }
                } else {
                    row.detailText = "Failed to get rule count for: \($0)"
                }
                
                rows.append(row)
            }
            
            section.rows = rows
            completion.fill(section)
        }
        
        return completion
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
