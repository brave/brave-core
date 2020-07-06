// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BRAVE_UI_BRAVE_CUSTOM_NOTIFICATION_PUBLIC_CPP_BRAVE_CUSTOM_NOTIFICATION_PUBLIC_EXPORT_H_
#define BRAVE_UI_BRAVE_CUSTOM_NOTIFICATION_PUBLIC_CPP_BRAVE_CUSTOM_NOTIFICATION_PUBLIC_EXPORT_H_

// Defines BRAVE_CUSTOM_NOTIFICATION_PUBLIC_EXPORT so that functionality implemented by the
// message_center module can be exported to consumers.

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
