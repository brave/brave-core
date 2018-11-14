/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

// This file is largely verbatim from Focus iOS (Blockzilla/Lib/TrackingProtection).
// The preload and postload js files are unmodified from Focus.

import Shared
import Deferred
import Data

struct TPPageStats {
    let adCount: Int
    let trackerCount: Int
    let scriptCount: Int

    var total: Int { return adCount + trackerCount + scriptCount }
    
    init(adCount: Int = 0, trackerCount: Int = 0, scriptCount: Int = 0) {
        self.adCount = adCount
        self.trackerCount = trackerCount
        self.scriptCount = scriptCount
    }

    func create(byAddingListItem listItem: BlocklistName) -> TPPageStats {
        switch listItem {
        case .ad: return TPPageStats(adCount: adCount + 1, trackerCount: trackerCount, scriptCount: scriptCount)
        case .tracker: return TPPageStats(adCount: adCount, trackerCount: trackerCount + 1, scriptCount: scriptCount)
        case .script: return TPPageStats(adCount: adCount, trackerCount: trackerCount, scriptCount: scriptCount + 1)
        default:
            break
        }
        return self
    }
}

enum TPStatsResourceType: String {
    case script
    case image
}

class TPStatsBlocklistChecker {
    static let shared = TPStatsBlocklistChecker()

    private var blockLists: TPStatsBlocklists?

    func isBlocked(url: URL, domain: Domain, resourceType: TPStatsResourceType? = nil) -> Deferred<BlocklistName?> {
        let deferred = Deferred<BlocklistName?>()
        
        guard let blockLists = blockLists, let host = url.host, !host.isEmpty else {
            // TP Stats init isn't complete yet
            deferred.fill(nil)
            return deferred
        }
        
        DispatchQueue.global().async {
            let enabledLists = BlocklistName.blocklists(forDomain: domain).on
            if let resourceType = resourceType {
                switch resourceType {
                case .script:
                    if enabledLists.contains(.script) {
                        deferred.fill(.script)
                        return
                    }
                case .image:
                    if enabledLists.contains(.image) {
                        deferred.fill(.image)
                        return
                    }
                }
            }
            if let blocklist = blockLists.blocklistForURL(url), enabledLists.contains(blocklist) {
                deferred.fill(blocklist)
            } else {
                deferred.fill(nil)
            }
        }
        return deferred
    }

    func startup() -> Deferred<()> {
        let deferred = Deferred<()>()
        DispatchQueue.global().async {
            let parser = TPStatsBlocklists()
            parser.load()
            DispatchQueue.main.async {
                self.blockLists = parser
                deferred.fill(())
            }
        }
        return deferred
    }
}

// The 'unless-domain' and 'if-domain' rules use wildcard expressions, convert this to regex.
func wildcardContentBlockerDomainToRegex(domain: String) -> NSRegularExpression? {
    struct Memo { static var domains =  [String: NSRegularExpression]() }
    if let memoized = Memo.domains[domain] {
        return memoized
    }

    // Convert the domain exceptions into regular expressions.
    var regex = domain + "$"
    if regex.first == "*" {
        regex = "." + regex
    }
    regex = regex.replacingOccurrences(of: ".", with: "\\.")
    do {
        let result = try NSRegularExpression(pattern: regex, options: [])
        Memo.domains[domain] = result
        return result
    } catch {
        assertionFailure("Blocklists: \(error.localizedDescription)")
        return nil
    }
}

fileprivate class TPStatsBlocklists {
    class Rule {
        let regex: NSRegularExpression
        let loadType: LoadType
        let resourceType: ResourceType
        let domainExceptions: [NSRegularExpression]?
        let list: BlocklistName

        init(regex: NSRegularExpression, loadType: LoadType, resourceType: ResourceType, domainExceptions: [NSRegularExpression]?, list: BlocklistName) {
            self.regex = regex
            self.loadType = loadType
            self.resourceType = resourceType
            self.domainExceptions = domainExceptions
            self.list = list
        }
    }

    private var blockRules = [String: [Rule]]()

    enum LoadType {
        case all
        case thirdParty
    }

    enum ResourceType {
        case all
        case font
    }

    func load() {
        // All rules have this prefix on the domain to match.
        let standardPrefix = "^https?://([^/]+\\.)?"
        
        for blockList in BlocklistName.allLists {
            let list: [[String: AnyObject]]
            do {
                guard let path = Bundle.main.path(forResource: blockList.filename, ofType: "json") else {
                    assertionFailure("Blocklists: bad file path.")
                    return
                }

                let json = try Data(contentsOf: URL(fileURLWithPath: path))
                guard let data = try JSONSerialization.jsonObject(with: json, options: []) as? [[String: AnyObject]] else {
                    assertionFailure("Blocklists: bad JSON cast.")
                    return
                }
                list = data
            } catch {
                assertionFailure("Blocklists: \(error.localizedDescription)")
                return
            }

            for rule in list {
                guard let trigger = rule["trigger"] as? [String: AnyObject],
                    let filter = trigger["url-filter"] as? String,
                    let filterRegex = try? NSRegularExpression(pattern: filter, options: []) else {
                        assertionFailure("Blocklists error: Rule has unexpected format.")
                        continue
                }

                guard let loc = filter.range(of: standardPrefix) else {
                    continue
                }
                let baseDomain = String(filter.suffix(from: loc.upperBound)).replacingOccurrences(of: "\\.", with: ".")
                assert(!baseDomain.isEmpty)

                // Sanity check for the lists.
                ["*", "?", "+"].forEach { x in
                    // This will only happen on debug
                    assert(!baseDomain.contains(x), "No wildcards allowed in baseDomain")
                }

                let domainExceptionsRegex = (trigger["unless-domain"] as? [String])?.compactMap { domain in
                    return wildcardContentBlockerDomainToRegex(domain: domain)
                }

                // Only "third-party" is supported; other types are not used in our block lists.
                let loadTypes = trigger["load-type"] as? [String] ?? []
                let loadType = loadTypes.contains("third-party") ? LoadType.thirdParty : .all

                // Only "font" is supported; other types are not used in our block lists.
                let resourceTypes = trigger["resource-type"] as? [String] ?? []
                let resourceType = resourceTypes.contains("font") ? ResourceType.font : .all

                let rule = Rule(regex: filterRegex, loadType: loadType, resourceType: resourceType, domainExceptions: domainExceptionsRegex, list: blockList)
                blockRules[baseDomain] = (blockRules[baseDomain] ?? []) + [rule]
            }
        }
    }

    func blocklistForURL(_ url: URL) -> BlocklistName? {
        let resourceString = url.absoluteString
        let resourceRange = NSRange(location: 0, length: resourceString.count)

        guard let baseDomain = url.baseDomain, let rules = blockRules[baseDomain] else {
            return nil
        }

        domainSearch: for rule in rules {
            // First, test the top-level filters to see if this URL might be blocked.
            if rule.regex.firstMatch(in: resourceString, options: .anchored, range: resourceRange) != nil {
                // Check the domain exceptions. If a domain exception matches, this filter does not apply.
                for domainRegex in (rule.domainExceptions ?? []) {
                    if domainRegex.firstMatch(in: resourceString, options: [], range: resourceRange) != nil {
                        continue domainSearch
                    }
                }
                return rule.list
            }
        }

        return nil
    }
}
