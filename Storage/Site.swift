/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import Shared

public protocol Identifiable: Equatable {
    var id: Int? { get set }
}

public func ==<T>(lhs: T, rhs: T) -> Bool where T: Identifiable {
    return lhs.id == rhs.id
}

public enum IconType: Int {
    case icon, appleIcon, appleIconPrecomposed, guess, local, noneFound
    
    public func isPreferredTo (_ other: IconType) -> Bool {
        return rank > other.rank
    }
    
    fileprivate var rank: Int {
        switch self {
        case .appleIconPrecomposed:
            return 5
        case .appleIcon:
            return 4
        case .icon:
            return 3
        case .local:
            return 2
        case .guess:
            return 1
        case .noneFound:
            return 0
        }
    }
}

open class Favicon: Identifiable {
    open var id: Int?

    public let url: String
    public let date: Date
    open var width: Int?
    open var height: Int?
    public let type: IconType?

    // BRAVE TODO: consider removing `type` optional
    public init(url: String, date: Date = Date(), type: IconType? = nil) {
        self.url = url
        self.date = date
        self.type = type
    }
}

// TODO: Site shouldn't have all of these optional decorators. Include those in the
// cursor results, perhaps as a tuple.
open class Site: Identifiable, Hashable {
    open var id: Int?
    var guid: String?

    open var tileURL: URL {
        return URL(string: url)?.domainURL ?? URL(string: "about:blank")!
    }

    public let url: String
    public let title: String
    open var metadata: PageMetadata?
     // Sites may have multiple favicons. We'll return the largest.
    open var icon: Favicon?
    open fileprivate(set) var bookmarked: Bool?

    public convenience init(url: String, title: String) {
        self.init(url: url, title: title, bookmarked: false, guid: nil)
    }

    public init(url: String, title: String, bookmarked: Bool?, guid: String? = nil) {
        self.url = url
        self.title = title
        self.bookmarked = bookmarked
        self.guid = guid
    }

    open func setBookmarked(_ bookmarked: Bool) {
        self.bookmarked = bookmarked
    }

    // This hash is a bit limited in scope, but contains enough data to make a unique distinction.
    //  If modified, verify usage elsewhere, as places may rely on the hash only including these two elements.
    public func hash(into hasher: inout Hasher) {
        hasher.combine(url)
        hasher.combine(title)
    }
}
