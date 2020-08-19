/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_UI_BRAVE_CUSTOM_NOTIFICATION_PUBLIC_CPP_BRAVE_CUSTOM_NOTIFICATION_PUBLIC_EXPORT_H_
#define BRAVE_UI_BRAVE_CUSTOM_NOTIFICATION_PUBLIC_CPP_BRAVE_CUSTOM_NOTIFICATION_PUBLIC_EXPORT_H_

#if defined(COMPONENT_BUILD)
#if defined(WIN32)

#if defined(BRAVE_CUSTOM_NOTIFICATION_PUBLIC_IMPLEMENTATION)
#define BRAVE_CUSTOM_NOTIFICATION_PUBLIC_EXPORT __declspec(dllexport)
#else
#define BRAVE_CUSTOM_NOTIFICATION_PUBLIC_EXPORT __declspec(dllimport)
#endif  // defined(BRAVE_CUSTOM_NOTIFICATION_PUBLIC_IMPLEMENTATION)

#else  // defined(WIN32)
#if defined(BRAVE_CUSTOM_NOTIFICATION_PUBLIC_IMPLEMENTATION)
#define BRAVE_CUSTOM_NOTIFICATION_PUBLIC_EXPORT __attribute__((visibility("default")))
#else
#define BRAVE_CUSTOM_NOTIFICATION_PUBLIC_EXPORT
#endif  // defined(BRAVE_CUSTOM_NOTIFICATION_PUBLIC_IMPLEMENTATION)
#endif

#else  // defined(COMPONENT_BUILD)
#define BRAVE_CUSTOM_NOTIFICATION_PUBLIC_EXPORT
#endif

#endif  // UI_BRAVE_CUSTOM_NOTIFICATION_PUBLIC_CPP_BRAVE_CUSTOM_NOTIFICATION_PUBLIC_EXPORT_H_
