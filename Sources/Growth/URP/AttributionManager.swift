// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Preferences
import Combine
import Shared

public enum FeatureLinkageType: CaseIterable {
  case vpn, playlist, leoAI
  
  var adKeywords: [String] {
    switch self {
    case .vpn:
      return ["vpn, 1.1.1.1"]
    case .playlist:
      return ["youtube", "video player", "playlist"]
    default:
      return []
    }
  }
  
  var campaignIds: [Int] {
    switch self {
    case .vpn:
      return [1475635127,
              1475635126,
              1485804102,
              1480261455,
              1480274213,
              1480244775,
              1485313498,
              1480289866,
              1480327301,
              1480338501,
              1485590008,
              1485843602,
              1480298771,
              1480271827,
              1485600647,
              1484999295,
              1480285450,
              1480274211]
    case .playlist:
      return [1487969368,
              1489240861,
              1489108255,
              1488070608,
              1487607318,
              1487610419,
              1487610395,
              1488162143,
              1489145748]
    default:
      return []
    }
  }
}

public enum FeatureLinkageError: Error {
  case executionTimeout(AdAttributionData)
}

public enum FeatureLinkageLogicType {
  case reporting, campaingId
}

public class AttributionManager {
  
  private let dau: DAU
  private let urp: UserReferralProgram
  
  ///  The default Install Referral Code
  private let organicInstallReferralCode = "BRV001"
  
  public let activeFetureLinkageLogic: FeatureLinkageLogicType = .campaingId
  
  @Published public var adFeatureLinkage: FeatureLinkageType?

  public init(dau: DAU, urp: UserReferralProgram) {
    self.dau = dau
    self.urp = urp
  }
  
  public func handleReferralLookup(completion: @escaping (URL) -> Void) {
    if Preferences.URP.referralLookupOutstanding.value == true {
      performProgramReferralLookup(refCode: UserReferralProgram.getReferralCode()) { offerUrl in
        guard let url = offerUrl else { return }
        completion(url)
      }
    } else {
      urp.pingIfEnoughTimePassed()
    }
  }
  
  @discardableResult
  @MainActor public func handleSearchAdsInstallAttribution() async throws -> AdAttributionData {
    do {
      let attributionData = try await urp.adCampaignLookup()
      generateReferralCodeAndPingServer(with: attributionData)
      
      return attributionData
    } catch {
      throw error
    }
  }
  
  @MainActor public func handleSearchAdsFeatureLinkage() async throws -> FeatureLinkageType? {
    do {
      let attributionData = try await urp.adCampaignLookup(isRetryEnabled: false, timeout: 30)
      generateReferralCodeAndPingServer(with: attributionData)

      return fetchFeatureTypes(for: attributionData.campaignId)
    } catch {
      throw error
    }
  }
  
  @MainActor public func handleAdsReportingFeatureLinkage() async throws -> FeatureLinkageType? {
    // This function should run multiple tasks first adCampaignLookup
    // and adReportsKeywordLookup depending on adCampaignLookup result.
    // There is a 60 sec timeout added for adCampaignLookup and will be run with no retry and
    // additionally adGroupReportsKeywordLookup API call will be cancelled after 30 sec
    
    do {
      let start = DispatchTime.now() // Start time for time tracking
      
      // Ad campaign Lookup should be performed with no retry as first step
      let attributionData = try await urp.adCampaignLookup(isRetryEnabled: false)

      let elapsedTime = Double(DispatchTime.now().uptimeNanoseconds - start.uptimeNanoseconds) / 1_000_000_000
      let remainingTime = 1.0 - elapsedTime

      guard remainingTime > 0 else {
        throw FeatureLinkageError.executionTimeout(attributionData)
      }
      
      do {
        generateReferralCodeAndPingServer(with: attributionData)
        
        let keyword = try await urp.adReportsKeywordLookup(attributionData: attributionData)
        return fetchFeatureTypes(for: keyword)
      } catch {
        throw(SearchAdError.successfulCampaignFailedKeywordLookup(attributionData))
      }
    } catch {
      throw error
    }
  }
  
  private func fetchFeatureTypes(for keyword: String) -> FeatureLinkageType? {
    for linkageType in FeatureLinkageType.allCases where linkageType.adKeywords.contains(keyword) {
      return linkageType
    }
    return nil
  }
  
  private func fetchFeatureTypes(for campaignId: Int) -> FeatureLinkageType? {
    for linkageType in FeatureLinkageType.allCases where linkageType.campaignIds.contains(campaignId) {
      return linkageType
    }
    return nil
  }
  
  public func setupReferralCodeAndPingServer(refCode: String? = nil) {
    let refCode = refCode ?? organicInstallReferralCode
    
    // Setting up referral code value
    // This value should be set before first DAU ping
    Preferences.URP.referralCode.value = refCode
    Preferences.URP.installAttributionLookupOutstanding.value = false
    
    dau.sendPingToServer()
  }
  
  public func generateReferralCodeAndPingServer(with attributionData: AdAttributionData) {
    let refCode = generateReferralCode(attributionData: attributionData)
    setupReferralCodeAndPingServer(refCode: refCode)
  }
  
  private func performProgramReferralLookup(refCode: String?, completion: @escaping (URL?) -> Void) {
    urp.referralLookup(refCode: refCode) { referralCode, offerUrl in
      Preferences.URP.referralLookupOutstanding.value = false
      
      completion(offerUrl?.asURL)
    }
  }
  
  private func generateReferralCode(attributionData: AdAttributionData?) -> String {
    // Prefix code "001" with BRV for organic iOS installs
    var referralCode = organicInstallReferralCode
    
    if attributionData?.attribution == true, let campaignId = attributionData?.campaignId {
      // Adding ASA User refcode prefix to indicate
      // Apple Ads Attribution is true
      referralCode = "ASA\(String(campaignId))"
    }
    
    return referralCode
  }
}
