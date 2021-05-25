/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#import <Foundation/Foundation.h>
#import "BraveCertificateExtensionModel.h"

#ifndef BRAVE_IOS_BROWSER_API_NETWORKING_MODELS_EXTENSIONS_BRAVE_CERTIFICATE_POLICY_MAPPINGS_EXTENSION_H_
#define BRAVE_IOS_BROWSER_API_NETWORKING_MODELS_EXTENSIONS_BRAVE_CERTIFICATE_POLICY_MAPPINGS_EXTENSION_H_

NS_ASSUME_NONNULL_BEGIN

OBJC_EXPORT
@interface BraveCertificatePolicyMappingExtensionModel: NSObject
@property(nonatomic, strong, readonly) NSString* issuerDomainPolicy;
@property(nonatomic, strong, readonly) NSString* subjectDomainPolicy;
@end

OBJC_EXPORT
@interface BraveCertificatePolicyMappingsExtensionModel: BraveCertificateExtensionModel
@property(nonatomic, strong, readonly) NSArray<BraveCertificatePolicyMappingExtensionModel*>* policies;
@end

NS_ASSUME_NONNULL_END

#endif  //  BRAVE_IOS_BROWSER_API_NETWORKING_MODELS_EXTENSIONS_BRAVE_CERTIFICATE_POLICY_MAPPINGS_EXTENSION_H_
