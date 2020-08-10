// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import WebKit
import Shared
import Data
import BraveShared

private let log = Logger.browserLogger

protocol ContentBlocker: class, Hashable {
    // Make constant `let
    var filename: String { get }
    var rule: WKContentRuleList? { get set }
    var fileVersionPref: Preferences.Option<String?>? { get }
    var fileVersion: String? { get }
}

extension ContentBlocker {
    func buildRule(ruleStore: WKContentRuleListStore) -> Deferred<Void> {
        let compilerDeferred = Deferred<Void>()
        
        let compileIt = needsCompiling(ruleStore: ruleStore)
        compileIt.upon { compile in
            if !compile {
                compilerDeferred.fill(())
                return
            }
            
            BlocklistName.loadJsonFromBundle(forResource: self.filename) { jsonString in
                ruleStore.compileContentRuleList(forIdentifier: self.filename, encodedContentRuleList: jsonString) { rule, error in
                    if let error = error {
                        // TODO #382: Potential telemetry location
                        log.error("Content blocker '\(self.filename)' errored: \(error.localizedDescription)")
                        assert(false)
                    }
                    assert(rule != nil)
                    
                    self.rule = rule
                    self.fileVersionPref?.value = self.fileVersion
                    compilerDeferred.fill(())
                }
            }
        }
        
        return compilerDeferred
    }
    
    private func needsCompiling(ruleStore: WKContentRuleListStore) -> Deferred<Bool> {
        let needsCompiling = Deferred<Bool>()
        if fileVersionPref?.value != fileVersion {
            // New file, so we must update the lists, no need to check the store
            needsCompiling.fill(true)
            return needsCompiling
        }
        
        ruleStore.lookUpContentRuleList(forIdentifier: self.filename) { rule, error in
            self.rule = rule
            needsCompiling.fill(self.rule == nil)
        }
        return needsCompiling
    }
    
    private static func loadJsonFromBundle(forResource file: String, completion: @escaping (_ jsonString: String) -> Void) {
        DispatchQueue.global().async {
            guard let path = Bundle.main.path(forResource: file, ofType: "json"),
                let source = try? String(contentsOfFile: path, encoding: .utf8) else {
                    assert(false)
                    return
            }
            
            DispatchQueue.main.async {
                completion(source)
            }
        }
    }
    
    public static func == (lhs: Self, rhs: Self) -> Bool {
        return lhs.filename == rhs.filename
    }
    
    public func hash(into hasher: inout Hasher) {
        hasher.combine(filename)
    }
}
