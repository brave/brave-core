//
//  AdBlockRustEngine.swift
//  AdblockRust
//
//  Created by Kyle Hickinson on 2021-01-13.
//

import Foundation

extension Data {
  var int8Array: [Int8] {
    return self.map { Int8(bitPattern: $0) }
  }
}

/// A wrapper around adblock_rust_lib header.
public class AdblockRustEngine {
  private let engine: OpaquePointer
  
  var deserializationPending = false
  
  public init(rules: String = "") { engine = engine_create(rules) }
  deinit { engine_destroy(engine) }
  
  public func shouldBlock(requestUrl: String, requestHost: String, sourceHost: String) -> Bool {
    if deserializationPending {
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
  
  @discardableResult public func set(data: Data) -> Bool {
    // Extra safety check to prevent race condition in case engine deserialization
    // is called from another thread than `shouldBlock` invocation(#2699).
    deserializationPending = true
    
    let status = engine_deserialize(engine, data.int8Array, data.count)
    if !status { assertionFailure("Failed to deserialize engine") }
    
    deserializationPending = false
    return status
  }
}
