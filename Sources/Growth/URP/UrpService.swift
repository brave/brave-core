/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import SafariServices
import Shared
import BraveShared
import CertificateUtilities
import SwiftyJSON
import os.log
import Combine
import BraveCore

enum UrpError {
  case networkError, downloadIdNotFound, ipNotFound, endpointError
}

/// Api endpoints for user referral program.
struct UrpService {
  private struct ParamKeys {
    static let api = "api_key"
    static let referralCode = "referral_code"
    static let platform = "platform"
    static let downLoadId = "download_id"
  }

  private let host: String
  private let adServicesURL: String
  private let apiKey: String
  private let sessionManager: URLSession
  private let certificateEvaluator: URPCertificatePinningService

  init?(host: String, apiKey: String, adServicesURL: String) {
    self.host = host
    self.apiKey = apiKey
    self.adServicesURL = adServicesURL

    // Certificate pinning
    certificateEvaluator = URPCertificatePinningService()
    sessionManager = URLSession(configuration: .default, delegate: certificateEvaluator, delegateQueue: .main)
  }

  func referralCodeLookup(refCode: String?, completion: @escaping (ReferralData?, UrpError?) -> Void) {
    guard var endPoint = URL(string: host) else {
      completion(nil, .endpointError)
      UrpLog.log("Host not a url: \(host)")
      return
    }

    var params = [UrpService.ParamKeys.api: apiKey]

    var lastPathComponent = "ua"
    if let refCode = refCode {
      params[UrpService.ParamKeys.referralCode] = refCode
      params[UrpService.ParamKeys.platform] = "ios"
      lastPathComponent = "nonua"
    }
    endPoint.append(pathComponents: "promo", "initialize", lastPathComponent)

    sessionManager.urpApiRequest(endPoint: endPoint, params: params) { response in
      switch response {
      case .success(let data):
        if let data = data as? Data {
          Logger.module.debug("Referral code lookup response: \(String(data: data, encoding: .utf8) ?? "nil")")
        }
        
        UrpLog.log("Referral code lookup response: \(data)")

        let json = JSON(data)
        let referral = ReferralData(json: json)
        completion(referral, nil)

      case .failure(let error):
        Logger.module.error("Referral code lookup response: \(error.localizedDescription)")
        UrpLog.log("Referral code lookup response: \(error.localizedDescription)")

        completion(nil, .endpointError)
      }
    }
  }
  
  @MainActor func adCampaignTokenLookupQueue(adAttributionToken: String) async throws -> (AdAttributionData?) {
    guard let endPoint = URL(string: adServicesURL) else {
      Logger.module.error("AdServicesURLString can not be resolved: \(adServicesURL)")
      throw URLError(.badURL)
    }
    
    let attributionDataToken = adAttributionToken.data(using: .utf8)
    
    do {
      let (result, _) = try await sessionManager.adServicesAttributionApiRequest(endPoint: endPoint, rawData: attributionDataToken)
      UrpLog.log("Ad Attribution response: \(result)")
      
      if let resultData = result as? Data {
        let jsonResponse = try JSONSerialization.jsonObject(with: resultData, options: []) as? [String: Any]
        let adAttributionData = try AdAttributionData(json: jsonResponse)
        
        return adAttributionData
      }
    } catch {
      throw error
    }

    return (nil)
  }

  func checkIfAuthorizedForGrant(with downloadId: String, completion: @escaping (Bool?, UrpError?) -> Void) {
    guard var endPoint = URL(string: host) else {
      completion(nil, .endpointError)
      return
    }
    endPoint.append(pathComponents: "promo", "activity")

    let params = [
      UrpService.ParamKeys.api: apiKey,
      UrpService.ParamKeys.downLoadId: downloadId,
    ]

    sessionManager.urpApiRequest(endPoint: endPoint, params: params) { response in
      switch response {
      case .success(let data):
        if let data = data as? Data {
          Logger.module.debug("Check if authorized for grant response: \(String(data: data, encoding: .utf8) ?? "nil")")
        }
        let json = JSON(data)
        completion(json["finalized"].boolValue, nil)

      case .failure(let error):
        Logger.module.error("Check if authorized for grant response: \(error.localizedDescription)")
        completion(nil, .endpointError)
      }
    }
  }
}

extension URLSession {
  /// All requests to referral api use PUT method, accept and receive json.
  func urpApiRequest(endPoint: URL, params: [String: String], completion: @escaping (Result<Any, Error>) -> Void) {
    request(endPoint, method: .put, parameters: params, encoding: .json) { response in
      completion(response)
    }
  }
  
  // Apple ad service attricution request requires plain text encoding with post method and passing token as rawdata
  func adServicesAttributionApiRequest(endPoint: URL, rawData: Data?) async throws -> (Any, URLResponse) {
    // According to attributiontoken API docs
    // An error reponse can occur API call is done too quickly after receiving a valid token.
    // A best practice is to initiate retries at intervals of 5 seconds, with a maximum of three attempts.
    return try await Task.retry(retryCount: 3, retryDelay: 5) {
      return try await self.request(endPoint, method: .post, rawData: rawData, encoding: .textPlain)
    }.value
  }
}

class URPCertificatePinningService: NSObject, URLSessionDelegate {
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
