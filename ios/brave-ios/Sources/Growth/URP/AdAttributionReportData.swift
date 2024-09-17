// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveShared
import Foundation
import os.log

public enum SerializationError: Error {
  case missing(String)
  case invalid(String, Any)
}

public enum SearchAdError: Error {
  case invalidCampaignTokenData
  case invalidGroupsReportData
  case failedCampaignTokenFetch
  case failedCampaignTokenLookup
  case missingReportsKeywordParameter
  case failedReportsKeywordLookup
  case successfulCampaignFailedKeywordLookup(AdAttributionData)
}

public struct AdAttributionData {
  // A value of true returns if a user clicks an Apple Search Ads impression up to 30 days before your app download.
  // If the API can’t find a matching attribution record, the attribution value is false.
  let attribution: Bool
  // The identifier of the organization that owns the campaign.
  // organizationId is the same as your account in the Apple Search Ads UI.
  let organizationId: Int?
  // The type of conversion is either Download or Redownload.
  let conversionType: String?
  // The unique identifier for the campaign.
  let campaignId: Int
  // The country or region for the campaign.
  let countryOrRegion: String?
  // The ad group if for the campaign which will be used for feature link
  let adGroupId: Int?
  // The keyword id for the campaign which will be used for feature link
  let keywordId: Int?

  init(
    attribution: Bool,
    organizationId: Int? = nil,
    conversionType: String? = nil,
    campaignId: Int,
    countryOrRegion: String? = nil,
    adGroupId: Int? = nil,
    keywordId: Int? = nil
  ) {
    self.attribution = attribution
    self.organizationId = organizationId
    self.conversionType = conversionType
    self.campaignId = campaignId
    self.countryOrRegion = countryOrRegion
    self.adGroupId = adGroupId
    self.keywordId = keywordId
  }

  init(json: [String: Any]?) throws {
    guard let json = json else {
      throw SerializationError.invalid("Invalid json Dictionary", "")
    }

    // Attribution and campaignId are the major properties here
    // They will indicate if the Apple Searhs Ads is clicked and for which campaign
    guard let attribution = json["attribution"] as? Bool else {
      Logger.module.error("Failed to unwrap json to Ad Attribution property.")
      throw SerializationError.missing("Attribution Context")
    }

    guard let campaignId = json["campaignId"] as? Int else {
      Logger.module.error("Failed to unwrap json to Campaign Id property.")
      throw SerializationError.missing("Campaign Id")
    }

    if let conversionType = json["conversionType"] as? String {
      guard conversionType == "Download" || conversionType == "Redownload" else {
        throw SerializationError.invalid("Conversion Type", conversionType)
      }
    }

    self.attribution = attribution
    self.organizationId = json["orgId"] as? Int
    self.conversionType = json["conversionType"] as? String
    self.campaignId = campaignId
    self.countryOrRegion = json["countryOrRegion"] as? String
    self.adGroupId = json["adGroupId"] as? Int
    self.keywordId = json["keywordId"] as? Int
  }
}

public struct AdGroupReportData {
  public struct KeywordReportResponse: Codable {
    let row: [KeywordRow]
  }

  public struct KeywordRow: Codable {
    let metadata: KeywordMetadata
  }

  public struct KeywordMetadata: Codable {
    let keyword: String
    let keywordId: Int
  }

  public let productKeyword: String

  init(data: Data, keywordId: Int) throws {
    do {
      let decoder = JSONDecoder()
      let keywordResponse = try decoder.decode(KeywordReportResponse.self, from: data)

      if let keywordRow = keywordResponse.row.first(where: { $0.metadata.keywordId == keywordId }) {
        productKeyword = keywordRow.metadata.keyword
      } else {
        throw SerializationError.invalid("Keyword with ID \(keywordId) not found", "")
      }
    } catch {
      throw SerializationError.invalid("Invalid json Dictionary", "")
    }
  }
}
