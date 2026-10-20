// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation

/// A pending piece of work that can be cancelled.
public protocol TransferTimer: AnyObject {
  func cancel()
}

/// The clock and serial queue the engine runs on. Every engine object is
/// confined to one scheduler: all of its methods must be called on it and all
/// of its callbacks run on it.
public protocol TransferScheduler: AnyObject {
  /// Seconds since the Unix epoch.
  var now: TimeInterval { get }
  /// Runs `work` later on the scheduler, never synchronously.
  func post(_ work: @escaping () -> Void)
  @discardableResult
  func schedule(after delay: TimeInterval, _ work: @escaping () -> Void) -> TransferTimer
}

extension TransferScheduler {
  /// Runs `work` every `interval` seconds until the returned timer is
  /// cancelled.
  public func scheduleRepeating(
    every interval: TimeInterval, _ work: @escaping () -> Void
  ) -> TransferTimer {
    let timer = RepeatingTimer()
    func arm() {
      timer.current = schedule(after: interval) { [weak timer] in
        guard let timer, !timer.isCancelled else { return }
        work()
        if !timer.isCancelled {
          arm()
        }
      }
    }
    arm()
    return timer
  }
}

private final class RepeatingTimer: TransferTimer {
  var current: TransferTimer?
  var isCancelled = false

  func cancel() {
    isCancelled = true
    current?.cancel()
    current = nil
  }
}

/// Runs work on a serial `DispatchQueue`.
public final class DispatchQueueScheduler: TransferScheduler {
  public let queue: DispatchQueue

  public init(queue: DispatchQueue) {
    self.queue = queue
  }

  public var now: TimeInterval { Date().timeIntervalSince1970 }

  public func post(_ work: @escaping () -> Void) {
    queue.async(execute: work)
  }

  @discardableResult
  public func schedule(after delay: TimeInterval, _ work: @escaping () -> Void) -> TransferTimer {
    let item = DispatchWorkItem(block: work)
    queue.asyncAfter(deadline: .now() + delay, execute: item)
    return DispatchTimer(item: item)
  }
}

private final class DispatchTimer: TransferTimer {
  let item: DispatchWorkItem

  init(item: DispatchWorkItem) {
    self.item = item
  }

  func cancel() {
    item.cancel()
  }
}
