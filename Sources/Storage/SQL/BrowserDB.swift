/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import Shared
import os.log

public typealias Args = [Any?]

open class BrowserDB {
  fileprivate let db: SwiftData

  // SQLITE_MAX_VARIABLE_NUMBER = 999 by default. This controls how many ?s can
  // appear in a query string.
  public static let maxVariableNumber = 999

  public init(filename: String, secretKey: String? = nil, schema: Schema, files: FileAccessor) {
    Logger.module.debug("Initializing BrowserDB: \(filename).")

    // Probably will be removed with Storage framework
    // swiftlint:disable:next force_try
    let file = URL(fileURLWithPath: (try! files.getAndEnsureDirectory())).appendingPathComponent(filename).path

    if AppConstants.buildChannel == .debug && secretKey != nil {
      Logger.module.debug("Will attempt to use encrypted DB: \(file) with secret = \(secretKey ?? "nil")")
    }

    self.db = SwiftData(filename: file, key: secretKey, prevKey: nil, schema: schema, files: files)
  }

  /*
     * Opening a WAL-using database with a hot journal cannot complete in read-only mode.
     * The supported mechanism for a read-only query against a WAL-using SQLite database is to use PRAGMA query_only,
     * but this isn't all that useful for us, because we have a mixed read/write workload.
     */
  @discardableResult func withConnection<T>(flags: SwiftData.Flags = .readWriteCreate, _ callback: @escaping (_ connection: SQLiteDBConnection) throws -> T) async throws -> T {
    return try await db.withConnection(flags, callback)
  }

  func transaction<T>(_ callback: @escaping (_ connection: SQLiteDBConnection) throws -> T) async throws -> T {
    return try await db.transaction(callback)
  }

  public class func varlist(_ count: Int) -> String {
    return "(" + Array(repeating: "?", count: count).joined(separator: ", ") + ")"
  }

  func write(_ sql: String, withArgs args: Args? = nil) async throws -> Int {
    return try await withConnection { connection -> Int in
      try connection.executeChange(sql, withArgs: args)

      let modified = connection.numberOfRowsModified
      Logger.module.debug("Modified rows: \(modified).")
      return modified
    }
  }

  public func forceClose() {
    db.forceClose()
  }

  public func reopenIfClosed() {
    db.reopenIfClosed()
  }

  public func run(_ sql: String, withArgs args: Args? = nil) async throws {
    return try await run([(sql, args)])
  }

  func run(_ commands: [String]) async throws {
    return try await run(commands.map { (sql: $0, args: nil) })
  }

  /**
     * Runs an array of SQL commands. Note: These will all run in order in a transaction and will block
     * the caller's thread until they've finished. If any of them fail the operation will abort (no more
     * commands will be run) and the transaction will roll back, returning a DatabaseError.
     */
  func run(_ commands: [(sql: String, args: Args?)]) async throws {
    if commands.isEmpty {
      return
    }

    return try await transaction { connection -> Void in
      for (sql, args) in commands {
        try connection.executeChange(sql, withArgs: args)
      }
    }
  }

  func runQuery<T>(_ sql: String, args: Args?, factory: @escaping (SDRow) -> T) async throws -> Cursor<T> {
    return try await withConnection { connection -> Cursor<T> in
      connection.executeQuery(sql, factory: factory, withArgs: args)
    }
  }
}
