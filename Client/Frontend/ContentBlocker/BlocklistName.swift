// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import WebKit
import Shared
import Deferred
import Data
import BraveShared

private let log = Logger.browserLogger

class BlocklistName: Hashable, CustomStringConvertible, ContentBlocker {
    
    static let ad = BlocklistName(filename: "block-ads")
    static let tracker = BlocklistName(filename: "block-trackers")
    static let https = BlocklistName(filename: "upgrade-http")
    static let image = BlocklistName(filename: "block-images")
    
    static var allLists: Set<BlocklistName> { return [.ad, .tracker, .https, .image] }
    
    let filename: String
    var rule: WKContentRuleList?
    
    init(filename: String) {
        self.filename = filename
    }
    
    var description: String {
        return "<\(type(of: self)): \(self.filename)>"
    }
    
    lazy var fileVersionPref: Preferences.Option<String?>? = {
        let prefMap = [BlocklistName.ad: Preferences.BlockFileVersion.adblock]
        return prefMap[self]
    }()
    
    lazy var fileVersion: String? = {
        let adVersion = Bundle.main.object(forInfoDictionaryKey: "CFBundleVersion") as? String
        return self == .ad ? adVersion : nil
    }()
    
    static func blocklists(forDomain domain: Domain) -> (on: Set<BlocklistName>, off: Set<BlocklistName>) {
        if domain.shield_allOff == 1 {
            return ([], allLists)
        }
        
        var onList = Set<BlocklistName>()
        
        if domain.isShieldExpected(.AdblockAndTp) {
            onList.formUnion([.ad, .tracker])
        }
        
        // For lists not implemented, always return exclude from `onList` to prevent accidental execution
        
        // TODO #159: Setup image shield
        
        if domain.isShieldExpected(.HTTPSE) {
            onList.formUnion([.https])
        }
        
        return (onList, allLists.subtracting(onList))
    }
    
    static func compileAll(ruleStore: WKContentRuleListStore) -> Deferred<Void> {
        let allCompiledDeferred = Deferred<Void>()
        let allOfThem = BlocklistName.allLists.map {
            $0.buildRule(ruleStore: ruleStore)
        }
        
        all(allOfThem).upon { _ in
            allCompiledDeferred.fill(())
        }
        
        return allCompiledDeferred
    }
    
    public static func == (lhs: BlocklistName, rhs: BlocklistName) -> Bool {
        return lhs.filename == rhs.filename
    }
    
    public func hash(into hasher: inout Hasher) {
        hasher.combine(filename)
    }
}
