/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_NETWORKING_MODELS_EXTENSIONS_BRAVE_CERTIFICATE_EXTENSION_PRIV_H_
#define BRAVE_IOS_BROWSER_API_NETWORKING_MODELS_EXTENSIONS_BRAVE_CERTIFICATE_EXTENSION_PRIV_H_

#import "BraveCertificateExtensionModel.h"

@interface BraveCertificateExtensionModel(Private)
- (instancetype)initWithType:(NSUInteger)type withExtension:(X509_EXTENSION*)extension;
@end

#endif  //  BRAVE_IOS_BROWSER_API_NETWORKING_MODELS_EXTENSIONS_BRAVE_CERTIFICATE_EXTENSION_PRIV_H_ 
