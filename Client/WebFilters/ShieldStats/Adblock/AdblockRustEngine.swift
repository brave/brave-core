// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Shared
import BraveShared

private let log = Logger.browserLogger

/// A wrapper around adblock_rust_lib header.
class AdblockRustEngine {
    private let engine: OpaquePointer
    
    var deserializationPending = false
    
    init(rules: String = "") { engine = engine_create(rules) }
    deinit { engine_destroy(engine) }
    
    func shouldBlock(requestUrl: String, requestHost: String, sourceHost: String) -> Bool {
        if deserializationPending {
            log.info("Not checking for blocked resource when engine deserialization is pending")
            return false
        }
        
        var explicitCancel = false
        var savedFromException = false
        let thirdPartyRequest = requestHost != sourceHost
        
        var emptyPointer: UnsafeMutablePointer<Int8>?
        
        return engine_match(engine, requestUrl, requestHost,
                                 sourceHost, thirdPartyRequest, "script",
                                 &explicitCancel, &savedFromException,
                                 UnsafeMutablePointer(mutating: &emptyPointer))
    }
    
    @discardableResult func set(data: Data) -> Bool {
        // Extra safety check to prevent race condition in case engine deserialization
        // is called from another thread than `shouldBlock` invocation(#2699).
        deserializationPending = true
        
        let status = engine_deserialize(engine, data.int8Array, data.count)
        if !status { assertionFailure("Failed to deserialize engine") }
        
        deserializationPending = false
        return status
    }
}
