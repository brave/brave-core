/* Copyright (c) 2018 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/common/chrome_constants.h"

#include "build/build_config.h"
#include "chrome/common/chrome_version.h"

#define FPL FILE_PATH_LITERAL

#if BUILDFLAG(IS_MAC)
// CHROMIUM_SRC_INTERNAL_USE
#define PRODUCT_STRING BRAVE_PRODUCT_STRING
#endif  // BUILDFLAG(IS_MAC)

namespace chrome {

constexpr char kChromeVersion[] = CHROME_VERSION_STRING;

// The following should not be used for UI strings; they are meant
// for system strings only. UI changes should be made in the GRD.
//
// There are four constants used to locate the executable name and path:
//
//     kBrowserProcessExecutableName
//     kHelperProcessExecutableName
//     kBrowserProcessExecutablePath
//     kHelperProcessExecutablePath
//
// In one condition, our tests will be built using the Chrome branding
// though we want to actually execute a Chromium branded application.
// This happens for the reference build on Mac.  To support that case,
// we also include a Chromium version of each of the four constants and
// in the UITest class we support switching to that version when told to
// do so.

#if BUILDFLAG(IS_WIN)
const base::FilePath::CharType kBrowserProcessExecutableName[] =
    FPL("brave.exe");
const base::FilePath::CharType kHelperProcessExecutableName[] =
    FPL("brave.exe");
#elif BUILDFLAG(IS_MAC)
const base::FilePath::CharType kBrowserProcessExecutableName[] =
    FPL(PRODUCT_STRING);
const base::FilePath::CharType kHelperProcessExecutableName[] =
    FPL(PRODUCT_STRING " Helper");
#elif BUILDFLAG(IS_ANDROID)
// NOTE: Keep it synced with the process names defined in AndroidManifest.xml.
const base::FilePath::CharType kBrowserProcessExecutableName[] = FPL("brave");
const base::FilePath::CharType kHelperProcessExecutableName[] =
    FPL("sandboxed_process");
#elif BUILDFLAG(IS_POSIX)
const base::FilePath::CharType kBrowserProcessExecutableName[] = FPL("brave");
// Helper processes end up with a name of "exe" due to execing via
// /proc/self/exe.  See bug 22703.
const base::FilePath::CharType kHelperProcessExecutableName[] = FPL("exe");
#endif  // OS_*

#if BUILDFLAG(IS_WIN)
const base::FilePath::CharType kBrowserProcessExecutablePath[] =
    FPL("brave.exe");
const base::FilePath::CharType kHelperProcessExecutablePath[] =
    FPL("brave.exe");
#elif BUILDFLAG(IS_MAC)
const base::FilePath::CharType kBrowserProcessExecutablePath[] =
    FPL(PRODUCT_STRING ".app/Contents/MacOS/" PRODUCT_STRING);
const base::FilePath::CharType kHelperProcessExecutablePath[] =
    FPL(PRODUCT_STRING " Helper.app/Contents/MacOS/" PRODUCT_STRING " Helper");
#elif BUILDFLAG(IS_ANDROID)
const base::FilePath::CharType kBrowserProcessExecutablePath[] = FPL("brave");
const base::FilePath::CharType kHelperProcessExecutablePath[] = FPL("brave");
#elif BUILDFLAG(IS_POSIX)
const base::FilePath::CharType kBrowserProcessExecutablePath[] = FPL("brave");
const base::FilePath::CharType kHelperProcessExecutablePath[] = FPL("brave");
#endif  // OS_*

#if BUILDFLAG(IS_MAC)
const base::FilePath::CharType kFrameworkName[] =
    FPL(PRODUCT_STRING " Framework.framework");
const base::FilePath::CharType kFrameworkExecutableName[] =
    FPL(PRODUCT_STRING " Framework");
constexpr char kMacHelperSuffixAlerts[] = " (Alerts)";
#endif  // OS_MAC

}  // namespace chrome

#if BUILDFLAG(IS_MAC)
#undef PRODUCT_STRING
#endif  // BUILDFLAG(IS_MAC)
#undef FPL
