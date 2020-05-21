// Copyright (c) 2019 The Brave Authors
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "build/build_config.h"
#include "extensions/buildflags/buildflags.h"

#if defined(OS_ANDROID)
#include "ui/native_theme/native_theme.h"

#define GetInstanceForNativeUi BraveGetInstanceForNativeUi
#define IDR_DEFAULT_FAVICON_32 IDR_DEFAULT_FAVICON
#define IDR_DEFAULT_FAVICON_64 IDR_DEFAULT_FAVICON
#define IDR_DEFAULT_FAVICON_DARK_32 IDR_DEFAULT_FAVICON_DARK
#define IDR_DEFAULT_FAVICON_DARK_64 IDR_DEFAULT_FAVICON_DARK
#if !BUILDFLAG(ENABLE_EXTENSIONS)
// Exclude extension headers to avoid build errors
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
#endif  // #if defined(OS_ANDROID)

#include "../../../../../chrome/browser/ui/webui/favicon_source.cc"  // NOLINT

#if defined(OS_ANDROID)
#if !BUILDFLAG(ENABLE_EXTENSIONS)
#undef EXTENSIONS_BROWSER_EXTENSION_REGISTRY_H_
#undef EXTENSIONS_COMMON_CONSTANTS_H_
#undef EXTENSIONS_COMMON_MANIFEST_H_
#endif  // #if !BUILDFLAG(ENABLE_EXTENSIONS)
#endif  // #if defined(OS_ANDROID)
