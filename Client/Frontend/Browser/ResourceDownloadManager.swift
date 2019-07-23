// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import WebKit
import Data
import BraveShared

struct DownloadedResourceResponse: Decodable {
    let statusCode: Int
    let data: Data?
    
    static func from(message: WKScriptMessage) throws -> DownloadedResourceResponse? {
        let data = try JSONSerialization.data(withJSONObject: message.body, options: .prettyPrinted)
        return try JSONDecoder().decode(DownloadedResourceResponse.self, from: data)
    }
    
    init(from decoder: Decoder) throws {
        let container = try decoder.container(keyedBy: CodingKeys.self)
        self.statusCode = try container.decode(Int.self, forKey: .statusCode)
        self.data = Data(base64Encoded: try container.decode(String.self, forKey: .base64Data))
    }
    
    private enum CodingKeys: String, CodingKey {
        case statusCode
        case base64Data
    }
}

class ResourceDownloadManager: TabContentScript {
    fileprivate weak var tab: Tab?
    
    init(tab: Tab) {
        self.tab = tab
    }
    
    static func name() -> String {
        return "ResourceDownloadManager"
    }
    
    func scriptMessageHandlerName() -> String? {
        return "resourceDownloadManager"
    }
    
    func userContentController(_ userContentController: WKUserContentController, didReceiveScriptMessage message: WKScriptMessage) {
        
        do {
            let response = try DownloadedResourceResponse.from(message: message)
            tab?.temporaryDocument?.onDocumentDownloaded(document: response, error: nil)
        } catch {
            tab?.temporaryDocument?.onDocumentDownloaded(document: nil, error: error)
        }
    }
    
    static func downloadResource(for tab: Tab, url: URL) {        
        let token = UserScriptManager.securityToken.uuidString.replacingOccurrences(of: "-", with: "", options: .literal)

        tab.webView?.evaluateJavaScript("D\(token).download(\"\(url)\");", completionHandler: { _, error in
            if let error = error {
                tab.temporaryDocument?.onDocumentDownloaded(document: nil, error: error)
            }
        })
    }
}
