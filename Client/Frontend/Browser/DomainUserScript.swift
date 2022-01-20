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
    case braveTalk
    
    static func get(for url: URL) -> Self? {
        var found: DomainUserScript?
        
        // First we look for exact domain match, if no matches we look for base domain matches.
        guard let host = url.host else { return nil }
        allCases.forEach {
            if $0.associatedDomains.contains(host) {
                found = $0
                return
            }
        }
        
        if found != nil { return found }
        
        guard let baseDomain = url.baseDomain else { return nil }
        allCases.forEach {
            if $0.associatedDomains.contains(baseDomain) {
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
        case .archive, .braveSearch, .braveTalk:
            return nil
        }
    }
    
    var associatedDomains: Set<String> {
        switch self {
        case .youtube:
            return .init(arrayLiteral: "youtube.com")
        case .archive:
            return .init(arrayLiteral: "archive.is", "archive.today", "archive.vn", "archive.fo")
        case .braveSearch:
            return .init(arrayLiteral: "search.brave.com", "search-dev.brave.com")
        case .braveTalk:
            return .init(arrayLiteral: "talk.brave.com", "beta.talk.brave.com",
                         "talk.bravesoftware.com", "beta.talk.bravesoftware.com",
                         "dev.talk.brave.software", "beta.talk.brave.software",
                         "talk.brave.software")
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
        case .braveTalk:
            return "BraveTalkHelper"
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
            let token = UserScriptManager.securityTokenString
            alteredSource = alteredSource.replacingOccurrences(of: "$<prunePaths>", with: "ABSPP\(token)",
                                                               options: .literal)
            alteredSource = alteredSource.replacingOccurrences(of: "$<findOwner>", with: "ABSFO\(token)",
                                                               options: .literal)
            alteredSource = alteredSource.replacingOccurrences(of: "$<setJS>", with: "ABSSJ\(token)",
                                                               options: .literal)
            
            return WKUserScript(source: alteredSource, injectionTime: .atDocumentStart, forMainFrameOnly: false, in: .page)
        case .archive:
            return WKUserScript(source: source, injectionTime: .atDocumentStart, forMainFrameOnly: false, in: .page)
        case .braveSearch:
            var alteredSource = source
            
            let securityToken = UserScriptManager.securityTokenString
            alteredSource = alteredSource
                .replacingOccurrences(of: "$<brave-search-helper>",
                                      with: "BSH\(UserScriptManager.messageHandlerTokenString)",
                                      options: .literal)
                .replacingOccurrences(of: "$<security_token>", with: securityToken)
                
            return WKUserScript(source: alteredSource, injectionTime: .atDocumentStart, forMainFrameOnly: false, in: .page)
        case .braveTalk:
            var alteredSource = source
            
            let securityToken = UserScriptManager.securityTokenString
            alteredSource = alteredSource
                .replacingOccurrences(of: "$<brave-talk-helper>",
                                      with: "BT\(UserScriptManager.messageHandlerTokenString)",
                                      options: .literal)
                .replacingOccurrences(of: "$<security_token>", with: securityToken)
                
            return WKUserScript(source: alteredSource, injectionTime: .atDocumentStart, forMainFrameOnly: false, in: .page)
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
