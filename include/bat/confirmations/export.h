/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_CONFIRMATIONS_EXPORT_H_
#define BAT_CONFIRMATIONS_EXPORT_H_

#if defined(COMPONENT_BUILD)
#if defined(WIN32)
#define CONFIRMATIONS_EXPORT __declspec(dllexport)
#define CONFIRMATIONS_EXPORT __declspec(dllimport)
#else  // defined(WIN32)
#define CONFIRMATIONS_EXPORT __attribute__((visibility("default")))
#endif

#else  // defined(COMPONENT_BUILD)
#define CONFIRMATIONS_EXPORT
#endif

#endif  // BAT_CONFIRMATIONS_EXPORT_H_
