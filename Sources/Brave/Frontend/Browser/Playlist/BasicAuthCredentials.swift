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
  private func loginsForChallenge(protectionSpace: URLProtectionSpace) async -> URLCredential? {
    guard let profile = UIApplication.shared.keyWindow?.windowScene?.browserViewController?.profile else {
      return nil
    }
    
    do {
      let cursor = try await profile.logins.getLoginsForProtectionSpace(protectionSpace)
      guard cursor.count >= 1 else {
        return nil
      }
      
      let logins = cursor.asArray()
      
      if logins.count > 1 {
        return (logins.first(where: { login in
          (login.protectionSpace.protocol == protectionSpace.protocol) && !login.hasMalformedHostname
        }))?.credentials
      } else if logins.count == 1, logins.first?.protectionSpace.protocol != protectionSpace.protocol {
        return logins.first?.credentials
      }
      
      return logins.first?.credentials
    } catch {
      return nil
    }
  }

  func urlSession(
    _ session: URLSession,
    didReceive challenge: URLAuthenticationChallenge
  ) async -> (URLSession.AuthChallengeDisposition, URLCredential?) {
    guard challenge.protectionSpace.authenticationMethod == NSURLAuthenticationMethodHTTPBasic ||
            challenge.protectionSpace.authenticationMethod == NSURLAuthenticationMethodHTTPDigest ||
            challenge.protectionSpace.authenticationMethod == NSURLAuthenticationMethodNTLM else {
      return (.performDefaultHandling, nil)
    }

    if !domains.contains(challenge.protectionSpace.host) {
      return (.performDefaultHandling, nil)
    }

    // -- Handle Authentication --

    // Too many failed attempts
    if challenge.previousFailureCount >= 3 {
      return (.rejectProtectionSpace, nil)
    }

    if let proposedCredential = challenge.proposedCredential,
      !(proposedCredential.user?.isEmpty ?? true),
      challenge.previousFailureCount == 0 {
      return (.useCredential, proposedCredential)
    }

    // Lookup the credentials
    // If there is no profile or the challenge is not an auth challenge, reject the challenge
    guard let credential = await loginsForChallenge(protectionSpace: challenge.protectionSpace) else {
      return (.rejectProtectionSpace, nil)
    }
      
    return (.useCredential, credential)
  }

  func urlSession(
    _ session: URLSession,
    task: URLSessionTask,
    didReceive challenge: URLAuthenticationChallenge
  ) async -> (URLSession.AuthChallengeDisposition, URLCredential?) {
    return await urlSession(session, didReceive: challenge)
  }
}
