// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveCore
@testable import Brave

extension NetworkManager {
  static func makeNetworkManager(for resources: [ResourceDownloader.Resource], statusCode: Int = 200, etag: String? = nil) -> NetworkManager {
    let session = BaseMockNetworkSession { url in
      guard let resource = resources.first(where: { resource in
        url.absoluteURL == resource.externalURL
      }) else {
        throw "Resource not found"
      }
      
      let data = try await self.mockData(for: resource)
      
      let response = ResourceDownloader.getMockResponse(
        for: resource,
        statusCode: statusCode,
        headerFields: etag != nil ? ["Etag": etag!] : nil
      )
      
      return (data, response)
    }
    
    return NetworkManager(session: session)
  }
  
  static func mockData(for resource: ResourceDownloader.Resource) async throws -> Data {
    try await Task<Data, Error>.detached(priority: .background) {
      switch resource {
      case .debounceRules:
        let bundle = Bundle.module
        let resourceURL = bundle.url(forResource: "debouncing", withExtension: "json")
        let data = try Data(contentsOf: resourceURL!)
        return data
        
      case .genericContentBlockingBehaviors, .filterListContentBlockingBehaviors:
        let bundle = Bundle.module
        let resourceURL = bundle.url(forResource: "content-blocking", withExtension: "json")
        let data = try Data(contentsOf: resourceURL!)
        return data
        
      default:
        // Because of the retry timeout we don't throw any errors but return some empty data
        return Data()
      }
    }.value
  }
}
