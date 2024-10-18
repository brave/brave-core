// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "src/ios/web_view/internal/cwv_x509_certificate.mm"

@implementation CWVX509Certificate (Internal)
- (scoped_refptr<net::X509Certificate>)internalCertificate {
  return _internalCertificate;
}
@end
