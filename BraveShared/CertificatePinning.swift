// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Shared

private let log = Logger.browserLogger

// Taken from: https://github.com/Brandon-T/Jarvis and modified to simplify

public class PinningCertificateEvaluator: NSObject, URLSessionDelegate {
    struct ExcludedPinningHostUrls {
        static let urls = ["laptop-updates.brave.com",
                           "updates.bravesoftware.com",
                           "updates-cdn.bravesoftware.com"]
    }
    
    private let hosts: [String]
    private let certificates: [SecCertificate]
    private let options: PinningOptions
    
    public init(hosts: [String], options: PinningOptions = [.default, .validateHost, .anchorSpecificAndSystemTrusts]) {
        self.hosts = hosts
        self.options = options
        
        // Load certificates in the main bundle..
        self.certificates = {
            let paths = Set([".cer", ".CER", ".crt", ".CRT", ".der", ".DER"].map {
                Bundle.main.paths(forResourcesOfType: $0, inDirectory: nil)
            }.joined())
            
            return paths.compactMap({ path -> SecCertificate? in
                guard let certificateData = try? Data(contentsOf: URL(fileURLWithPath: path)) as CFData else {
                    return nil
                }
                return SecCertificateCreateWithData(nil, certificateData)
            })
        }()
    }
    
    public init(hosts: [String: SecCertificate], options: PinningOptions = [.default, .validateHost, .anchorSpecificAndSystemTrusts]) {
        self.hosts = hosts.map({ $0.key })
        self.certificates = hosts.map({ $0.value })
        self.options = options
    }
    
    public func urlSession(_ session: URLSession, didReceive challenge: URLAuthenticationChallenge, completionHandler: @escaping (URLSession.AuthChallengeDisposition, URLCredential?) -> Void) {
        
        // Certificate pinning
        if challenge.protectionSpace.authenticationMethod == NSURLAuthenticationMethodServerTrust {
            if let serverTrust = challenge.protectionSpace.serverTrust {
                do {
                    let host = challenge.protectionSpace.host
                    if ExcludedPinningHostUrls.urls.contains(host) {
                        return completionHandler(.performDefaultHandling, nil)
                    }
                    
                    if !canPinHost(host) {
                        fatalErrorInDebugModeIfPinningFailed()
                        throw error(reason: "Host not specified for pinning: \(host)")
                    }
                    
                    try evaluate(serverTrust, forHost: host)
                    return completionHandler(.useCredential, URLCredential(trust: serverTrust))
                } catch {
                    log.error(error)
                    fatalErrorInDebugModeIfPinningFailed()
                    return completionHandler(.cancelAuthenticationChallenge, nil)
                }
            }
            fatalErrorInDebugModeIfPinningFailed()
            return completionHandler(.cancelAuthenticationChallenge, nil)
        }
        return completionHandler(.performDefaultHandling, nil)
    }
    
    private func canPinHost(_ host: String) -> Bool {
        return hosts.contains(host)
    }
    
    private func error(reason: String) -> NSError {
        return NSError(domain: "com.brave.pinning-certificate-evaluator", code: -1, userInfo: [NSLocalizedDescriptionKey: reason])
    }
    
    public func evaluate(_ trust: SecTrust, forHost host: String) throws {
        // Certificate validation
        guard !certificates.isEmpty else {
            throw error(reason: "Empty Certificates")
        }
        
        // Certificate anchoring
        if options.contains(.anchorSpecificTrustsOnly) || options.contains(.anchorSpecificAndSystemTrusts) {
            // Add the certificates to the trust
            guard SecTrustSetAnchorCertificates(trust, certificates as CFArray) == errSecSuccess else {
                throw error(reason: "Certificate Anchor Failed")
            }
            
            if options.contains(.anchorSpecificTrustsOnly) {
                // Trust only the passed in certificates (true)
                //
                // This is the default behaviour, however we do it explicitly to throw an exception
                // immediately upon failure
                // The default behaviour will silently ignore the exception until validation
                guard SecTrustSetAnchorCertificatesOnly(trust, true) == errSecSuccess else {
                    throw error(reason: "Self-Signed Certificate Anchor Only Failed")
                }
            } else {
                // Trust also the built in system certificates (false)
                guard SecTrustSetAnchorCertificatesOnly(trust, false) == errSecSuccess else {
                    throw error(reason: "Certificate Anchor Only Failed")
                }
            }
        }
        
        // Default validation
        if options.contains(.default) {
            guard SecTrustSetPolicies(trust, SecPolicyCreateSSL(true, nil)) == errSecSuccess else {
                throw error(reason: "Trust Set Policies Failed")
            }
            
            var err: CFError?
            if !SecTrustEvaluateWithError(trust, &err) {
                if let err = err as Error? {
                    throw error(reason: "Trust Evaluation Failed: \(err)")
                }
                
                throw error(reason: "Unable to Evaluate Trust")
            }
        }
        
        // Host validation
        if options.contains(.validateHost) {
            guard SecTrustSetPolicies(trust, SecPolicyCreateSSL(true, host as CFString)) == errSecSuccess else {
                throw error(reason: "Trust Set Policies for Host Failed")
            }
            
            var err: CFError?
            if !SecTrustEvaluateWithError(trust, &err) {
                if let err = err as Error? {
                    throw error(reason: "Trust Evaluation Failed: \(err)")
                }
                
                throw error(reason: "Unable to Evaluate Trust")
            }
        }
        
        // Certificate binary matching
        let serverCertificates = Set((0..<SecTrustGetCertificateCount(trust))
            .compactMap { SecTrustGetCertificateAtIndex(trust, $0) }
            .compactMap({ SecCertificateCopyData($0) as Data }))
        
        // Set Certificate validation
        let clientCertificates = Set(certificates.compactMap({ SecCertificateCopyData($0) as Data }))
        if serverCertificates.isDisjoint(with: clientCertificates) {
            throw error(reason: "Pinning Failed")
        }
    }
    
    private func fatalErrorInDebugModeIfPinningFailed() {
        if !AppConstants.buildChannel.isPublic {
            assertionFailure("An SSL Pinning error has occurred")
        }
    }
    
    public struct PinningOptions: OptionSet {
        public let rawValue: Int
        
        public init(rawValue: Int) {
            self.rawValue = rawValue
        }
        
        /// System's default pinning policies
        public static let `default` = PinningOptions(rawValue: 1 << 0)
        
        /// Host Validation
        public static let validateHost = PinningOptions(rawValue: 1 << 1)
        
        /// Anchor ONLY the specified trusts passed to `PinningCertificateEvaluator.evaluate(trust:host:)`
        public static let anchorSpecificTrustsOnly = PinningOptions(rawValue: 1 << 2)
        
        /// Anchor the specified trusts passed to `PinningCertificateEvaluator.evaluate(trust:host:)`
        /// Also anchors the default trusts of the System
        public static let anchorSpecificAndSystemTrusts = PinningOptions(rawValue: 1 << 3)
        
        /// Default pinning policies + Host Validation + Anchor the specified trusts + Anchor system trusts
        public static let all: PinningOptions = [.default, .validateHost, .anchorSpecificAndSystemTrusts]
    }
}
