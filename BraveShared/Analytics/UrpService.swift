/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import Alamofire
import SafariServices
import Shared
import SwiftyJSON

private let log = Logger.browserLogger

enum UrpError {
    case networkError, downloadIdNotFound, ipNotFound, endpointError
}

/// Api endpoints for user referral program.
struct UrpService {
    private static let apiKeyParam = "api_key"
    private static let downLoadIdKeyParam = "download_id"

    let host: String
    private let apiKey: String
    let sessionManager: SessionManager

    init?(host: String, apiKey: String) {
        self.host = host
        self.apiKey = apiKey

        guard let hostUrl = try? host.asURL(), let normalizedHost = hostUrl.normalizedHost else { return nil }

        // Certificate pinning
        let serverTrustPolicies: [String: ServerTrustPolicy] = [
            normalizedHost: .pinCertificates(
                certificates: ServerTrustPolicy.certificates(),
                validateCertificateChain: true,
                validateHost: true
            )
        ]

        sessionManager = SessionManager(
            serverTrustPolicyManager: ServerTrustPolicyManager(
                policies: serverTrustPolicies
            )
        )
    }

    func referralCodeLookup(completion: @escaping (ReferralData?, UrpError?) -> Void) {
        guard var endPoint = try? host.asURL() else {
            completion(nil, .endpointError)
            return
        }
        endPoint.appendPathComponent("promo/initialize/ua")

        let params = [UrpService.apiKeyParam: apiKey]

        sessionManager.urpApiRequest(endPoint: endPoint, params: params) { response in
            log.debug("Referral code lookup response: \(response)")
            let json = JSON(response.data as Any)

            let referral = ReferralData(json: json)
            completion(referral, nil)
        }
    }
    
    func checkIfAuthorizedForGrant(with downloadId: String, completion: @escaping (Bool?, UrpError?) -> Void) {
        guard var endPoint = try? host.asURL() else {
            completion(nil, .endpointError)
            return
        }
        endPoint.appendPathComponent("promo/activity")

        let params = [
            UrpService.apiKeyParam: apiKey,
            UrpService.downLoadIdKeyParam: downloadId
        ]

        sessionManager.urpApiRequest(endPoint: endPoint, params: params) { response in
            log.debug("Check if authorized for grant response: \(response)")
            let json = JSON(response.data as Any)

            completion(json["finalized"].boolValue, nil)
        }
    }

    func fetchCustomHeaders(completion: @escaping ([CustomHeaderData], UrpError?) -> Void) {
        guard var endPoint = try? host.asURL() else {
            completion([], .endpointError)
            return
        }
        endPoint.appendPathComponent("promo/custom-headers")

        let params = [UrpService.apiKeyParam: apiKey]

        sessionManager.request(endPoint, parameters: params).responseJSON { response in
            let json = JSON(response.data as Any)
            let customHeaders = CustomHeaderData.customHeaders(from: json)
            completion(customHeaders, nil)
        }
    }
}

extension SessionManager {
    /// All requests to referral api use PUT method, accept and receive json.
    func urpApiRequest(endPoint: URL, params: [String: String], completion: @escaping (DataResponse<Any>) -> Void) {
        self.request(endPoint, method: .put, parameters: params, encoding: JSONEncoding.default).responseJSON { response in
            completion(response)
        }
    }
}
