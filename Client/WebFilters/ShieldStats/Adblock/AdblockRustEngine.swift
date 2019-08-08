// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation

/// A wrapper around adblock_rust_lib header.
class AdblockRustEngine {
    private let engine: OpaquePointer
    
    init(rules: String = "") { engine = engine_create(rules) }
    deinit { engine_destroy(engine) }
    
    func shouldBlock(requestUrl: String, requestHost: String, sourceHost: String) -> Bool {
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
        return engine_deserialize(engine, data.int8Array, data.count)
    }
}
