// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Shared

private let log = Logger.browserLogger

/// A javascript file stored in app's Bundle.
/// The file does not need to be statically copied into the app.
/// You can get a javacsript file using a package manager like npm
/// then link that file(as a reference) to the project to make it visible
/// in the Bundle.
public protocol BundledJSProtocol {
    /// Name of the javascript file without .js extension.
    var name: String { get }
    /// Bundle where the javascript file is located.
    var bundle: Bundle { get }
    /// Gets a bundled javascript file and returns it as a String.
    var get: String? { get }
}

public extension BundledJSProtocol {
    var get: String? {
        do {
            guard let filePath = bundle.path(forResource: name, ofType: "js") else {
                throw "Could not find script named: \(name)"
            }
            
            return try String(contentsOfFile: filePath, encoding: String.Encoding.utf8)
        } catch {
            log.error("Could not find or parse script named: \(name)")
            return nil
        }
    }
}
