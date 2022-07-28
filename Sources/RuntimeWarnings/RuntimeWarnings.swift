// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import os

#if DEBUG
/// Runtime warning DSO handler and OSLog to support showing inline warnings within Xcode like the main thread
/// checker and SwiftUI runtime issues.
///
/// Use with a standard `os_log`:
///
///     os_log(.fault, dso: os_rw.dso, log: os_rw.log(), "Runtime warning!")
///
/// https://www.pointfree.co/blog/posts/70-unobtrusive-runtime-warnings-for-libraries
public enum os_rw {
  public static let dso: UnsafeMutableRawPointer = {
    var info = Dl_info()
    dladdr(dlsym(dlopen(nil, RTLD_LAZY), "LocalizedString"), &info)
    return info.dli_fbase
  }()
  public static func log(category: String = "Brave") -> OSLog {
    .init(subsystem: "com.apple.runtime-issues", category: category)
  }
}
#endif
