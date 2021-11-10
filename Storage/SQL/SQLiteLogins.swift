/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import Shared
import XCGLogger

private let log = Logger.syncLogger

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
        let credential = URLCredential(user: row["username"] as? String ?? "",
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

        let protectionSpace = URLProtectionSpace(host: host,
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
        log.debug("Notifying login did change.")

        // For now we don't care about the contents.
        // This posts immediately to the shared notification center.
        NotificationCenter.default.post(name: .dataLoginDidChange, object: nil)
    }

    open func getUsageDataForLoginByGUID(_ guid: GUID) -> Deferred<Maybe<LoginUsageData>> {
        let projection = SQLiteLogins.loginColumns
        let sql = """
            SELECT \(projection)
            FROM loginsL
            WHERE is_deleted = 0 AND guid = ?
            UNION ALL
            SELECT \(projection)
            FROM loginsM
            WHERE is_overridden = 0 AND guid = ?
            LIMIT 1
            """

        let args: Args = [guid, guid]
        return db.runQuery(sql, args: args, factory: SQLiteLogins.loginUsageDataFactory)
            >>== { value in
            deferMaybe(value[0]!)
        }
    }

    open func getLoginDataForGUID(_ guid: GUID) -> Deferred<Maybe<Login>> {
        let projection = SQLiteLogins.loginColumns
        let sql = """
            SELECT \(projection)
            FROM loginsL
            WHERE is_deleted = 0 AND guid = ?
            UNION ALL
            SELECT \(projection)
            FROM loginsM
            WHERE is_overriden IS NOT 1 AND guid = ?
            ORDER BY hostname ASC
            LIMIT 1
            """

        let args: Args = [guid, guid]
        return db.runQuery(sql, args: args, factory: SQLiteLogins.loginFactory)
            >>== { value in
            if let login = value[0] {
                return deferMaybe(login)
            } else {
                return deferMaybe(LoginDataError(description: "Login not found for GUID \(guid)"))
            }
        }
    }

    open func getLoginsForProtectionSpace(_ protectionSpace: URLProtectionSpace) -> Deferred<Maybe<Cursor<LoginData>>> {
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
        if Logger.logPII {
            log.debug("Looking for login: \(protectionSpace.urlString()) && \(protectionSpace.host)")
        }
        return db.runQuery(sql, args: args, factory: SQLiteLogins.loginDataFactory)
    }

    // username is really Either<String, NULL>; we explicitly match no username.
    open func getLoginsForProtectionSpace(_ protectionSpace: URLProtectionSpace, withUsername username: String?) -> Deferred<Maybe<Cursor<LoginData>>> {
        let projection = SQLiteLogins.mainWithLastUsedColumns

        let args: Args
        let usernameMatch: String
        if let username = username {
            args = [
                protectionSpace.urlString(), username, protectionSpace.host,
                protectionSpace.urlString(), username, protectionSpace.host
            ]
            usernameMatch = "username = ?"
        } else {
            args = [
                protectionSpace.urlString(), protectionSpace.host,
                protectionSpace.urlString(), protectionSpace.host
            ]
            usernameMatch = "username IS NULL"
        }

        if Logger.logPII {
            log.debug("Looking for login with username: \(username ?? "nil"), first arg: \(args[0] ?? "nil")")
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

        return db.runQuery(sql, args: args, factory: SQLiteLogins.loginDataFactory)
    }

    open func getAllLogins() -> Deferred<Maybe<Cursor<Login>>> {
        return searchLoginsWithQuery(nil)
    }
    
    open func getLoginsForQuery(_ query: String) -> Deferred<Maybe<Cursor<Login>>> {
        return searchLoginsWithQuery(query)
    }

    open func searchLoginsWithQuery(_ query: String?) -> Deferred<Maybe<Cursor<Login>>> {
        let projection = SQLiteLogins.loginColumns
        var searchClauses = [String]()
        var args: Args?
        if let query = query, !query.isEmpty {
            // Add wildcards to change query to 'contains in' and add them to args. We need 6 args because
            // we include the where clause twice: Once for the local table and another for the remote.
            args = (0..<6).map { _ in
                return "%\(query)%" as String?
            }

            searchClauses.append("username LIKE ? ")
            searchClauses.append(" password LIKE ? ")
            searchClauses.append(" hostname LIKE ?")
        }

        let whereSearchClause = searchClauses.count > 0 ? "AND (" + searchClauses.joined(separator: "OR") + ") " : ""
        let sql = """
            SELECT \(projection)
            FROM loginsL
            WHERE is_deleted = 0 \(whereSearchClause)
            UNION ALL
            SELECT \(projection)
            FROM loginsM
            WHERE is_overridden = 0 \(whereSearchClause)
            ORDER BY hostname ASC
            """

        return db.runQuery(sql, args: args, factory: SQLiteLogins.loginFactory)
    }

    open func addLogin(_ login: LoginData) -> Success {
        if let error = login.isValid.failureValue {
            return deferMaybe(error)
        }

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
            dateMicro,            // timeCreated
            dateMicro,            // timeLastUsed
            dateMicro,            // timePasswordChanged
            dateMilli,            // localModified
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

        return db.run(sql, withArgs: args)
                >>> effect(self.notifyLoginDidChange)
    }

    fileprivate func cloneMirrorToOverlay(whereClause: String?, args: Args?) -> Deferred<Maybe<Int>> {
        let shared = "guid, hostname, httpRealm, formSubmitURL, usernameField, passwordField, timeCreated, timeLastUsed, timePasswordChanged, timesUsed, username, password "
        let local = ", local_modified, is_deleted, sync_status "
        let sql = "INSERT OR IGNORE INTO loginsL (\(shared)\(local)) SELECT \(shared), NULL AS local_modified, 0 AS is_deleted, 0 AS sync_status FROM loginsM \(whereClause ?? "")"
        return self.db.write(sql, withArgs: args)
    }

    /**
     * Returns success if either a local row already existed, or
     * one could be copied from the mirror.
     */
    fileprivate func ensureLocalOverlayExistsForGUID(_ guid: GUID) -> Success {
        let sql = "SELECT guid FROM loginsL WHERE guid = ?"
        let args: Args = [guid]
        let c = db.runQuery(sql, args: args, factory: { _ in 1 })

        return c >>== { rows in
            if rows.count > 0 {
                return succeed()
            }
            log.debug("No overlay; cloning one for GUID \(guid).")
            return self.cloneMirrorToOverlay(guid)
                >>== { count in
                    if count > 0 {
                        return succeed()
                    }
                    log.warning("Failed to create local overlay for GUID \(guid).")
                    return deferMaybe(NoSuchRecordError(guid: guid))
            }
        }
    }

    fileprivate func cloneMirrorToOverlay(_ guid: GUID) -> Deferred<Maybe<Int>> {
        let whereClause = "WHERE guid = ?"
        let args: Args = [guid]

        return self.cloneMirrorToOverlay(whereClause: whereClause, args: args)
    }

    fileprivate func markMirrorAsOverridden(_ guid: GUID) -> Success {
        let args: Args = [guid]
        let sql = "UPDATE loginsM SET is_overridden = 1 WHERE guid = ?"

        return self.db.run(sql, withArgs: args)
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
    open func updateLoginByGUID(_ guid: GUID, new: LoginData, significant: Bool) -> Success {
        if let error = new.isValid.failureValue {
            return deferMaybe(error)
        }

        // Right now this method is only ever called if the password changes at
        // point of use, so we always set `timePasswordChanged` and `timeLastUsed`.
        // We can (but don't) also assume that `significant` will always be `true`,
        // at least for the time being.
        let nowMicro = Date.nowMicroseconds()
        let nowMilli = nowMicro / 1000
        let dateMicro = nowMicro
        let dateMilli = nowMilli

        let args: Args = [
            dateMilli,            // local_modified
            dateMicro,            // timeLastUsed
            dateMicro,            // timePasswordChanged
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

        return self.ensureLocalOverlayExistsForGUID(guid)
           >>> { self.markMirrorAsOverridden(guid) }
           >>> { self.db.run(update, withArgs: args) }
            >>> effect(self.notifyLoginDidChange)
    }

    open func addUseOfLoginByGUID(_ guid: GUID) -> Success {
        let sql = """
            UPDATE loginsL SET
                timesUsed = timesUsed + 1, timeLastUsed = ?, local_modified = ?
            WHERE guid = ? AND is_deleted = 0
            """

        // For now, mere use is not enough to flip sync_status to Changed.

        let nowMicro = Date.nowMicroseconds()
        let nowMilli = nowMicro / 1000
        let args: Args = [nowMicro, nowMilli, guid]

        return self.ensureLocalOverlayExistsForGUID(guid)
           >>> { self.markMirrorAsOverridden(guid) }
           >>> { self.db.run(sql, withArgs: args) }
    }

    open func removeLoginByGUID(_ guid: GUID) -> Success {
        return removeLoginsWithGUIDs([guid])
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
        return [ (delete, args), (update, args), (markMirrorAsOverridden, args), (insert, args)]
    }

    open func removeLoginsWithGUIDs(_ guids: [GUID]) -> Success {
        let timestamp = Date.now()
        return db.run(chunk(guids, by: BrowserDB.maxVariableNumber).flatMap {
            self.getDeletionStatementsForGUIDs($0, nowMillis: timestamp)
        }) >>> effect(self.notifyLoginDidChange)
    }

    open func removeAll() -> Success {
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
        return self.db.run(delete)
           >>> { self.db.run(update) }
           >>> { self.db.run("UPDATE loginsM SET is_overridden = 1") }
           >>> { self.db.run(insert) }
            >>> effect(self.notifyLoginDidChange)
    }
}

class NoSuchRecordError: MaybeErrorType {
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
