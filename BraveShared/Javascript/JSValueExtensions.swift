// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import JavaScriptCore

extension JSValue {
    /// Calls javascript function and listens for errors.
    /// Errors caught in context are returned with its message,
    /// rest of errors are returned as `unknown` type of error.
    func tryToCall(withArguments arguments: [Any], expectReturnValue: Bool = true) throws -> JSValue {
        guard let result = call(withArguments: arguments) else {
            throw JSValue.unknownError
        }
        
        if let exception = context.exception {
            throw exception.toString()
        }
        
        // In JavaScript, if a function does not explicitly return a value,
        // it implicitly returns the value undefined.
        if expectReturnValue && result.isUndefined {
            throw JSValue.unknownError
        }
        
        return result
    }
    
    /// Throw this error if undefined behavior happens or you don't care about error type/message.
    public static let unknownError = "Unknown error"
}
