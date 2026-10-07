import Foundation

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// Encapsulate a callback in a way that we can use it with NSTimer.
private class Callback {
  private let handler: () -> Void

  init(handler: @escaping () -> Void) {
    self.handler = handler
  }

  @objc
  func go() {
    handler()
  }
}

/// Taken from http://stackoverflow.com/questions/27116684/how-can-i-debounce-a-method-call
/// Allows creating a block that will fire after a delay. Resets the timer if called again before the delay expires.
///  *
public func debounce(_ delay: TimeInterval, action: @escaping () -> Void) -> () -> Void {
  let callback = Callback(handler: action)
  var timer: Timer?

  return {
    // If calling again, invalidate the last timer.
    if let timer = timer {
      timer.invalidate()
    }
    timer = Timer(
      timeInterval: delay,
      target: callback,
      selector: #selector(Callback.go),
      userInfo: nil,
      repeats: false
    )
    RunLoop.current.add(timer!, forMode: RunLoop.Mode.default)
  }
}
