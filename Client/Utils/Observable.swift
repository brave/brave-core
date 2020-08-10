// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation

/// A property wrapper which notifies observers when it's wrapped value changes
///
/// TODO: Replace usages of this with Combine/@Published when minimum deployment target is iOS 13
@propertyWrapper struct Observable<ValueType> {
    typealias Handler = (_ oldValue: ValueType, _ newValue: ValueType) -> Void
    
    private class Observer {
        let closure: Handler
        let queue: DispatchQueue
        init(_ queue: DispatchQueue, _ closure: @escaping Handler) {
            self.queue = queue
            self.closure = closure
        }
    }
    private var observers = NSMapTable<AnyObject, Observer>.weakToStrongObjects()
    
    mutating func observe(
        from object: AnyObject,
        on queue: DispatchQueue = .main,
        _ handler: @escaping Handler
    ) {
        observers.setObject(Observer(queue, handler), forKey: object)
    }
    
    init(wrappedValue: ValueType) {
        self.wrappedValue = wrappedValue
    }
    
    var wrappedValue: ValueType {
        didSet {
            guard let iterator = observers.objectEnumerator() else { return }
            let newValue = wrappedValue
            for observer in iterator {
                if let observer = observer as? Observer {
                    observer.queue.async {
                        observer.closure(oldValue, newValue)
                    }
                }
            }
        }
    }
}
