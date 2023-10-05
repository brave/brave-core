// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import AVFoundation
import WebKit
import MobileCoreServices
import Data
import Shared
import BraveShields
import Preferences
import BraveCore
import Storage
import os.log

public protocol PlaylistWebLoaderFactory {
  func makeWebLoader() -> any PlaylistWebLoader
}

public protocol PlaylistWebLoader: UIView {
  func load(url: URL) async -> PlaylistInfo?
  func stop()
}
