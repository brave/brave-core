// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import XCTest
import BraveShared
@testable import CertificateUtilities
@testable import BraveCore

class CertificatePinningTest: XCTestCase {
  // Test whether pinning via Brave-Core works
  // https://github.com/brave/brave-core/blob/master/chromium_src/net/tools/transport_security_state_generator/input_file_parsers.cc
  func testBraveCoreLivePinningSuccess() {
    let urls = ["https://brave.com"]
    
    var managers = [URLSession]()
    var expectations = [XCTestExpectation]()
    for host in urls {
      let expectation = XCTestExpectation(description: "Test Pinning Live URLs: \(host)")
      expectations.append(expectation)

      guard let hostUrl = URL(string: host) else {
        XCTFail("Invalid URL/Host for pinning: \(host)")
        expectation.fulfill()
        return
      }

      let sessionManager = URLSession(configuration: .default, delegate: self, delegateQueue: .main)
      managers.append(sessionManager)

      sessionManager.dataTask(with: hostUrl) { data, response, error in
        if let error = error as NSError?, error.code == NSURLErrorCancelled {
          // Pinning failed. NET::ERR_SSL_PINNED_KEY_NOT_IN_CERT_CHAIN
          XCTFail("Invalid URL/Host for pinning: \(error.localizedDescription) for host: \(host)")
          expectation.fulfill()
          return
        }

        // Success. Website is pinned.
        expectation.fulfill()
      }.resume()
      sessionManager.finishTasksAndInvalidate()
    }

    wait(for: expectations, timeout: 10.0)
  }
  
  // Test whether pinning via Brave-Core works
  // https://github.com/brave/brave-core/blob/master/chromium_src/net/tools/transport_security_state_generator/input_file_parsers.cc
  func testBraveCoreLivePinningFailure() {
    let urls = ["https://ssl-pinning.someblog.org"]
    
    var managers = [URLSession]()
    var expectations = [XCTestExpectation]()
    for host in urls {
      let expectation = XCTestExpectation(description: "Test Pinning Live URLs: \(host)")
      expectations.append(expectation)

      guard let hostUrl = URL(string: host) else {
        XCTFail("Invalid URL/Host for pinning: \(host)")
        expectation.fulfill()
        return
      }

      let sessionManager = URLSession(configuration: .default, delegate: self, delegateQueue: .main)
      managers.append(sessionManager)

      sessionManager.dataTask(with: hostUrl) { data, response, error in
        if let error = error as NSError?, error.code == NSURLErrorCancelled {
          // Pinning failed. NET::ERR_SSL_PINNED_KEY_NOT_IN_CERT_CHAIN
          expectation.fulfill()
          return
        }

        XCTFail("Website \(host) pinned successfully. Expected to fail!")
        expectation.fulfill()
      }.resume()
      sessionManager.finishTasksAndInvalidate()
    }

    wait(for: expectations, timeout: 10.0)
  }
}

extension CertificatePinningTest: URLSessionDelegate {
  func urlSession(_ session: URLSession, didReceive challenge: URLAuthenticationChallenge) async -> (URLSession.AuthChallengeDisposition, URLCredential?) {
    if challenge.protectionSpace.authenticationMethod == NSURLAuthenticationMethodServerTrust {
      if let serverTrust = challenge.protectionSpace.serverTrust {
        let result = BraveCertificateUtility.verifyTrust(serverTrust, host: challenge.protectionSpace.host, port: challenge.protectionSpace.port)
        // Cert is valid and should be pinned
        if result == 0 {
          return (.useCredential, URLCredential(trust: serverTrust))
        }
        
        // Cert is valid and should not be pinned
        // Let the system handle it and we'll show an error if the system cannot validate it
        if result == Int32.min {
          return (.performDefaultHandling, nil)
        }
      }
      return (.cancelAuthenticationChallenge, nil)
    }
    return (.performDefaultHandling, nil)
  }
}
