// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ios/web_view/internal/cwv_x509_certificate_internal.h"
#include "net/cert/x509_util_apple.h"

@implementation CWVX509Certificate (Extras)

- (SecCertificateRef)certificateRef {
  return net::x509_util::CreateSecCertificateFromX509Certificate(
             self.internalCertificate.get())
      .release();  // Swift bridging should handle the lifetime
}

@end
