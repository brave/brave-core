// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_IOS_WEB_VIEW_INTERNAL_CWV_X509_CERTIFICATE_INTERNAL_H_
#define BRAVE_CHROMIUM_SRC_IOS_WEB_VIEW_INTERNAL_CWV_X509_CERTIFICATE_INTERNAL_H_

#include <ios/web_view/internal/cwv_x509_certificate_internal.h>  // IWYU pragma: export

// Expose the underlying net::X509Certificate that is not public so that
// cwv_x509_certificate_extras.mm can access the private property and expose
// additional functionality
@interface CWVX509Certificate (Internal)
@property(readonly) scoped_refptr<net::X509Certificate> internalCertificate;
@end

#endif  // BRAVE_CHROMIUM_SRC_IOS_WEB_VIEW_INTERNAL_CWV_X509_CERTIFICATE_INTERNAL_H_
