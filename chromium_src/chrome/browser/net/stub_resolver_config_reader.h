/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_NET_STUB_RESOLVER_CONFIG_READER_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_NET_STUB_RESOLVER_CONFIG_READER_H_

#define StubResolverConfigReader StubResolverConfigReader_ChromiumImpl
#include <chrome/browser/net/stub_resolver_config_reader.h>  // IWYU pragma: export
#undef StubResolverConfigReader

class StubResolverConfigReader : public StubResolverConfigReader_ChromiumImpl {
 public:
  explicit StubResolverConfigReader(PrefService* local_state,
                                    bool set_up_pref_defaults = true)
      : StubResolverConfigReader_ChromiumImpl(local_state,
                                              set_up_pref_defaults) {}
  bool ShouldDisableDohForManaged() override;
};

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_NET_STUB_RESOLVER_CONFIG_READER_H_
