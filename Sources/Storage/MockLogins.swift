/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import Shared

open class MockLogins: BrowserLogins {
  fileprivate var cache = [Login]()

  public init(files: FileAccessor) {
  }

  open func getLoginsForProtectionSpace(_ protectionSpace: URLProtectionSpace) -> Cursor<LoginData> {
    let cursor = ArrayCursor(
      data: cache.filter({ login in
        return login.protectionSpace.host == protectionSpace.host
      }).sorted(by: { (loginA, loginB) -> Bool in
        return loginA.timeLastUsed > loginB.timeLastUsed
      }).map({ login in
        return login as LoginData
      }))
    return cursor
  }

  open func getLoginsForProtectionSpace(_ protectionSpace: URLProtectionSpace, withUsername username: String?) -> Cursor<LoginData> {
    let cursor = ArrayCursor(
      data: cache.filter({ login in
        return login.protectionSpace.host == protectionSpace.host && login.username == username
      }).sorted(by: { (loginA, loginB) -> Bool in
        return loginA.timeLastUsed > loginB.timeLastUsed
      }).map({ login in
        return login as LoginData
      }))
    return cursor
  }

  open func getAllLogins() -> Cursor<Login> {
    let cursor = ArrayCursor(
      data: cache.sorted(by: { (loginA, loginB) -> Bool in
        return loginA.hostname > loginB.hostname
      }))
    return cursor
  }
  
  open func addLogin(_ login: LoginData) async throws {
    if let _ = cache.firstIndex(of: login as! Login) {
      throw LoginDataError(description: "Already in the cache")
    }
    cache.append(login as! Login)
  }

  open func updateLoginByGUID(_ guid: GUID, new: LoginData, significant: Bool) async throws {
    // TODO
  }

  open func getModifiedLoginsToUpload() -> [Login] {
    // TODO
    return []
  }

  open func getDeletedLoginsToUpload() -> [GUID] {
    // TODO
    return []
  }

  open func updateLogin(_ login: LoginData) async throws {
    if let index = cache.firstIndex(of: login as! Login) {
      cache[index].timePasswordChanged = Date.nowMicroseconds()
      return
    }
    throw LoginDataError(description: "Password wasn't cached yet. Can't update")
  }

  open func addUseOfLoginByGUID(_ guid: GUID) async throws {
    if let login = cache.filter({ $0.guid == guid }).first {
      login.timeLastUsed = Date.nowMicroseconds()
      return
    }
    throw LoginDataError(description: "Password wasn't cached yet. Can't update")
  }

  open func removeLoginByGUID(_ guid: GUID) async throws {
    let filtered = cache.filter { $0.guid != guid }
    if filtered.count == cache.count {
      throw LoginDataError(description: "Can not remove a password that wasn't stored")
    }
    cache = filtered
  }

  open func removeLoginsWithGUIDs(_ guids: [GUID]) async throws {
    for guid in guids {
      try await self.removeLoginByGUID(guid)
    }
  }

  open func removeAll() async throws {
    cache.removeAll(keepingCapacity: false)
  }

  open func hasSyncedLogins() -> Bool {
    return true
  }

  // TODO
  open func deleteByGUID(_ guid: GUID, deletedAt: Timestamp) async throws { }
  open func markAsSynchronized<T: Collection>(_: T, modified: Timestamp) async throws -> Timestamp where T.Iterator.Element == GUID { return 0 }
  open func markAsDeleted<T: Collection>(_ guids: T) async throws where T.Iterator.Element == GUID { }
  open func onRemovedAccount() async throws { }
}
