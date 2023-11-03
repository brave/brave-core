// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import os.log

public struct AdAttributionData {
  // A value of true returns if a user clicks an Apple Search Ads impression up to 30 days before your app download.
  // If the API can’t find a matching attribution record, the attribution value is false.
  public let attribution: Bool
  // The identifier of the organization that owns the campaign.
  // organizationId is the same as your account in the Apple Search Ads UI.
  public let organizationId: Int?
  // The type of conversion is either Download or Redownload.
  public let conversionType: String?
  // The unique identifier for the campaign.
  public let campaignId: Int
  // The country or region for the campaign.
  public let countryOrRegion: String?
  
  init(attribution: Bool, organizationId: Int? = nil, conversionType: String? = nil, campaignId: Int, countryOrRegion: String? = nil) {
    self.attribution = attribution
    self.organizationId = organizationId
    self.conversionType = conversionType
    self.campaignId = campaignId
    self.countryOrRegion = countryOrRegion
  }
}

enum SerializationError: Error {
  case missing(String)
  case invalid(String, Any)
}

extension AdAttributionData {
  init(json: [String: Any]?) throws {
    guard let json = json else {
      throw SerializationError.invalid("Invalid json Dictionary", "")
    }
    
    // Attribution and campaignId are the major properties here
    // They will indicate if the Apple Searhs Ads is clicked and for which campaign
    guard let attribution = json["attribution"] as? Bool else {
      Logger.module.error("Failed to unwrap json to Ad Attribution property.")
      UrpLog.log("Failed to unwrap json to Ad Attribution property. \(json)")
      
      throw SerializationError.missing("Attribution Context")
    }
    
    guard let campaignId = json["campaignId"] as? Int else {
      Logger.module.error("Failed to unwrap json to Campaign Id property.")
      UrpLog.log("Failed to unwrap json to Campaign Id property. \(json)")
      
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
  }
}
