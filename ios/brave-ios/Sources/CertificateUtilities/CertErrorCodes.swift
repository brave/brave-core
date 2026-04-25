// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import CoreFoundation
import Foundation
import Security
import Shared

// Regardless of cause, cfurlErrorServerCertificateUntrusted is currently returned in all cases.
// Check the other cases in case this gets fixed in the future.
// NOTE: In rare cases like Bad Cipher algorithm, it can show cfurlErrorSecureConnectionFailed
// swift-format-ignore
extension CFNetworkErrors {
  public static let braveCertificatePinningFailed = CFNetworkErrors(rawValue: Int32.min)!
  public static let certErrors: [CFNetworkErrors] = [
    .cfurlErrorSecureConnectionFailed,
    .cfurlErrorServerCertificateHasBadDate,
    .cfurlErrorServerCertificateUntrusted,
    .cfurlErrorServerCertificateHasUnknownRoot,
    .cfurlErrorServerCertificateNotYetValid,
    .cfurlErrorClientCertificateRejected,
    .cfurlErrorClientCertificateRequired,
    .braveCertificatePinningFailed,
  ]
}
