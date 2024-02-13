// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation

public extension Sequence {
  func asyncForEach(_ operation: (Element) async throws -> Void) async rethrows {
    for element in self {
      try await operation(element)
    }
  }

  func asyncConcurrentForEach(_ operation: @escaping (Element) async throws -> Void) async rethrows {
    await withThrowingTaskGroup(of: Void.self) { group in
      for element in self {
        group.addTask { try await operation(element) }
      }
    }
  }

  func asyncMap<T>(_ transform: (Element) async throws -> T) async rethrows -> [T] {
    var results = [T]()
    for element in self {
      try await results.append(transform(element))
    }
    return results
  }

  func asyncConcurrentMap<T>(_ transform: @escaping (Element) async throws -> T) async rethrows -> [T] {
    try await withThrowingTaskGroup(of: T.self) { group in
      for element in self {
        group.addTask { try await transform(element) }
      }

      var results = [T]()
      for try await result in group {
        results.append(result)
      }
      return results
    }
  }

  func asyncCompactMap<T>(_ transform: (Element) async throws -> T?) async rethrows -> [T] {
    var results = [T]()
    for element in self {
      if let result = try await transform(element) {
        results.append(result)
      }
    }
    return results
  }

  func asyncConcurrentCompactMap<T>(_ transform: @escaping (Element) async throws -> T?) async rethrows -> [T] {
    try await withThrowingTaskGroup(of: T?.self) { group in
      for element in self {
        group.addTask {
          try await transform(element)
        }
      }

      var results = [T]()
      for try await result in group {
        if let result = result {
          results.append(result)
        }
      }
      return results
    }
  }
  
  func asyncFilter(_ isIncluded: (Element) async throws -> Bool) async rethrows -> [Element] {
    var results = [Element]()
    for element in self {
      if try await isIncluded(element) {
        results.append(element)
      }
    }
    return results
  }
  
  func asyncConcurrentFilter(_ isIncluded: @escaping (Element) async throws -> Bool) async rethrows -> [Element] {
    try await withThrowingTaskGroup(of: Element?.self) { group in
      for element in self {
        group.addTask {
          if try await isIncluded(element) {
            return element
          } else {
            return nil
          }
        }
      }

      var results = [Element]()
      for try await result in group {
        if let result = result {
          results.append(result)
        }
      }
      return results
    }
  }
}

public extension Task where Failure == Error {
  @discardableResult
  static func retry(
    priority: TaskPriority? = nil,
    retryCount: Int = 3,
    retryDelay: TimeInterval = 1,
    operation: @Sendable @escaping () async throws -> Success
  ) -> Task {
    Task(priority: priority) {
      for _ in 0..<retryCount {
        try Task<Never, Never>.checkCancellation()

        do {
          return try await operation()
        } catch {
          try await Task<Never, Never>.sleep(nanoseconds: UInt64(retryDelay) * NSEC_PER_SEC)
          continue
        }
      }

      try Task<Never, Never>.checkCancellation()
      return try await operation()
    }
  }
}

public extension Task where Success == Never, Failure == Never {
  /// Suspends the current task for at least the given duration
  /// in seconds.
  ///
  /// If the task is canceled before the time ends,
  /// this function throws `CancellationError`.
  ///
  /// This function doesn't block the underlying thread.
  static func sleep(seconds: TimeInterval) async throws {
    try await sleep(nanoseconds: NSEC_PER_MSEC * UInt64(seconds * 1000))
  }
}

public extension Task where Failure == Error {
  @discardableResult
  static func delayed(
    bySeconds seconds: TimeInterval,
    priority: TaskPriority? = nil,
    operation: @escaping @Sendable () async throws -> Success
  ) -> Task {
    Task(priority: priority) {
      try await Task<Never, Never>.sleep(seconds: seconds)
      return try await operation()
    }
  }
}
