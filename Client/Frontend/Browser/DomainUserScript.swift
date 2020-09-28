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
        case .archive:
            return nil
        }
    }
    
    private var associatedDomains: Set<String> {
        switch self {
        case .youtube:
            return .init(arrayLiteral: "youtube.com")
        case .archive:
            return .init(arrayLiteral: "archive.is", "archive.today", "archive.vn", "archive.fo")
        }
    }
    
    private var scriptName: String {
        switch self {
        case .youtube:
            return "YoutubeAdblock"
        case .archive:
            return "ArchiveIsCompat"
        }
    }
    
    var script: WKUserScript? {
        switch self {
        case .youtube:
            guard let source = sourceFile else { return nil }
            
            //Verify that the application itself is making a call to the JS script instead of other scripts on the page.
            //This variable will be unique amongst scripts loaded in the page.
            //When the script is called, the token is provided in order to access the script variable.
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
            guard let source = sourceFile else { return nil }
            return WKUserScript(source: source, injectionTime: .atDocumentStart, forMainFrameOnly: false)
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
