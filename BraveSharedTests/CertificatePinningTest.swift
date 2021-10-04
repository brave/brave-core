// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import XCTest
@testable import BraveShared

extension CertificatePinningTest {
    private func certificate(named: String) -> SecCertificate {
        let path = Bundle(for: CertificatePinningTest.self).path(forResource: named, ofType: ".cer")!
        let certificateData = try! Data(contentsOf: URL(fileURLWithPath: path)) as CFData
        return SecCertificateCreateWithData(nil, certificateData)!
    }
    
    private func trust(for certificates: [SecCertificate]) -> SecTrust {
        var trust: SecTrust!
        SecTrustCreateWithCertificates(certificates as CFTypeRef, SecPolicyCreateBasicX509(), &trust)
        return trust
    }
}

class CertificatePinningTest: XCTestCase {
    
    private lazy var leaf = certificate(named: "leaf")
    private lazy var intermediate = certificate(named: "intermediate")
    private lazy var root = certificate(named: "root")
    
    func testFailPinningWithHostValidation() {
        let host = "github.com"
        let trust = self.trust(for: [leaf, intermediate, root])
        let evaluator = PinningCertificateEvaluator(hosts: [host: leaf], options: [.default, .validateHost])
        
        do {
            try evaluator.evaluate(trust, forHost: host)
            XCTFail("Validation succeeded but should have failed")
        } catch {
            
        }
    }
    
    func testExpiredCertificate() {
        let leaf = certificate(named: "expired.badssl.com-leaf")
        let intermediate = certificate(named: "expired.badssl.com-intermediate-ca-1")
        let intermediate2 = certificate(named: "expired.badssl.com-intermediate-ca-2")
        let root = certificate(named: "expired.badssl.com-root-ca")
        
        let host = "badssl.com"
        let trust = self.trust(for: [leaf, intermediate, intermediate2, root])
        let evaluator = PinningCertificateEvaluator(hosts: [host: leaf], options: [.default, .validateHost])
        
        do {
            try evaluator.evaluate(trust, forHost: host)
            XCTFail("Validation succeeded but should have failed")
        } catch {
            
        }
    }
    
    func testUntrustedRoot() {
        let leaf = certificate(named: "untrusted.badssl.com-leaf")
        let root = certificate(named: "untrusted.badssl.com-root")

        let host = "badssl.com"
        let trust = self.trust(for: [root])
        let evaluator = PinningCertificateEvaluator(hosts: [host: leaf], options: [.default, .validateHost])
        
        do {
            try evaluator.evaluate(trust, forHost: host)
            XCTFail("Validation succeeded but should have failed")
        } catch {
            
        }
    }
    
    func testSelfSignedRoot() {
        let leaf = certificate(named: "untrusted.badssl.com-leaf")
        let root = certificate(named: "self-signed.badssl.com")

        let host = "badssl.com"
        let trust = self.trust(for: [root])
        let evaluator = PinningCertificateEvaluator(hosts: [host: leaf], options: [.default, .validateHost])
        
        do {
            try evaluator.evaluate(trust, forHost: host)
            XCTFail("Validation succeeded but should have failed")
        } catch {
            
        }
    }
    
    // As of iOS 13, it's extremely hard to :allow: a self-signed certificate!
    // iOS 13 is validating the certificate against a list of CA's.. so to even use a self-signed certificate, you have to add it to the keychain/keystore as trusted..
    // Or sign it with a trusted CA.. For now this test has been disabled as we aren't using self-signed certificates anyway.
    // https://support.apple.com/en-us/HT210176
    func testSelfSignedRootAllowed() {
        let leaf = certificate(named: "self-signed")

        let host = "unit-test.brave.com"
        let trust = self.trust(for: [leaf])
        let evaluator = PinningCertificateEvaluator(hosts: [host: leaf], options: [.default, .validateHost, .allowSelfSigned])
        
        do {
            try evaluator.evaluate(trust, forHost: host)
        } catch {
            XCTFail("Validation failed but should have succeeded: \(error)")
        }
    }
    
    // Same as :testSelfSignedRootAllowed:
    func testSelfSignedRootAllowed2() {
        let leaf = certificate(named: "expired.badssl.com-leaf")
        let root = certificate(named: "self-signed.badssl.com")

        let host = "badssl.com"
        let trust = self.trust(for: [root])
        let evaluator = PinningCertificateEvaluator(hosts: [host: leaf], options: [.default, .validateHost, .allowSelfSigned])
        
        do {
            try evaluator.evaluate(trust, forHost: host)
        } catch {
            XCTFail("Validation failed but should have succeeded: \(error)")
        }
    }
    
    // Test whether or not exception URLs are NOT pinned!
    func testLivePinningSuccess() {
        let urls = PinningCertificateEvaluator.ExcludedPinningHostUrls.urls.map({ "https://\($0)" })
        
        var expectations = [XCTestExpectation]()
        for host in urls {
            let expectation = XCTestExpectation(description: "Test Pinning Live URLs: \(host)")
            expectations.append(expectation)
            
            guard let hostUrl = URL(string: host),
                  let normalizedHost = hostUrl.normalizedHost() else {
                
                XCTFail("Invalid URL/Host for pinning: \(host)")
                expectation.fulfill()
                return
            }
            
            let certificateEvaluator = PinningCertificateEvaluator(hosts: [normalizedHost])
            let sessionManager = URLSession(configuration: .default, delegate: certificateEvaluator, delegateQueue: .main)

            sessionManager.request(hostUrl, method: .put, parameters: ["unit-test":"unit-value"], encoding: .json) { response in
                switch response {
                case .success:
                    break
                    
                case .failure(let error as NSError):
                    if error.code == NSURLErrorCancelled {
                        XCTFail("Invalid URL/Host for pinning: \(error) for host: \(host)")
                    }
                }
                
                expectation.fulfill()
            }.resume()
            sessionManager.finishTasksAndInvalidate()
        }
        
        wait(for: expectations, timeout: 10.0)
    }
    
    // Test whether or not pinning actually works on a live URL
    func testLivePinningFailure() {
        let urls = ["https://brave.com"]
        
        var managers = [URLSession]()
        var expectations = [XCTestExpectation]()
        for host in urls {
            let expectation = XCTestExpectation(description: "Test Pinning Live URLs: \(host)")
            expectations.append(expectation)
            
            guard let hostUrl = URL(string: host),
                  let normalizedHost = hostUrl.normalizedHost() else {
                
                XCTFail("Invalid URL/Host for pinning: \(host)")
                expectation.fulfill()
                return
            }
            
            let certificateEvaluator = PinningCertificateEvaluator(hosts: [normalizedHost])
            let sessionManager = URLSession(configuration: .default, delegate: certificateEvaluator, delegateQueue: .main)
            managers.append(sessionManager)

            sessionManager.dataTask(with: hostUrl) { data, response, error in
                if let error = error as NSError?, error.code == NSURLErrorCancelled {
                    XCTFail("Invalid URL/Host for pinning: \(error) for host: \(host)")
                }
                
                expectation.fulfill()
            }.resume()
            sessionManager.finishTasksAndInvalidate()
        }
        
        wait(for: expectations, timeout: 10.0)
    }
}
