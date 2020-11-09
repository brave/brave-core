// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/. */

import Foundation
import WebKit
import Shared
import GCDWebServers
import BraveShared

private let log = Logger.browserLogger

class BookmarksInterstitialPageHandler {
    
    class func showBookmarksPage(tabManager: TabManager, url: URL) -> Bool {
        if !FileManager.default.fileExists(atPath: url.absoluteString) {
            return false
        }
        
        let components = URLComponents(string: WebServer.sharedInstance.base + "/interstitial/Bookmarks.html")!
        if let pageUrl = components.url {
            let request = PrivilegedRequest(url: pageUrl).then {
                $0.setValue("bookmarks", forHTTPHeaderField: "X-REQUEST-INTERSTITIAL-INFO")
                $0.setValue(url.absoluteString, forHTTPHeaderField: "X-REQUEST-INTERSTITIAL-URL")
            }
            
            let tab = tabManager.addTabAndSelect(isPrivate: PrivateBrowsingManager.shared.isPrivateBrowsing)
            tab.webView?.load(request as URLRequest)
            return true
        }
        return false
    }
}

extension BookmarksInterstitialPageHandler {
    static func register(_ webServer: WebServer) {
        let registerHandler = { (page: String) in
            webServer.registerHandlerForMethod("GET", module: "interstitial", resource: page, handler: { (request) -> GCDWebServerResponse? in
                guard let url = request?.url, request?.headers["X-REQUEST-INTERSTITIAL-INFO"]?.isEmpty == false else {
                    return GCDWebServerResponse(statusCode: 404)
                }
                
                return BookmarksInterstitialPageHandler.responseForURL(url, headers: request?.headers ?? [:])
            })
        }
        
        webServer.registerMainBundleResource("Bookmarks.html", module: "interstitial")
        registerHandler("Bookmarks.html")
    }
    
    private static func responseForURL(_ url: URL, headers: [String: String]) -> GCDWebServerResponse? {
        guard let key = headers["X-REQUEST-INTERSTITIAL-INFO"] else {
            return GCDWebServerResponse(statusCode: 404)
        }
        
        switch key {
        case "bookmarks":
            guard let urlInfo = headers["X-REQUEST-INTERSTITIAL-URL"], let url = URL(string: urlInfo) else {
                return GCDWebServerResponse(statusCode: 404)
            }
            
            guard let html = try? String(contentsOfFile: url.absoluteString, encoding: .utf8) else {
                return GCDWebServerResponse(statusCode: 404)
            }
            
            let variables = [
                "bookmarks_tab_title": Strings.Sync.v2MigrationInterstitialTitle,
                "bookmarks_page_description": Strings.Sync.v2MigrationInterstitialPageDescription,
                "bookmarks_file_url": html
            ]
            
            let asset = Bundle.main.path(forResource: "Bookmarks", ofType: "html")
            return buildResponse(asset: asset, variables: variables)
        default:
            return GCDWebServerResponse(statusCode: 404)
        }
    }
    
    private static func buildResponse(asset: String?, variables: [String: String]) -> GCDWebServerResponse? {
        guard let unwrappedAsset = asset else {
            log.error("Asset is nil")
            return GCDWebServerResponse(statusCode: 404)
        }
        
        let response = GCDWebServerDataResponse(htmlTemplate: unwrappedAsset, variables: variables)
        response?.setValue("no cache", forAdditionalHeader: "Pragma")
        response?.setValue("no-cache,must-revalidate", forAdditionalHeader: "Cache-Control")
        response?.setValue(Date().description, forAdditionalHeader: "Expires")
        return response
    }
}
