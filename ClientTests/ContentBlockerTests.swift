// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import XCTest
import WebKit
@testable import Client

class ContentBlockerTests: XCTestCase {
    
    var store: WKContentRuleListStore!
    var contentBlocker: BlocklistName!
    let contentBlockerName = "test-content-blocker"
    
    override func setUp() {
        super.setUp()
        
        let testBundle = Bundle(for: type(of: self))
        let bundleURL = testBundle.bundleURL
        store = WKContentRuleListStore(url: bundleURL)!
        
        let cleanStoreExpectation = expectation(description: "rule list setup")
        cleanStoreExpectation.isInverted = true
        
        contentBlocker = BlocklistName(filename: contentBlockerName)
        
        store.getAvailableContentRuleListIdentifiers { ids in
            if ids?.isEmpty == false {
                cleanStoreExpectation.fulfill()
            }
        }
        
        wait(for: [cleanStoreExpectation], timeout: 1)
    }
    
    override func tearDown() {
        let ruleListsRemoved = XCTestExpectation(description: "rule lists removed")
        
        var removedRuleLists: [XCTestExpectation] = []
        
        store.getAvailableContentRuleListIdentifiers { ids in
            guard let ids = ids else { return }
            
            ids.forEach { id in
                let idExpectation = self.expectation(description: "id: \(id)")
                
                removedRuleLists.append(idExpectation)
                
                self.store.removeContentRuleList(forIdentifier: id) { error in
                    if error != nil { return }
                    idExpectation.fulfill()
                    
                }
            }
            
            ruleListsRemoved.fulfill()
        }
        
        wait(for: [ruleListsRemoved] + removedRuleLists, timeout: 2)
        
        super.tearDown()
    }

    func testCompilation() {
        let validRule = """
                [{"trigger":{"url-filter":"-pubblicita300x275\\\\."},"action":{"type":"block"}}]
                """
        
        compile(jsonString: validRule, expectSuccess: true)
    }

    func testCompilationFailure() {
        let invalidJson = "badJson content rule"
        compile(jsonString: invalidJson, expectSuccess: false)
    }
    
    func testCompilationEmptyData() {
        compile(jsonString: nil, expectSuccess: false)
    }
    
    func testGettingRegionalContentBlocker() {
        XCTAssertNil(ContentBlockerRegion.with(localeCode: nil))
        
        XCTAssertNil(ContentBlockerRegion.with(localeCode: "en"),
                     "Regional content blocker should be disabled for english locale")
        
        let validLocale = "pl"
        let valid = ContentBlockerRegion.with(localeCode: validLocale)
        XCTAssertEqual(valid?.filename, validLocale)
        
        let invalidLocale = "xx"
        XCTAssertNil(ContentBlockerRegion.with(localeCode: invalidLocale))
        
    }
    
    private func compile(jsonString json: String?, expectSuccess: Bool) {
        let data = json?.data(using: .utf8)
        let completion = contentBlocker?.compile(data: data, ruleStore: store)
        
        let exp = XCTestExpectation(description: "compile")
        exp.isInverted = !expectSuccess
        
        completion?.upon {
            exp.fulfill()
        }
        
        wait(for: [exp], timeout: 1)
        
        XCTAssert(true)
    }
}
