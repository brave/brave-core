/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BraveCertificateExtensionModel_Priv_h
#define BraveCertificateExtensionModel_Priv_h

#import "BraveCertificateExtensionModel.h"

@interface BraveCertificateExtensionModel(Private)
- (instancetype)initWithType:(NSUInteger)type withExtension:(X509_EXTENSION*)extension;
@end

#endif /* BraveCertificateExtensionModel_Priv_h */
