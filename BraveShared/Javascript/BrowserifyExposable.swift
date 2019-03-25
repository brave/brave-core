// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import JavaScriptCore

public struct ExposedJSFunction {
    /// Name of the function that will be called from JSContext.
    let name: String
    /// Should be in format: `function(args) { // call browserified method; };`
    let body: String
    
    public init(name: String, body: String) {
        self.name = name
        self.body = body
    }
}

/// Browserify doesn't let you access the functions and modules from outside of the browserified code
/// Therefore functions needs to be exposed by wrapping them up on a global object.
public protocol BrowserifyExposable {
    /// Functions that are wrapped up to be globally available for Javascript core.
    var exposedFunctions: [ExposedJSFunction] { get }
    
    /// Ads exposed functions to JSContext so they can be called using `globalObject` or `window`
    func addExposedFunctions(to context: JSContext)
    
    /// A helper method to generate a function that can be called from JavascriptCore.
    static func generateFunction(name: String, arguments: String) -> String
}

public extension BrowserifyExposable {
    func addExposedFunctions(to context: JSContext) {
        exposedFunctions.forEach {
            context.evaluateScript("\($0.name) = \($0.body)")
        }
    }
    
    static func generateFunction(name: String, arguments: String = "") -> String {
        return """
        function(\(arguments)) {
        return module.exports.\(name)(\(arguments))
        }
        """
    }
}
