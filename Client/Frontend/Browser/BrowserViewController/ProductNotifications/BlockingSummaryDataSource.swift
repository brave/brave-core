// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import Shared
import BraveShared

private let log = Logger.browserLogger

// MARK: - BlockingSummary

struct BlockingSummary: Codable {
    
    // MARK: CodingKeys
    
    enum CodingKeys: String, CodingKey {
        case site
        case savings
        case numchildpages = "numofchildpages"
        case childsavings = "avgchildsavings"
        case sitesavings = "avgsitesavings"
        case totalsavings
        case avgsavings
    }
    
    // MARK: Internal
    
    let site: String
    let savings: Int
    let numchildpages: Int
    let childsavings: Double
    let sitesavings: Double
    let totalsavings: Double
    let avgsavings: String
    
    // MARK: Lifecycle
    
    public init(from decoder: Decoder) throws {
        let container = try decoder.container(keyedBy: CodingKeys.self)
        
        site = try container.decode(String.self, forKey: .site)
        savings = try container.decode(Int.self, forKey: .savings)
        numchildpages = try container.decode(Int.self, forKey: .numchildpages)
        childsavings = try container.decode(Double.self, forKey: .childsavings)
        sitesavings = try container.decode(Double.self, forKey: .sitesavings)
        totalsavings = try container.decode(Double.self, forKey: .totalsavings)
        
        let unformattedSaving = try container.decode(Double.self, forKey: .avgsavings)
        avgsavings = String(format: "%.2f", unformattedSaving)
    }
}

// MARK: - BlockingSummaryDataSource

class BlockingSummaryDataSource {
    
    // MARK: Lifecycle
    
    init(with fileName: String = "blocking-summary", bundle: Bundle = Bundle.main) {
        blockingSummaryList = fetchBlockingSummaryObjects(with: fileName, for: bundle)
    }
    
    // MARK: Internal
    
    func fetchDomainFetchedSiteSavings(_ url: URL) -> String? {
        let domain = url.baseDomain ?? url.host ?? url.hostSLD

        return blockingSummaryList.first(where: { $0.site.contains(domain) })?.avgsavings
    }
    
    // MARK: Private
    
    /// The list containing details related with blocking values of sites fetched from the JSON file
    private var blockingSummaryList = [BlockingSummary]()
    
    /// The function which uses the Data from Local JSON file to fetch list of objects
    private func fetchBlockingSummaryObjects(with fileName: String, for bundle: Bundle) -> [BlockingSummary] {
        var blockingSummaryList = [BlockingSummary]()
        
        guard let blockSummaryData = createJSONDataFrom(fileName: fileName, bundle: bundle) else {
            return blockingSummaryList
        }

        do {
            blockingSummaryList = try JSONDecoder().decode([BlockingSummary].self, from: blockSummaryData)
        } catch {
            log.error("Failed to decode blockign summary object from json Data \(error)")
        }
        
        return blockingSummaryList
    }
    
    /// The helper function with created the Data from parametrized file path
    private func createJSONDataFrom(fileName: String, bundle: Bundle) -> Data? {
        guard let filePath =  bundle.path(forResource: fileName, ofType: "json") else {
            return nil
        }
        
        do {
            return try Data(contentsOf: URL(fileURLWithPath: filePath))
        } catch {
            log.error("Failed to get bundle path for \(fileName)")
        }
        
        return nil
    }

}
