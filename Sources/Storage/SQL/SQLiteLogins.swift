/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import Shared
import os.log

open class SQLiteLogins: BrowserLogins {

  fileprivate let db: BrowserDB
  fileprivate static let mainColumns: String = "guid, username, password, hostname, httpRealm, formSubmitURL, usernameField, passwordField"
  fileprivate static let mainWithLastUsedColumns: String = mainColumns + ", timeLastUsed, timesUsed"
  fileprivate static let loginColumns: String = mainColumns + ", timeCreated, timeLastUsed, timePasswordChanged, timesUsed"

  public init(db: BrowserDB) {
    self.db = db
  }

  fileprivate class func populateLogin(_ login: Login, row: SDRow) {
    login.formSubmitURL = row["formSubmitURL"] as? String
    login.usernameField = row["usernameField"] as? String
    login.passwordField = row["passwordField"] as? String
    login.guid = row["guid"] as! String

    if let timeCreated = row.getTimestamp("timeCreated"),
      let timeLastUsed = row.getTimestamp("timeLastUsed"),
      let timePasswordChanged = row.getTimestamp("timePasswordChanged"),
      let timesUsed = row["timesUsed"] as? Int {
      login.timeCreated = timeCreated
      login.timeLastUsed = timeLastUsed
      login.timePasswordChanged = timePasswordChanged
      login.timesUsed = timesUsed
    }
  }

  fileprivate class func constructLogin<T: Login>(_ row: SDRow, c: T.Type) -> T {
    let credential = URLCredential(
      user: row["username"] as? String ?? "",
      password: row["password"] as! String,
      persistence: .none)

    // There was a bug in previous versions of the app where we saved only the hostname and not the
    // scheme and port in the DB. To work with these scheme-less hostnames, we try to extract the scheme and
    // hostname by converting to a URL first. If there is no valid hostname or scheme for the URL,
    // fallback to returning the raw hostname value from the DB as the host and allow NSURLProtectionSpace
    // to use the default (http) scheme. See https://bugzilla.mozilla.org/show_bug.cgi?id=1238103.

    let hostnameString = (row["hostname"] as? String) ?? ""
    let hostnameURL = hostnameString.asURL

    let scheme = hostnameURL?.scheme
    let port = hostnameURL?.port ?? 0

    // Check for malformed hostname urls in the DB
    let host: String
    var malformedHostname = false
    if let h = hostnameURL?.host {
      host = h
    } else {
      host = hostnameString
      malformedHostname = true
    }

    let protectionSpace = URLProtectionSpace(
      host: host,
      port: port,
      protocol: scheme,
      realm: row["httpRealm"] as? String,
      authenticationMethod: nil)

    let login = T(credential: credential, protectionSpace: protectionSpace)
    self.populateLogin(login, row: row)
    login.hasMalformedHostname = malformedHostname
    return login
  }

  fileprivate class func loginFactory(_ row: SDRow) -> Login {
    return self.constructLogin(row, c: Login.self)
  }

  fileprivate class func loginDataFactory(_ row: SDRow) -> LoginData {
    return loginFactory(row) as LoginData
  }

  fileprivate class func loginUsageDataFactory(_ row: SDRow) -> LoginUsageData {
    return loginFactory(row) as LoginUsageData
  }

  func notifyLoginDidChange() {
    Logger.module.debug("Notifying login did change.")

    // For now we don't care about the contents.
    // This posts immediately to the shared notification center.
    NotificationCenter.default.post(name: .dataLoginDidChange, object: nil)
  }

  open func getLoginsForProtectionSpace(_ protectionSpace: URLProtectionSpace) async throws -> Cursor<LoginData> {
    let projection = SQLiteLogins.mainWithLastUsedColumns

    let sql = """
      SELECT \(projection)
      FROM loginsL WHERE is_deleted = 0 AND hostname IS ? OR hostname IS ?
      UNION ALL
      SELECT \(projection)
      FROM loginsM WHERE is_overridden = 0 AND hostname IS ? OR hostname IS ?
      ORDER BY timeLastUsed DESC
      """

    // Since we store hostnames as the full scheme/protocol + host, combine the two to look up in our DB.
    // In the case of https://bugzilla.mozilla.org/show_bug.cgi?id=1238103, there may be hostnames without
    // a scheme. Check for these as well.
    let args: Args = [
      protectionSpace.urlString(),
      protectionSpace.host,
      protectionSpace.urlString(),
      protectionSpace.host,
    ]
    
    Logger.module.debug("Looking for login: \(protectionSpace.urlString()) && \(protectionSpace.host)")
    
    return try await db.runQuery(sql, args: args, factory: SQLiteLogins.loginDataFactory)
  }

  // username is really Either<String, NULL>; we explicitly match no username.
  open func getLoginsForProtectionSpace(_ protectionSpace: URLProtectionSpace, withUsername username: String?) async throws -> Cursor<LoginData> {
    let projection = SQLiteLogins.mainWithLastUsedColumns

    let args: Args
    let usernameMatch: String
    if let username = username {
      args = [
        protectionSpace.urlString(), username, protectionSpace.host,
        protectionSpace.urlString(), username, protectionSpace.host,
      ]
      usernameMatch = "username = ?"
    } else {
      args = [
        protectionSpace.urlString(), protectionSpace.host,
        protectionSpace.urlString(), protectionSpace.host,
      ]
      usernameMatch = "username IS NULL"
    }

    let sql = """
      SELECT \(projection)
      FROM loginsL
      WHERE is_deleted = 0 AND hostname IS ? AND \(usernameMatch) OR hostname IS ?
      UNION ALL
      SELECT \(projection)
      FROM loginsM
      WHERE is_overridden = 0 AND hostname IS ? AND \(usernameMatch) OR hostname IS ?
      ORDER BY timeLastUsed DESC
      """

    return try await db.runQuery(sql, args: args, factory: SQLiteLogins.loginDataFactory)
  }

  open func getAllLogins() async throws -> Cursor<Login> {
    let projection = SQLiteLogins.loginColumns
    
    let sql = """
      SELECT \(projection)
      FROM loginsL
      WHERE is_deleted = 0
      UNION ALL
      SELECT \(projection)
      FROM loginsM
      WHERE is_overridden = 0
      ORDER BY hostname ASC
      """

    return try await db.runQuery(sql, args: nil, factory: SQLiteLogins.loginFactory)
  }

  open func addLogin(_ login: LoginData) async throws {
    try login.validate()

    let nowMicro = Date.nowMicroseconds()
    let nowMilli = nowMicro / 1000
    let dateMicro = nowMicro
    let dateMilli = nowMilli

    let args: Args = [
      login.hostname,
      login.httpRealm,
      login.formSubmitURL,
      login.usernameField,
      login.passwordField,
      login.username,
      login.password,
      login.guid,
      dateMicro,  // timeCreated
      dateMicro,  // timeLastUsed
      dateMicro,  // timePasswordChanged
      dateMilli,  // localModified
    ]

    let sql = """
      INSERT OR IGNORE INTO loginsL (
          -- Shared fields.
          hostname,
          httpRealm,
          formSubmitURL,
          usernameField,
          passwordField,
          timesUsed,
          username,
          password,
          -- Local metadata.
          guid,
          timeCreated,
          timeLastUsed,
          timePasswordChanged,
          local_modified,
          is_deleted,
          sync_status
      )
      VALUES (?, ?, ?, ?, ?, 1, ?, ?, ?, ?, ?, ?, ?, 0, \(LoginsSchema.SyncStatus.new.rawValue))
      """

    try await db.run(sql, withArgs: args)
    self.notifyLoginDidChange()
  }

  fileprivate func cloneMirrorToOverlay(whereClause: String?, args: Args?) async throws -> Int {
    let shared = "guid, hostname, httpRealm, formSubmitURL, usernameField, passwordField, timeCreated, timeLastUsed, timePasswordChanged, timesUsed, username, password "
    let local = ", local_modified, is_deleted, sync_status "
    let sql = "INSERT OR IGNORE INTO loginsL (\(shared)\(local)) SELECT \(shared), NULL AS local_modified, 0 AS is_deleted, 0 AS sync_status FROM loginsM \(whereClause ?? "")"
    return try await self.db.write(sql, withArgs: args)
  }

  /**
     * Returns success if either a local row already existed, or
     * one could be copied from the mirror.
     */
  fileprivate func ensureLocalOverlayExistsForGUID(_ guid: GUID) async throws {
    let sql = "SELECT guid FROM loginsL WHERE guid = ?"
    let args: Args = [guid]
    let rows = try await db.runQuery(sql, args: args, factory: { _ in 1 })
    if rows.count > 0 {
      return
    }
    Logger.module.debug("No overlay; cloning one for GUID \(guid).")
    let count = try await self.cloneMirrorToOverlay(guid)
    if count > 0 {
      return
    }
    Logger.module.warning("Failed to create local overlay for GUID \(guid).")
    throw NoSuchRecordError(guid: guid)
  }

  fileprivate func cloneMirrorToOverlay(_ guid: GUID) async throws -> Int {
    let whereClause = "WHERE guid = ?"
    let args: Args = [guid]

    return try await self.cloneMirrorToOverlay(whereClause: whereClause, args: args)
  }

  fileprivate func markMirrorAsOverridden(_ guid: GUID) async throws {
    let args: Args = [guid]
    let sql = "UPDATE loginsM SET is_overridden = 1 WHERE guid = ?"

    return try await self.db.run(sql, withArgs: args)
  }

  /**
     * Replace the local DB row with the provided GUID.
     * If no local overlay exists, one is first created.
     *
     * If `significant` is `true`, the `sync_status` of the row is bumped to at least `Changed`.
     * If it's already `New`, it remains marked as `New`.
     *
     * This flag allows callers to make minor changes (such as incrementing a usage count)
     * without triggering an upload or a conflict.
     */
  open func updateLoginByGUID(_ guid: GUID, new: LoginData, significant: Bool) async throws {
    try new.validate()

    // Right now this method is only ever called if the password changes at
    // point of use, so we always set `timePasswordChanged` and `timeLastUsed`.
    // We can (but don't) also assume that `significant` will always be `true`,
    // at least for the time being.
    let nowMicro = Date.nowMicroseconds()
    let nowMilli = nowMicro / 1000
    let dateMicro = nowMicro
    let dateMilli = nowMilli

    let args: Args = [
      dateMilli,  // local_modified
      dateMicro,  // timeLastUsed
      dateMicro,  // timePasswordChanged
      new.httpRealm,
      new.formSubmitURL,
      new.usernameField,
      new.passwordField,
      new.password,
      new.hostname,
      new.username,
      guid,
    ]

    let update = """
      UPDATE loginsL SET
          local_modified = ?, timeLastUsed = ?, timePasswordChanged = ?,
          httpRealm = ?, formSubmitURL = ?, usernameField = ?,
          passwordField = ?, timesUsed = timesUsed + 1,
          password = ?, hostname = ?, username = ?
          -- We keep rows marked as New in preference to marking them as changed. This allows us to
          -- delete them immediately if they don't reach the server.
          \(significant ? ", sync_status = max(sync_status, 1)" : "")
      WHERE guid = ?
      """

    try await self.ensureLocalOverlayExistsForGUID(guid)
    try await self.markMirrorAsOverridden(guid)
    try await self.db.run(update, withArgs: args)
    self.notifyLoginDidChange()
  }

  open func addUseOfLoginByGUID(_ guid: GUID) async throws {
    let sql = """
      UPDATE loginsL SET
          timesUsed = timesUsed + 1, timeLastUsed = ?, local_modified = ?
      WHERE guid = ? AND is_deleted = 0
      """

    // For now, mere use is not enough to flip sync_status to Changed.

    let nowMicro = Date.nowMicroseconds()
    let nowMilli = nowMicro / 1000
    let args: Args = [nowMicro, nowMilli, guid]

    try await self.ensureLocalOverlayExistsForGUID(guid)
    try await self.markMirrorAsOverridden(guid)
    try await self.db.run(sql, withArgs: args)
  }

  open func removeLoginByGUID(_ guid: GUID) async throws {
    return try await removeLoginsWithGUIDs([guid])
  }

  fileprivate func getDeletionStatementsForGUIDs(_ guids: ArraySlice<GUID>, nowMillis: Timestamp) -> [(sql: String, args: Args?)] {
    let inClause = BrowserDB.varlist(guids.count)

    // Immediately delete anything that's marked as new -- i.e., it's never reached
    // the server.
    let delete =
      "DELETE FROM loginsL WHERE guid IN \(inClause) AND sync_status = \(LoginsSchema.SyncStatus.new.rawValue)"

    // Otherwise, mark it as changed.
    let update = """
      UPDATE loginsL SET
          local_modified = \(nowMillis),
          sync_status = \(LoginsSchema.SyncStatus.changed.rawValue),
          is_deleted = 1,
          password = '',
          hostname = '',
          username = ''
      WHERE guid IN \(inClause)
      """

    let markMirrorAsOverridden =
      "UPDATE loginsM SET is_overridden = 1 WHERE guid IN \(inClause)"

    let insert = """
      INSERT OR IGNORE INTO loginsL (
          guid, local_modified, is_deleted, sync_status, hostname, timeCreated, timePasswordChanged, password, username
      )
      SELECT
          guid, \(nowMillis), 1, \(LoginsSchema.SyncStatus.changed.rawValue), '', timeCreated, \(nowMillis)000, '', ''
      FROM loginsM
      WHERE guid IN \(inClause)
      """

    let args: Args = guids.map { $0 }
    return [(delete, args), (update, args), (markMirrorAsOverridden, args), (insert, args)]
  }

  open func removeLoginsWithGUIDs(_ guids: [GUID]) async throws {
    let timestamp = Date.now()
    try await db.run(
      chunk(guids, by: BrowserDB.maxVariableNumber).flatMap {
        self.getDeletionStatementsForGUIDs($0, nowMillis: timestamp)
      })
    self.notifyLoginDidChange()
  }

  open func removeAll() async throws {
    // Immediately delete anything that's marked as new -- i.e., it's never reached
    // the server. If Sync isn't set up, this will be everything.
    let delete =
      "DELETE FROM loginsL WHERE sync_status = \(LoginsSchema.SyncStatus.new.rawValue)"

    let nowMillis = Date.now()

    // Mark anything we haven't already deleted.
    let update =
      "UPDATE loginsL SET local_modified = \(nowMillis), sync_status = \(LoginsSchema.SyncStatus.changed.rawValue), is_deleted = 1, password = '', hostname = '', username = '' WHERE is_deleted = 0"

    // Copy all the remaining rows from our mirror, marking them as locally deleted. The
    // OR IGNORE will cause conflicts due to non-unique guids to be dropped, preserving
    // anything we already deleted.
    let insert = """
      INSERT OR IGNORE INTO loginsL (
          guid, local_modified, is_deleted, sync_status, hostname, timeCreated, timePasswordChanged, password, username
      )
      SELECT
          guid, \(nowMillis), 1, \(LoginsSchema.SyncStatus.changed.rawValue), '', timeCreated, \(nowMillis)000, '', ''
      FROM loginsM
      """

    // After that, we mark all of the mirror rows as overridden.
    try await self.db.run(delete)
    try await self.db.run(update)
    try await self.db.run("UPDATE loginsM SET is_overridden = 1")
    try await self.db.run(insert)
    self.notifyLoginDidChange()
  }
}

class NoSuchRecordError: Error {
  let guid: GUID
  init(guid: GUID) {
    self.guid = guid
  }
  var description: String {
    return "No such record: \(guid)."
  }
}

extension SDRow {
  func getTimestamp(_ column: String) -> Timestamp? {
    return (self[column] as? NSNumber)?.uint64Value
  }

  func getBoolean(_ column: String) -> Bool {
    if let val = self[column] as? Int {
      return val != 0
    }
    return false
  }
}
