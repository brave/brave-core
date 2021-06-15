// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Shared
import BraveShared
import WebKit

private let log = Logger.browserLogger

enum DomainUserScript: CaseIterable {
    case youtube
    case archive
    case braveSearch
    
    static func get(for domain: String) -> Self? {
        var found: DomainUserScript?
        
        allCases.forEach {
            if $0.associatedDomains.contains(domain) {
                found = $0
                return
            }
        }
        
        return found
    }
    
    /// Returns a shield type for a given user script domain.
    /// Returns nil if the domain's user script can't be turned off via a shield toggle.
    var shieldType: BraveShield? {
        switch self {
        case .youtube:
            return .AdblockAndTp
        case .archive, .braveSearch:
            return nil
        }
    }
    
    private var associatedDomains: Set<String> {
        switch self {
        case .youtube:
            return .init(arrayLiteral: "youtube.com")
        case .archive:
            return .init(arrayLiteral: "archive.is", "archive.today", "archive.vn", "archive.fo")
        case .braveSearch:
            return .init(arrayLiteral: "brave.com")
        }
    }
    
    private var scriptName: String {
        switch self {
        case .youtube:
            return "YoutubeAdblock"
        case .archive:
            return "ArchiveIsCompat"
        case .braveSearch:
            return "BraveSearchHelper"
        }
    }
    
    var script: WKUserScript? {
        guard let source = sourceFile else { return nil }
        
        switch self {
        case .youtube:
            // Verify that the application itself is making a call to the JS script instead of other scripts on the page.
            // This variable will be unique amongst scripts loaded in the page.
            // When the script is called, the token is provided in order to access the script variable.
            var alteredSource = source
            let token = UserScriptManager.securityToken.uuidString.replacingOccurrences(of: "-", with: "",
                                                                                        options: .literal)
            alteredSource = alteredSource.replacingOccurrences(of: "$<prunePaths>", with: "ABSPP\(token)",
                                                               options: .literal)
            alteredSource = alteredSource.replacingOccurrences(of: "$<findOwner>", with: "ABSFO\(token)",
                                                               options: .literal)
            alteredSource = alteredSource.replacingOccurrences(of: "$<setJS>", with: "ABSSJ\(token)",
                                                               options: .literal)
            
            return WKUserScript(source: alteredSource, injectionTime: .atDocumentStart, forMainFrameOnly: false)
        case .archive:
            return WKUserScript(source: source, injectionTime: .atDocumentStart, forMainFrameOnly: false)
        case .braveSearch:
            var alteredSource = source
            
            let securityToken = UserScriptManager.securityToken.uuidString
                .replacingOccurrences(of: "-", with: "", options: .literal)
            alteredSource = alteredSource
                .replacingOccurrences(of: "$<brave-search-helper>",
                                      with: "BSH\(UserScriptManager.messageHandlerTokenString)",
                                      options: .literal)
                .replacingOccurrences(of: "$<security_token>", with: securityToken)
                
            return WKUserScript(source: alteredSource, injectionTime: .atDocumentStart, forMainFrameOnly: false)
        }
    }
    
    private var sourceFile: String? {
        guard let path = Bundle.main.path(forResource: scriptName, ofType: "js"),
            let source = try? String(contentsOfFile: path) else {
            log.error("Failed to load \(scriptName).js")
            return nil
        }
        
        return source
    }
}
