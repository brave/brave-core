// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#import "ios/web/security/crw_cert_verification_controller.h"

#define decideLoadPolicyForTrust decideLoadPolicyForTrust_ChromiumImpl
#define querySSLStatusForTrust querySSLStatusForTrust_ChromiumImpl
#include "src/ios/web/security/crw_cert_verification_controller.mm"
#undef querySSLStatusForTrust
#undef decideLoadPolicyForTrust

#include "brave/ios/browser/api/net/certificate_utility.h"

@implementation CRWCertVerificationController (Override)

- (void)verifyTrustWithPinning:(base::apple::ScopedCFTypeRef<SecTrustRef>)trust
                          host:(NSString*)host
             completionHandler:(void (^)(bool trusted))completionHandler {
  base::ThreadPool::PostTask(
      FROM_HERE, {TaskShutdownBehavior::BLOCK_SHUTDOWN, base::MayBlock()},
      base::BindOnce(^{
        auto result = [BraveCertificateUtility verifyTrust:trust.get()
                                                      host:host
                                                      port:443];
        bool trusted =
            result == 0 || result == std::numeric_limits<std::int32_t>::min();
        // TODO(crbug.com/40588591): This should use PostTask to post to
        // WebThread::UI with BLOCK_SHUTDOWN once shutdown behaviors are
        // supported on the UI thread. BLOCK_SHUTDOWN is necessary because
        // WKWebView throws an exception if the completion handler doesn't run.
        dispatch_async(dispatch_get_main_queue(), ^{
          completionHandler(trusted);
        });
      }));
}

- (void)decideLoadPolicyForTrust:
            (base::apple::ScopedCFTypeRef<SecTrustRef>)trust
                            host:(NSString*)host
               completionHandler:(web::PolicyDecisionHandler)completionHandler {
  DCHECK_CURRENTLY_ON(WebThread::UI);
  DCHECK(completionHandler);
  [self
      decideLoadPolicyForTrust_ChromiumImpl:trust
                                       host:host
                          completionHandler:^(web::CertAcceptPolicy policy,
                                              net::CertStatus status) {
                            if (policy == web::CERT_ACCEPT_POLICY_ALLOW ||
                                policy ==
                                    web::
                                        CERT_ACCEPT_POLICY_RECOVERABLE_ERROR_ACCEPTED_BY_USER) {
                              // SSL pinning check
                              [self
                                  verifyTrustWithPinning:trust
                                                    host:host
                                       completionHandler:^(bool trusted) {
                                         completionHandler(
                                             trusted
                                                 ? policy
                                                 : web::
                                                       CERT_ACCEPT_POLICY_NON_RECOVERABLE_ERROR,
                                             trusted
                                                 ? status
                                                 : net::
                                                       CERT_STATUS_PINNED_KEY_MISSING);
                                       }];
                              return;
                            }
                            completionHandler(policy, status);
                          }];
}

- (void)querySSLStatusForTrust:(base::apple::ScopedCFTypeRef<SecTrustRef>)trust
                          host:(NSString*)host
             completionHandler:(web::StatusQueryHandler)completionHandler {
  [self
      querySSLStatusForTrust_ChromiumImpl:trust
                                     host:host
                        completionHandler:^(web::SecurityStyle style,
                                            net::CertStatus status) {
                          if (status !=
                              web::SECURITY_STYLE_AUTHENTICATION_BROKEN) {
                            // SSL pinning check
                            [self
                                verifyTrustWithPinning:trust
                                                  host:host
                                     completionHandler:^(bool trusted) {
                                       completionHandler(
                                           trusted
                                               ? style
                                               : web::
                                                     SECURITY_STYLE_AUTHENTICATION_BROKEN,
                                           trusted
                                               ? status
                                               : net::
                                                     CERT_STATUS_PINNED_KEY_MISSING);
                                     }];
                            return;
                          }
                          completionHandler(style, status);
                        }];
}

@end
