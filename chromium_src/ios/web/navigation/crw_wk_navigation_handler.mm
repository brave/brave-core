// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ios/web/navigation/crw_wk_navigation_handler.h"

#import <Foundation/Foundation.h>
#include <Security/Security.h>

#include "ios/web/public/web_state.h"
#include "ios/web/security/crw_cert_verification_controller.h"
#include "net/cert/cert_status_flags.h"

@interface CRWWKNavigationHandler (Brave)

/// Returns YES if the cert policy & status matched a certificate pinning
/// violation and the resulting auth challenge was cancelled
- (BOOL)handleCertificatePinningForChallenge:
            (NSURLAuthenticationChallenge*)challenge
                            certAcceptPolicy:(web::CertAcceptPolicy)policy
                                  certStatus:(net::CertStatus)certStatus
                           completionHandler:
                               (void (^)(NSURLSessionAuthChallengeDisposition,
                                         NSURLCredential*))completionHandler;

@end

#define BRAVE_SHOULD_BLOCK_JAVASCRIPT                                          \
  brave::ShouldBlockJavaScript(static_cast<web::WebState*>(self.webStateImpl), \
                               action.request, preferences);
#define BRAVE_SHOULD_BLOCK_UNIVERSAL_LINKS                            \
  brave::ShouldBlockUniversalLinks(                                   \
      static_cast<web::WebState*>(self.webStateImpl), action.request, \
      &forceBlockUniversalLinks);
#define BRAVE_PROCESS_AUTH_CHALLENGE                                   \
  if ([self handleCertificatePinningForChallenge:challenge             \
                                certAcceptPolicy:policy                \
                                      certStatus:certStatus            \
                               completionHandler:completionHandler]) { \
    return;                                                            \
  }
#define BRAVE_HANDLE_LOAD_ERROR                        \
  if (policyDecisionCancellationError &&               \
      policyDecisionCancellationError.code ==          \
          net::ERR_SSL_PINNED_KEY_NOT_IN_CERT_CHAIN) { \
    policyDecisionCancellationError = nil;             \
  }
#define BRAVE_DID_FAIL_PROVISIONAL_NAVIGATION                                \
  if (self.pendingNavigationInfo.cancellationError &&                        \
      self.pendingNavigationInfo.cancellationError.code ==                   \
          net::ERR_SSL_PINNED_KEY_NOT_IN_CERT_CHAIN) {                       \
    NSMutableDictionary* userInfo =                                          \
        [self.pendingNavigationInfo.cancellationError.userInfo mutableCopy]; \
    userInfo[NSURLErrorFailingURLStringErrorKey] =                           \
        base::SysUTF8ToNSString(navigationContext->GetUrl().spec());         \
    userInfo[web::kNSErrorFailingURLKey] =                                   \
        [NSURL URLWithString:base::SysUTF8ToNSString(                        \
                                 navigationContext->GetUrl().spec())];       \
    error = [NSError errorWithDomain:NSURLErrorDomain                        \
                                code:NSURLErrorServerCertificateUntrusted    \
                            userInfo:userInfo];                              \
    self.pendingNavigationInfo.cancellationError = nil;                      \
    self.pendingNavigationInfo.cancelled = NO;                               \
  }
#include "src/ios/web/navigation/crw_wk_navigation_handler.mm"
#undef BRAVE_DID_FAIL_PROVISIONAL_NAVIGATION
#undef BRAVE_HANDLE_LOAD_ERROR
#undef BRAVE_PROCESS_AUTH_CHALLENGE
#undef BRAVE_SHOULD_BLOCK_UNIVERSAL_LINKS
#undef BRAVE_SHOULD_BLOCK_JAVASCRIPT

@implementation CRWWKNavigationHandler (Brave)

- (NSURL*)URLForProtectionSpace:(NSURLProtectionSpace*)protectionSpace {
  return [NSURL
      URLWithString:[NSString stringWithFormat:@"%@://%@:%ld",
                                               protectionSpace.protocol,
                                               protectionSpace.host,
                                               static_cast<long>(
                                                   protectionSpace.port)]];
}

- (NSArray*)certificateChainForTrust:(SecTrustRef)trust {
  DCHECK(trust);
  return (__bridge NSArray*)SecTrustCopyCertificateChain(trust);
}

- (BOOL)handleCertificatePinningForChallenge:
            (NSURLAuthenticationChallenge*)challenge
                            certAcceptPolicy:(web::CertAcceptPolicy)policy
                                  certStatus:(net::CertStatus)certStatus
                           completionHandler:
                               (void (^)(NSURLSessionAuthChallengeDisposition,
                                         NSURLCredential*))completionHandler {
  if (policy == web::CERT_ACCEPT_POLICY_NON_RECOVERABLE_ERROR &&
      (certStatus & net::CERT_STATUS_PINNED_KEY_MISSING) != 0) {
    if (self.pendingNavigationInfo) {
      self.pendingNavigationInfo.cancelled = YES;
      NSMutableDictionary* userInfo = [[NSMutableDictionary alloc] init];
      userInfo[web::kNSErrorPeerCertificateChainKey] =
          [self certificateChainForTrust:challenge.protectionSpace.serverTrust];
      self.pendingNavigationInfo.cancellationError =
          [NSError errorWithDomain:net::kNSErrorDomain
                              code:net::ERR_SSL_PINNED_KEY_NOT_IN_CERT_CHAIN
                          userInfo:userInfo];
    }

    if (SecTrustGetCertificateCount(challenge.protectionSpace.serverTrust)) {
      scoped_refptr<net::X509Certificate> leafCert = nil;
      base::apple::ScopedCFTypeRef<CFArrayRef> certificateChain(
          SecTrustCopyCertificateChain(challenge.protectionSpace.serverTrust));
      SecCertificateRef secCertificate =
          base::apple::CFCastStrict<SecCertificateRef>(
              CFArrayGetValueAtIndex(certificateChain.get(), 0));
      leafCert = net::x509_util::CreateX509CertificateFromSecCertificate(
          base::apple::ScopedCFTypeRef<SecCertificateRef>(
              secCertificate, base::scoped_policy::RETAIN),
          {});

      if (leafCert) {
        bool is_recoverable =
            policy ==
            web::CERT_ACCEPT_POLICY_RECOVERABLE_ERROR_UNDECIDED_BY_USER;
        std::string host =
            base::SysNSStringToUTF8(challenge.protectionSpace.host);
        _certVerificationErrors->Put(
            web::CertHostPair(leafCert, host),
            web::CertVerificationError(is_recoverable, certStatus));
      }
    }

    completionHandler(NSURLSessionAuthChallengeCancelAuthenticationChallenge,
                      nil);
    return YES;
  }
  return NO;
}

@end
