// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import UIKit

class BasicAuthCredentialsManager: NSObject, URLSessionDataDelegate {
  private let domains: [String]
  
  init(for domains: [String]) {
    self.domains = domains
  }
  
  @MainActor
  private func loginsForChallenge(challenge: URLAuthenticationChallenge) async -> URLCredential? {
    guard let profile = UIApplication.shared.keyWindow?.windowScene?.browserViewController?.profile else {
      return nil
    }
    
    do {
      let cursor = try await profile.logins.getLoginsForProtectionSpace(challenge.protectionSpace)
      guard cursor.count >= 1 else {
        return nil
      }
      
      let logins = cursor.asArray()
      
      if logins.count > 1 {
        return (logins.find { login in
          (login.protectionSpace.protocol == challenge.protectionSpace.protocol) && !login.hasMalformedHostname
        })?.credentials
      } else if logins.count == 1, logins.first?.protectionSpace.protocol != challenge.protectionSpace.protocol {
        return logins.first?.credentials
      }
      
      return logins.first?.credentials
    } catch {
      return nil
    }
  }

  func urlSession(_ session: URLSession, didReceive challenge: URLAuthenticationChallenge, completionHandler: @escaping (URLSession.AuthChallengeDisposition, URLCredential?) -> Void) {

    guard challenge.protectionSpace.authenticationMethod == NSURLAuthenticationMethodHTTPBasic || challenge.protectionSpace.authenticationMethod == NSURLAuthenticationMethodHTTPDigest || challenge.protectionSpace.authenticationMethod == NSURLAuthenticationMethodNTLM else {
      completionHandler(.performDefaultHandling, nil)
      return
    }

    if !domains.contains(challenge.protectionSpace.host) {
      completionHandler(.performDefaultHandling, nil)
      return
    }

    // -- Handle Authentication --

    // Too many failed attempts
    if challenge.previousFailureCount >= 3 {
      completionHandler(.rejectProtectionSpace, nil)
      return
    }

    if let proposedCredential = challenge.proposedCredential,
      !(proposedCredential.user?.isEmpty ?? true),
      challenge.previousFailureCount == 0 {
      completionHandler(.useCredential, proposedCredential)
      return
    }

    // Lookup the credentials
    // If there is no profile or the challenge is not an auth challenge, reject the challenge
    Task { @MainActor in
      guard let credential = await loginsForChallenge(challenge: challenge) else {
        completionHandler(.rejectProtectionSpace, nil)
        return
      }
      
      completionHandler(.useCredential, credential)
    }
  }

  func urlSession(_ session: URLSession, task: URLSessionTask, didReceive challenge: URLAuthenticationChallenge, completionHandler: @escaping (URLSession.AuthChallengeDisposition, URLCredential?) -> Void) {
    urlSession(session, didReceive: challenge, completionHandler: completionHandler)
  }
}
