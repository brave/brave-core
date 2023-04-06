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


    // No proposed credential - reject challenge
    return (.rejectProtectionSpace, nil)
  }

  func urlSession(
    _ session: URLSession,
    task: URLSessionTask,
    didReceive challenge: URLAuthenticationChallenge
  ) async -> (URLSession.AuthChallengeDisposition, URLCredential?) {
    return await urlSession(session, didReceive: challenge)
  }
}
