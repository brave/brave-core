// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "build/build_config.h"
#include "extensions/buildflags/buildflags.h"

#if defined(OS_ANDROID)
#include "ui/native_theme/native_theme.h"

#define IDR_DEFAULT_FAVICON_32 IDR_DEFAULT_FAVICON
#define IDR_DEFAULT_FAVICON_64 IDR_DEFAULT_FAVICON
#define IDR_DEFAULT_FAVICON_DARK_32 IDR_DEFAULT_FAVICON_DARK
#define IDR_DEFAULT_FAVICON_DARK_64 IDR_DEFAULT_FAVICON_DARK
// FaviconSource was excluded from Android builds
// https://chromium.googlesource.com/chromium/src/+/2ad1441f59880e901664277108e4a490f4b6ea88
// But it is still used for icons in rewards webui, including Android page.
// Thus we exclude desktop related code from it for Android builds.
#if !BUILDFLAG(ENABLE_EXTENSIONS)
// Exclude extension headers to avoid build errors
#define BRAVE_CHROMIUM_SRC_EXTENSIONS_COMMON_CONSTANTS_H_
#define EXTENSIONS_BROWSER_EXTENSION_REGISTRY_H_
#define EXTENSIONS_COMMON_CONSTANTS_H_
#define EXTENSIONS_COMMON_MANIFEST_H_
#include "chrome/browser/profiles/profile.h"
#include "url/gurl.h"
// Just dummy values to avoid build errors
namespace {
namespace extensions {
class Manifest {
 public:
  enum Type { NUM_LOAD_TYPES };
};
class Extension {
 public:
  Manifest::Type GetType() const { return Manifest::Type::NUM_LOAD_TYPES; }
};
class UnusedClassB {
 public:
  UnusedClassB() {}
  Extension* GetExtensionOrAppByURL(const GURL& url) { return nullptr; }
};
class UnusedClassA {
 public:
  UnusedClassA() {}
  UnusedClassB enabled_extensions() { return UnusedClassB(); }
};
class ExtensionRegistry {
 public:
  static std::unique_ptr<UnusedClassA> Get(Profile* profile) {
    return std::make_unique<UnusedClassA>();
  }
};
}  // namespace extensions
}  // namespace
#endif  // #if !BUILDFLAG(ENABLE_EXTENSIONS)
// InstantService is only used on desktop
#define CHROME_BROWSER_SEARCH_INSTANT_SERVICE_H_
// Dummy class to workaround InstantService code on Android
class InstantService {
 public:
  static bool ShouldServiceRequest(const GURL& url,
                                   content::BrowserContext* browser_context,
                                   int render_process_id) {
    return false;
  }
};

// Toolkit views are not enabled for Android, so just fallback to what we had
// before the change.
#if !defined(TOOLKIT_VIEWS)
namespace webui {
ui::NativeTheme* GetNativeTheme(content::WebContents* web_contents) {
  return ui::NativeTheme::BraveGetInstanceForNativeUi();
}
}  // namespace webui
#endif  // !defined(TOOLKIT_VIEWS)

#endif  // #if defined(OS_ANDROID)

#include "src/chrome/browser/ui/webui/favicon_source.cc"

#if defined(OS_ANDROID)
#if !BUILDFLAG(ENABLE_EXTENSIONS)
#undef EXTENSIONS_BROWSER_EXTENSION_REGISTRY_H_
#undef EXTENSIONS_COMMON_CONSTANTS_H_
#undef EXTENSIONS_COMMON_MANIFEST_H_
#endif  // #if !BUILDFLAG(ENABLE_EXTENSIONS)
#endif  // #if defined(OS_ANDROID)
