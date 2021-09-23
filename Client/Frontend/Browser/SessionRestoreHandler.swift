/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import WebKit
import GCDWebServers
import Shared

/// Handles requests to /about/sessionrestore to restore session history.
struct SessionRestoreHandler {
    static func register(_ webServer: WebServer) {
        // Register the handler that accepts /about/sessionrestore?history=...&currentpage=... requests.
        webServer.registerHandlerForMethod("GET", module: "about", resource: "sessionrestore") { request in
            
            // Session Restore should only ever be called from a privileged request
            // This is because we need to inject the `RESTORE_TOKEN` into the page.
            // Therefore, we don't want to leak it to a malicious page
            guard let query = request?.query,
                      query[PrivilegedRequest.key] == PrivilegedRequest.token else {
                return GCDWebServerResponse(statusCode: 404)
            }
            
            if let sessionRestorePath = Bundle.main.path(forResource: "SessionRestore", ofType: "html") {
                do {
                    var sessionRestoreString = try String(contentsOfFile: sessionRestorePath)

                    defer {
                        NotificationCenter.default.post(name: .didRestoreSession, object: self)
                    }

                    let securityToken = UserScriptManager.messageHandlerToken.uuidString
                    sessionRestoreString = sessionRestoreString.replacingOccurrences(of: "%SECURITY_TOKEN%", with: securityToken, options: .literal)
                    sessionRestoreString = sessionRestoreString.replacingOccurrences(of: "%SESSION_RESTORE_KEY%", with: PrivilegedRequest.key, options: .literal)
                    sessionRestoreString = sessionRestoreString.replacingOccurrences(of: "%SESSION_RESTORE_TOKEN%", with: PrivilegedRequest.token, options: .literal)

                    return GCDWebServerDataResponse(html: sessionRestoreString)
                } catch _ {}
            }

            return GCDWebServerResponse(statusCode: 404)
        }
    }
}
