/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation

/**
 Assertion for checking that the call is being made on the main thread.

 - parameter message: Message to display in case of assertion.
 */
public func assertIsMainThread(_ message: String) {
  assert(Thread.isMainThread, message)
}

// Simple timer for manual profiling. Not for production use.
// Prints only if timing is longer than a threshold (to reduce noisy output).
open class PerformanceTimer {
  let startTime: CFAbsoluteTime
  var endTime: CFAbsoluteTime?
  let threshold: Double
  let label: String

  public init(thresholdSeconds: Double = 0.001, label: String = "") {
    self.threshold = thresholdSeconds
    self.label = label
    startTime = CFAbsoluteTimeGetCurrent()
  }

  public func stopAndPrint() {
    if let t = stop() {
      print("Ran for \(t) seconds. [\(label)]")
    }
  }

  public func stop() -> String? {
    endTime = CFAbsoluteTimeGetCurrent()
    if let duration = duration {
      return "\(duration)"
    }
    return nil
  }

  public var duration: CFAbsoluteTime? {
    if let endTime = endTime {
      let time = endTime - startTime
      return time > threshold ? time : nil
    } else {
      return nil
    }
  }
}
