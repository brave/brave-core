// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveShared
import Shared
import UIKit
import os.log

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

public class BlockingSummaryDataSource {

  // MARK: Lifecycle

  public init(with filePath: String? = nil) {
    self.filePath =
      filePath ?? Bundle.module.path(forResource: "blocking-summary", ofType: "json")
  }

  public func fetchDomainFetchedSiteSavings(_ url: URL) async -> String? {
    if !isSummaryListLoaded {
      await fetchBlockingSummaryObjects(with: filePath)
    }
    let domain = url.baseDomain ?? url.host ?? url.hostSLD

    return blockingSummaryList.first(where: { $0.site.contains(domain) })?.avgsavings
  }

  // MARK: Private

  private let filePath: String?
  private var isSummaryListLoaded: Bool = false

  /// The list containing details related with blocking values of sites fetched from the JSON file
  private var blockingSummaryList = [BlockingSummary]()

  /// The function which uses the Data from Local JSON file to fetch list of objects
  private func fetchBlockingSummaryObjects(with filePath: String?) async {
    guard let filePath else { return }
    defer { isSummaryListLoaded = true }
    guard let blockSummaryData = await AsyncFileManager.default.contents(atPath: filePath) else {
      Logger.module.error("Failed to get bundle path for \(filePath)")
      return
    }

    do {
      blockingSummaryList = try JSONDecoder().decode([BlockingSummary].self, from: blockSummaryData)
    } catch {
      Logger.module.error(
        "Failed to decode blockign summary object from json Data \(error.localizedDescription)"
      )
    }
  }
}
