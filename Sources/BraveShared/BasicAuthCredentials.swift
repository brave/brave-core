// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import UIKit
import BraveCore
import os.log

public class BasicAuthCredentialsManager: NSObject, URLSessionDataDelegate {
  private static var credentials = [String: URLCredential]()  // ["origin:port": credential]
  
  public static let validDomains: Set<String> = [
    // Brave Search
    "search.brave.software", "search.bravesoftware.com",
    "safesearch.brave.software", "safesearch.bravesoftware.com",
    "search-dev-local.brave.com",
    
    // Playlist
    "playlist.bravesoftware.com"
  ]
  
  public static func setCredential(origin: String, credential: URLCredential?) {
    credentials[origin] = credential
  }

  public func urlSession(
    _ session: URLSession,
    didReceive challenge: URLAuthenticationChallenge
  ) async -> (URLSession.AuthChallengeDisposition, URLCredential?) {
    // Certificate Pinning
    if challenge.protectionSpace.authenticationMethod == NSURLAuthenticationMethodServerTrust {
      if let serverTrust = challenge.protectionSpace.serverTrust {
        let host = challenge.protectionSpace.host
        let port = challenge.protectionSpace.port
        
        let result = BraveCertificateUtility.verifyTrust(serverTrust,
                                                         host: host,
                                                         port: port)
        // Cert is valid and should be pinned
        if result == 0 {
          return (.useCredential, URLCredential(trust: serverTrust))
        }
        
        // Cert is valid and should not be pinned
        // Let the system handle it and we'll show an error if the system cannot validate it
        if result == Int32.min {
          return (.performDefaultHandling, nil)
        }
        
        // Cert is invalid and cannot be pinned
        Logger.module.error("CERTIFICATE_INVALID")
        return (.cancelAuthenticationChallenge, nil)
      }
    }
    
    // MARK: -- Handle Authentication --

    // Too many failed attempts
    if challenge.previousFailureCount >= 3 {
      return (.rejectProtectionSpace, nil)
    }
    
    // URLAuthenticationChallenge isn't Sendable atm
    let protectionSpace = challenge.protectionSpace
    let origin = "\(challenge.protectionSpace.host):\(challenge.protectionSpace.port)"
    
    guard protectionSpace.authenticationMethod == NSURLAuthenticationMethodHTTPBasic ||
            protectionSpace.authenticationMethod == NSURLAuthenticationMethodHTTPDigest ||
            protectionSpace.authenticationMethod == NSURLAuthenticationMethodNTLM else {
      return (.performDefaultHandling, nil)
    }

    if let proposedCredential = challenge.proposedCredential ?? BasicAuthCredentialsManager.credentials[origin],
      !(proposedCredential.user?.isEmpty ?? true),
      challenge.previousFailureCount == 0 {
      return (.useCredential, proposedCredential)
    }

    // No proposed credential - reject the request
    if BasicAuthCredentialsManager.validDomains.contains(protectionSpace.host) {
      return (.rejectProtectionSpace, nil)
    }
    
    // No proposed credential - perform default handling
    return (.performDefaultHandling, nil)
  }

  public func urlSession(
    _ session: URLSession,
    task: URLSessionTask,
    didReceive challenge: URLAuthenticationChallenge
  ) async -> (URLSession.AuthChallengeDisposition, URLCredential?) {
    return await urlSession(session, didReceive: challenge)
  }
}
