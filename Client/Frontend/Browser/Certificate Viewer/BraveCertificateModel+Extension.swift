// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Security
import BraveCore

extension BraveCertificateModel {
  convenience init?(name: String) {
    if let certificate = Self.loadCertificate(name: name) {
      self.init(certificate: certificate)
    } else {
      return nil
    }
  }

  private static func loadCertificate(name: String) -> SecCertificate? {
    guard let certificateData = loadCertificateData(name: name) else {
      return nil
    }
    return SecCertificateCreateWithData(nil, certificateData)
  }

  private static func loadCertificateData(name: String) -> CFData? {
    guard let path = Bundle.module.path(forResource: name, ofType: "cer") else {
      return nil
    }

    guard let certificateData = try? Data(contentsOf: URL(fileURLWithPath: path)) as CFData else {
      return nil
    }
    return certificateData
  }
}
