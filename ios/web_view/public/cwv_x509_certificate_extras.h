// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_WEB_VIEW_PUBLIC_CWV_X509_CERTIFICATE_EXTRAS_H_
#define BRAVE_IOS_WEB_VIEW_PUBLIC_CWV_X509_CERTIFICATE_EXTRAS_H_

#include "cwv_x509_certificate.h"  // NOLINT

/// Adds additional functionality to CWVX509Certificate that is not be supported
/// out of the box but can be implemented using the underlying
/// net::X509Certificate
@interface CWVX509Certificate (Extras)
/// The underlying security reference
@property(readonly, nullable) SecCertificateRef certificateRef;
@end

#endif  // BRAVE_IOS_WEB_VIEW_PUBLIC_CWV_X509_CERTIFICATE_EXTRAS_H_
