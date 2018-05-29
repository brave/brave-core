/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import Shared
import SwiftyJSON

/*
 * Due to sync's 'callbacks' running through the same function, parameters are reused
 * for ever single function call, resulting in a complex web of peculiar naming.
 *
 * Direct key mappings are using to pluck the variable names from the data, and an attempt
 * to make them more native feeling (e.g. descriptive names) has been made. In some cases
 * variable names are still generic due to the extreme usage of them (i.e. no nice way to make non-generic)
 *
 * At some point a switch to fullblown generic names may need necessary, but this hybrid approach seemed best
 * at the time of building it
 */

typealias SyncDefaultResponseType = SyncRecord
final class SyncResponse {
    
    // MARK: Declaration for string constants to be used to decode and also serialize.
    fileprivate struct SerializationKeys {
        static let arg2 = "arg2"
        static let message = "message"
        static let arg1 = "arg1"
        static let arg3 = "arg3"
        static let arg4 = "arg4" // isTruncated
    }
    
    // MARK: Properties
    // TODO: rename this property
    var rootElements: JSON? // arg2
    var message: String?
    var arg1: String?
    var lastFetchedTimestamp: Int? // arg3
    var isTruncated: Bool? // arg4
    
    /// Initiates the instance based on the object.
    ///
    /// - parameter object: The object of either Dictionary or Array kind that was passed.
    /// - returns: An initialized instance of the class.
    convenience init(object: String) {
        self.init(json: JSON(parseJSON: object))
    }
    
    /// Initiates the instance based on the JSON that was passed.
    ///
    /// - parameter json: JSON object from SwiftyJSON.
    required init(json: JSON?) {
        rootElements = json?[SerializationKeys.arg2]
        
        message = json?[SerializationKeys.message].string
        arg1 = json?[SerializationKeys.arg1].string
        lastFetchedTimestamp = json?[SerializationKeys.arg3].int
        isTruncated = json?[SerializationKeys.arg4].bool
    }
}
