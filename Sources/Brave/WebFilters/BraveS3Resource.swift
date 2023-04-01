// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Shared

enum BraveS3Resource: Hashable, DownloadResourceInterface {
  /// Rules for debouncing links
  case debounceRules
  /// Generic iOS only content blocking behaviours used for the iOS content blocker
  case genericContentBlockingBehaviors
  /// Cosmetic filter rules
  case generalCosmeticFilters
  /// Adblock rules for a filter list
  /// iOS only content blocking behaviours used for the iOS content blocker for a given filter list
  case filterListContentBlockingBehaviors(uuid: String, componentId: String)
  
  /// The name of the info plist key that contains the service key
  private static let servicesKeyName = "SERVICES_KEY"
  /// The name of the header value that contains the service key
  private static let servicesKeyHeaderValue = "BraveServiceKey"
  /// The base s3 environment url that hosts the debouncing (and other) files.
  /// Cannot be used as-is and must be combined with a path
  private static var baseResourceURL: URL = {
    if AppConstants.buildChannel.isPublic {
      return URL(string: "https://adblock-data.s3.brave.com")!
    } else {
      return URL(string: "https://adblock-data-staging.s3.bravesoftware.com")!
    }
  }()
  
  /// The folder name under which this data should be saved under
  var cacheFolderName: String {
    switch self {
    case .debounceRules:
      return "debounce-data"
    case .filterListContentBlockingBehaviors(_, let componentId):
      return ["filter-lists", componentId].joined(separator: "/")
    case .genericContentBlockingBehaviors:
      return "abp-data"
    case .generalCosmeticFilters:
      return "cmf-data"
    }
  }
  
  /// Get the file name that is stored on the device
  var cacheFileName: String {
    switch self {
    case .debounceRules:
      return "ios-debouce.json"
    case .filterListContentBlockingBehaviors(let uuid, _):
      return "\(uuid)-latest.json"
    case .genericContentBlockingBehaviors:
      return "latest.json"
    case .generalCosmeticFilters:
      return "ios-cosmetic-filters.dat"
    }
  }
  
  /// Get the external path for the given filter list and this resource type
  var externalURL: URL {
    switch self {
    case .debounceRules:
      return Self.baseResourceURL.appendingPathComponent("/ios/debounce.json")
    case .filterListContentBlockingBehaviors(let uuid, _):
      return Self.baseResourceURL.appendingPathComponent("/ios/\(uuid)-latest.json")
    case .genericContentBlockingBehaviors:
      return Self.baseResourceURL.appendingPathComponent("/ios/latest.json")
    case .generalCosmeticFilters:
      return Self.baseResourceURL.appendingPathComponent("/ios/ios-cosmetic-filters.dat")
    }
  }
  
  var headers: [String: String] {
    var headers = [String: String]()
    
    if let servicesKeyValue = Bundle.main.getPlistString(for: Self.servicesKeyName) {
      headers[Self.servicesKeyHeaderValue] = servicesKeyValue
    }
    
    return headers
  }
}
