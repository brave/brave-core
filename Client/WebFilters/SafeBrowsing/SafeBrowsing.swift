// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import WebKit
import Shared
import BraveShared
import Data

private let log = Logger.browserLogger

class SafeBrowsing {
    let listNames = MalwareList.generate()
    
    private(set) var domainList: Set<String>
    
    init(domainList: Set<String> = Set<String>()) {
        self.domainList = domainList
        
        if domainList.isEmpty {
            fillDomainList()
        }
    }
    
    func shouldBlock(_ url: URL) -> Bool {
        guard let baseDomain = url.baseDomain else {
            log.error("url: \(url) host is nil")
            return false
        }
        
        let safeBrowsingLocalShield = Domain.getBraveShield(forUrl: url, shield: .SafeBrowsing,
                                                            context: DataController.viewContext)
        let isGlobalShieldOn = Preferences.Shields.blockPhishingAndMalware.value
        
        let isSafeBrowsingEnabled = Bool(truncating: (safeBrowsingLocalShield ?? NSNumber(value: isGlobalShieldOn)))
        let isUrlBlacklisted = domainList.contains(baseDomain)

        return isSafeBrowsingEnabled && isUrlBlacklisted
    }
    
    func showMalwareWarningPage(forUrl url: URL, inWebView webView: WKWebView) {
        var components = URLComponents(string: WebServer.sharedInstance.base + "/errors/SafeBrowsingError.html")!
        
        // This parameter help us to know what url caused the malware protection to show so we can
        // update url with it(instead of showing localhost error page url).
        components.queryItems = [URLQueryItem(name: "url", value: url.absoluteString)]
        
        webView.load(PrivilegedRequest(url: components.url!) as URLRequest)
    }
    
    private func fillDomainList() {
        DispatchQueue.global(qos: .background).async {
            var newList = Set<String>()
            
            self.listNames.forEach {
                if let list = self.openList(withName: $0.name) {
                    newList.formUnion(self.parse(list, regex: $0.regex))
                }
            }
            
            DispatchQueue.main.async {
                log.info("Safe browsing list was filled with \(newList.count) records")
                self.domainList = newList
            }
        }
    }
    
    private func openList(withName name: String) -> String? {
        guard let filePath = Bundle.main.path(forResource: name, ofType: "txt") else {
            log.error("Could not find text file with :\(name) name")
            return nil
        }

        do {
            return try String(contentsOfFile: filePath, encoding: .utf8)
        } catch {
            log.error("Could not open safe browsing list file with :\(name) name, error: \(error)")
            return nil
        }
    }
    
    private func parse(_ list: String, regex: NSRegularExpression?) -> Set<String> {
        var domains = Set<String>()
        
        guard let regex = regex else {
            domains.formUnion(list.components(separatedBy: .newlines))
            return domains
        }
        
        let nsList = list as NSString

        regex.matches(in: list, options: [], range: NSRange(location: 0, length: nsList.length))
            .map {
                nsList.substring(with: $0.range)
            }.forEach {
                domains.insert($0)
        }
        
        return domains
    }
}
