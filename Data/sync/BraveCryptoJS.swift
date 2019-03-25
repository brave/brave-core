// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveShared

struct BraveCryptoJS: BrowserifyExposable, BundledJSProtocol {
    let bundle = Bundle.data
    let name = "crypto"
    let exposedFunctions = Functions.exposedFunctions
    
    enum Functions: String {
        case fromBytesOrHex, toBytes32
        
        /// Some of crypto.js functions are placed behind this namespace.
        private static let passphraseNamespace = "passphrase."
        
        var nameSpace: String {
            switch self {
            case .fromBytesOrHex, .toBytes32: return Functions.passphraseNamespace
            }
        }
        
        var arguments: String {
            switch self {
            case .fromBytesOrHex: return "bytes"
            case .toBytes32: return "passphrase"
            }
        }
        
        var body: String {
            return generateFunction(name: nameSpace + rawValue, arguments: arguments)
        }
        
        /// Creates new instance on access, thus private
        private var exposedFunction: ExposedJSFunction {
            return ExposedJSFunction(name: rawValue, body: body)
        }
        
        static var exposedFunctions: [ExposedJSFunction] {
            return [fromBytesOrHex.exposedFunction, toBytes32.exposedFunction]
        }
    }
}
