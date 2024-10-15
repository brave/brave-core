/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_JSON_JSON_EXPORT_H_
#define BRAVE_COMPONENTS_JSON_JSON_EXPORT_H_

#if defined(COMPONENT_BUILD)
#if defined(WIN32)

#if defined(JSON_HELPER_IMPLEMENTATION)
#define JSON_HELPER_EXPORT __declspec(dllexport)
#else
#define JSON_HELPER_EXPORT __declspec(dllimport)
#endif  // defined(JSON_HELPER_IMPLEMENTATION)

#else  // defined(WIN32)
#if defined(JSON_HELPER_IMPLEMENTATION)
#define JSON_HELPER_EXPORT __attribute__((visibility("default")))
#else
#define JSON_HELPER_EXPORT
#endif  // defined(JSON_HELPER_IMPLEMENTATION)
#endif

#else  // defined(COMPONENT_BUILD)
#define JSON_HELPER_EXPORT
#endif

#endif  // BRAVE_COMPONENTS_JSON_JSON_EXPORT_H_
