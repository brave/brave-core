// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Foundation
import Storage

extension TabDataValues {
  private struct CertStoreKey: TabDataKey {
    static var defaultValue: CertStore? = nil
  }

  /// A reference to a certificate store that can be used to check if a certificate has been allowed
  /// by the user directly via an error page.
  ///
  /// WebKit only
  public var certificateStore: CertStore? {
    get { self[CertStoreKey.self] }
    set { self[CertStoreKey.self] = newValue }
  }
}
