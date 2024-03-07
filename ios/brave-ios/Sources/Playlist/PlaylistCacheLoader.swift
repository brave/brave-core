// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import AVFoundation
import BraveCore
import BraveShields
import Data
import Foundation
import MobileCoreServices
import Preferences
import Shared
import WebKit
import os.log

public protocol PlaylistWebLoaderFactory {
  func makeWebLoader() -> any PlaylistWebLoader
}

public protocol PlaylistWebLoader: UIView {
  func load(url: URL) async -> PlaylistInfo?
  func stop()
}
