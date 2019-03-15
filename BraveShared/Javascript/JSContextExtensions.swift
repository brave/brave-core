// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import JavaScriptCore

public extension JSContext {
    /// Converts Swift's array to Javascript's Uint8Array.
    /// Returns nil if one or more of array elements can't be casted to UInt8
    func arrayToUint8Array(values: [Int]) -> JSValue? {
        // Make sure all integers are castable to Uint8
        if values.contains(where: { UInt8(exactly: $0) == nil }) { return nil }
        
        guard let result = evaluateScript("new Uint8Array(\(values))") else { return nil }
        
        return result.isUndefined ? nil : result
    }
}
