/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_SESSIONS_CORE_SERIALIZED_NAVIGATION_ENTRY_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_SESSIONS_CORE_SERIALIZED_NAVIGATION_ENTRY_H_

#include <optional>
#include <string>

#define set_http_status_code(...)                                            \
  NotUsed() const;                                                           \
                                                                             \
 private:                                                                    \
  std::optional<std::pair<std::string, std::string>> storage_partition_key_; \
  std::string virtual_url_prefix_;                                           \
                                                                             \
 public:                                                                     \
  const std::optional<std::pair<std::string, std::string>>&                  \
  storage_partition_key() const {                                            \
    return storage_partition_key_;                                           \
  }                                                                          \
  void set_storage_partition_key(                                            \
      const std::pair<std::string, std::string>& key) {                      \
    storage_partition_key_ = key;                                            \
  }                                                                          \
  const std::string& virtual_url_prefix() const {                            \
    return virtual_url_prefix_;                                              \
  }                                                                          \
  void set_virtual_url_prefix(const std::string& url_prefix) {               \
    virtual_url_prefix_ = url_prefix;                                        \
  }                                                                          \
  void set_http_status_code(__VA_ARGS__)

#include <components/sessions/core/serialized_navigation_entry.h>  // IWYU pragma: export

#undef set_http_status_code

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_SESSIONS_CORE_SERIALIZED_NAVIGATION_ENTRY_H_
