/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_EXPORT_H_
#define BAT_ADS_EXPORT_H_

#if defined(STANDALONE_BUILD)
#if defined(WIN32)

#if defined(BASE_IMPLEMENTATION)
#define ADS_EXPORT __declspec(dllexport)
#else
#define ADS_EXPORT __declspec(dllimport)
#endif  // defined(BASE_IMPLEMENTATION)

#else  // defined(WIN32)
#define ADS_EXPORT __attribute__((visibility("default")))
#endif

#else  // defined(STANDALONE_BUILD)
#define ADS_EXPORT
#endif

#endif  // BAT_ADS_EXPORT_H_
