// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_IOS_WEB_SECURITY_CRW_CERT_VERIFICATION_CONTROLLER_H_
#define BRAVE_CHROMIUM_SRC_IOS_WEB_SECURITY_CRW_CERT_VERIFICATION_CONTROLLER_H_

#define decideLoadPolicyForTrust decideLoadPolicyForTrust_ChromiumImpl
#define querySSLStatusForTrust querySSLStatusForTrust_ChromiumImpl
#include "src/ios/web/security/crw_cert_verification_controller.h"  // IWYU pragma: export
#undef querySSLStatusForTrust
#undef decideLoadPolicyForTrust

@interface CRWCertVerificationController (Override)
- (void)decideLoadPolicyForTrust:
            (base::apple::ScopedCFTypeRef<SecTrustRef>)trust
                            host:(NSString*)host
               completionHandler:(web::PolicyDecisionHandler)completionHandler;
- (void)querySSLStatusForTrust:(base::apple::ScopedCFTypeRef<SecTrustRef>)trust
                          host:(NSString*)host
             completionHandler:(web::StatusQueryHandler)completionHandler;
@end

#endif  // BRAVE_CHROMIUM_SRC_IOS_WEB_SECURITY_CRW_CERT_VERIFICATION_CONTROLLER_H_
