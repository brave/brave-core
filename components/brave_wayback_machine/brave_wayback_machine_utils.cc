/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wayback_machine/brave_wayback_machine_utils.h"

#include <string>
#include <vector>

#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/components/brave_wayback_machine/url_constants.h"
#include "net/base/url_util.h"
#include "url/gurl.h"
#include "url/url_util.h"

bool IsWaybackMachineDisabledFor(const GURL& url) {
  if (net::IsLocalhost(url))
    return true;

  if (base::EndsWith(url.host(), ".local", base::CompareCase::SENSITIVE))
    return true;

  if (base::EndsWith(url.host(), ".onion", base::CompareCase::SENSITIVE))
    return true;

  // Disable on web.archive.org
  if (url.host() == kWaybackHost)
    return true;

  return false;
}

GURL FixupWaybackQueryURL(const GURL& url) {
  constexpr char kTimeStampKey[] = "timestamp";
  constexpr char kCallbackKey[] = "callback";

  // Get latest page always from wayback machine by deleting timestamp and
  // callback keys in query string.
  // To find encoded key, compares keys after decoding.
  std::vector<std::string> query_parts;
  std::string fragment;
  for (net::QueryIterator it(url); !it.IsAtEnd(); it.Advance()) {
    std::string key = std::string(it.GetKey());
    url::RawCanonOutputW<1024> canonOutput;
    url::DecodeURLEscapeSequences(key.c_str(), key.length(),
                                  url::DecodeURLMode::kUTF8OrIsomorphic,
                                  &canonOutput);
    const std::string decoded_key = base::UTF16ToUTF8(
        base::StringPiece16(canonOutput.data(), canonOutput.length()));
    // Skip target keys.
    if (decoded_key == kTimeStampKey || decoded_key == kCallbackKey)
      continue;

    query_parts.push_back(base::StringPrintf(
        "%s=%s", key.c_str(), std::string(it.GetValue()).c_str()));
  }

  std::string query = base::JoinString(query_parts, "&");

  GURL::Replacements replacements;
  replacements.SetQueryStr(query);
  return url.ReplaceComponents(replacements);
}
