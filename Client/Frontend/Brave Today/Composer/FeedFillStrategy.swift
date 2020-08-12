// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation

/// Defines a ruleset for getting the next set of items from a list of `FeedItem`'s
protocol FillStrategy {
    /// Obtain the next `length` number of feed items from a list. If exactly `length` items can be queried,
    /// then those items are removed from `list` and returned.
    ///
    /// You can optionally provide some `predicate` to determine what items are valid in `list`
    ///
    /// - Returns: A set of feed items if `list` (or the filtered variant given some `predicate`) contains at
    ///            least `length` items.
    func next(
        _ length: Int,
        from list: inout [FeedItem],
        where predicate: ((FeedItem) -> Bool)?
    ) -> [FeedItem]?
}

extension FillStrategy {
    func next(
        _ length: Int,
        from list: inout [FeedItem],
        where predicate: ((FeedItem) -> Bool)? = nil
    ) -> [FeedItem]? {
        next(length, from: &list, where: predicate)
    }
    /// Obtain the next feed item from `list`. If that item can be queried successfully, then that item is
    /// removed from `list` and returned.
    func next(
        from list: inout [FeedItem],
        where predicate: ((FeedItem) -> Bool)? = nil
    ) -> FeedItem? {
        next(1, from: &list, where: predicate)?.first
    }
}

/// A fill strategy that always pulls from the beginning of the list
struct DefaultFillStrategy: FillStrategy {
    func next(
        _ length: Int,
        from list: inout [FeedItem],
        where predicate: ((FeedItem) -> Bool)? = nil
    ) -> [FeedItem]? {
        if let predicate = predicate {
            let filteredItems = list.filter(predicate)
            if filteredItems.count < length { return nil }
            let items = Array(filteredItems.prefix(upTo: length))
            items.forEach { item in
                if let index = list.firstIndex(of: item) {
                    list.remove(at: index)
                }
            }
            return items
        } else {
            if list.count < length { return nil }
            let items = Array(list.prefix(upTo: length))
            list.removeFirst(items.count)
            return items
        }
    }
}

/// A fill strategy that always pulls from the beginning of the list after said list has been filtered
/// by some given predicate
struct FilteredFillStrategy: FillStrategy {
    /// A global predicate to determine what items are valid to pull from. For example, only pulling items
    /// that are in a given category
    var isIncluded: ((FeedItem) -> Bool)
    
    func next(
        _ length: Int,
        from list: inout [FeedItem],
        where predicate: ((FeedItem) -> Bool)? = nil
    ) -> [FeedItem]? {
        let workingList = list.filter {
            (predicate?($0) ?? true) && isIncluded($0)
        }
        if workingList.count < length { return nil }
        let items = Array(workingList.prefix(upTo: length))
        items.forEach { item in
            if let index = list.firstIndex(of: item) {
                list.remove(at: index)
            }
        }
        return items
    }
}

/// A fill strategy that pulls random items from the list
struct RandomizedFillStrategy: FillStrategy {
    /// A global predicate to determine what random items are valid to pull from. For example, only pulling
    /// random items that are less than 48 hours old
    var isIncluded: ((FeedItem) -> Bool)?
    
    func next(
        _ length: Int,
        from list: inout [FeedItem],
        where predicate: ((FeedItem) -> Bool)? = nil
    ) -> [FeedItem]? {
        var workingList = list
        if predicate != nil || isIncluded != nil {
            workingList = workingList.filter {
                (predicate?($0) ?? true) && (isIncluded?($0) ?? true)
            }
        }
        if workingList.count < length { return nil }
        return (0..<length).compactMap { _ in
            if let index = workingList.indices.randomElement() {
                let item = workingList.remove(at: index)
                if let index = list.firstIndex(of: item) {
                    list.remove(at: index)
                }
                return item
            }
            return nil
        }
    }
}
