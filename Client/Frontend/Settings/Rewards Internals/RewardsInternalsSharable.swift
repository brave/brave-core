// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveRewards
import Shared

/// A set of helpers for `RewardsInternalsFileGenerator` to use to generate data to share with support
struct RewardsInternalsSharableBuilder {
    /// The rewards instance that we are generating the data from
    var rewards: BraveRewards
    /// A formatter to use when adding dates to a file
    var dateFormatter: DateFormatter
    /// A formatter to use when adding dates and times to a file
    var dateAndTimeFormatter: DateFormatter
    /// Writes a JSON object to `path` with a given name.
    func writeJSON(from object: Any, named: String, at path: String) throws {
        let data = try JSONSerialization.data(withJSONObject: object, options: [.prettyPrinted])
        try data.write(to: URL(fileURLWithPath: "\(path)/\(named).json"))
    }
}

/// Creates or moves files that will be in the final package to share with support
protocol RewardsInternalsFileGenerator {
    /// Generate or move files to a given path using a builder. Call `completion` when the tasks are done
    func generateFiles(
        at path: String,
        using builder: RewardsInternalsSharableBuilder,
        completion: @escaping (Error?) -> Void
    )
}

struct RewardsInternalsSharable: Equatable {
    /// The id for this sharables (i.e. "Basic", "DB", etc)
    var id: String
    /// The title that will display in the `RewardsInternalsShareController` for this sharable
    var title: String
    /// A description of what data is contained in this sharable and any disclosures that the user
    /// should know about when sharing this data.
    var description: String?
    /// Ask the sharable to copy, move, or save the files that will be shared to the file path passed in
    ///
    /// The file path will be a temporary directory and will be removed after the user dismisses
    /// the share sheet.
    var generator: RewardsInternalsFileGenerator
    
    static func == (lhs: RewardsInternalsSharable, rhs: RewardsInternalsSharable) -> Bool {
        return lhs.id == rhs.id
    }
    
    static let basic = RewardsInternalsSharable(
        id: "basic",
        title: Strings.RewardsInternals.sharableBasicTitle,
        description: Strings.RewardsInternals.sharableBasicDescription,
        generator: RewardsInternalsBasicInfoGenerator()
    )
    
    static let logs = RewardsInternalsSharable(
        id: "logs",
        title: Strings.RewardsInternals.logsTitle,
        description: Strings.RewardsInternals.sharableLogsDescription,
        generator: RewardsInternalsLogsGenerator()
    )
    
    static let promotions = RewardsInternalsSharable(
        id: "promotions",
        title: Strings.RewardsInternals.promotionsTitle,
        description: Strings.RewardsInternals.sharablePromotionsDescription,
        generator: RewardsInternalsPromotionsGenerator()
    )
    
    static let contributions = RewardsInternalsSharable(
        id: "contributions",
        title: Strings.RewardsInternals.contributionsTitle,
        description: Strings.RewardsInternals.sharableContributionsDescription,
        generator: RewardsInternalsContributionsGenerator()
    )
    
    static let database = RewardsInternalsSharable(
        id: "database",
        title: Strings.RewardsInternals.sharableDatabaseTitle,
        description: Strings.RewardsInternals.sharableDatabaseDescription,
        generator: RewardsInternalsDatabaseGenerator()
    )
    
    /// The default set of sharables when sharing from the main Rewards Internals screen
    static let `default`: [RewardsInternalsSharable] = [
        .basic,
        .promotions,
        .contributions
    ]
    
    /// A set of all the available sharables
    static let all: [RewardsInternalsSharable] = [
        .basic,
        .promotions,
        .contributions,
    ]
}

enum RewardsInternalsSharableError: Error {
    case rewardsInternalsUnavailable
}

/// A file generator that copies the Rewards ledger database into the sharable path
private struct RewardsInternalsDatabaseGenerator: RewardsInternalsFileGenerator {
    func generateFiles(at path: String, using builder: RewardsInternalsSharableBuilder, completion: @escaping (Error?) -> Void) {
        // Move Rewards database to path
        do {
            let dbPath = URL(fileURLWithPath: builder.rewards.ledger.rewardsDatabasePath)
            let newPath = URL(fileURLWithPath: path).appendingPathComponent(dbPath.lastPathComponent)
            try FileManager.default.copyItem(atPath: dbPath.path, toPath: newPath.path)
            completion(nil)
        } catch {
            completion(error)
        }
    }
}
