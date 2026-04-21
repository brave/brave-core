/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/webcat/core/csp_validator.h"

#include <algorithm>
#include <iterator>
#include <optional>
#include <string_view>
#include <vector>

#include "base/logging.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"

namespace webcat {

namespace {

std::vector<std::string_view> SplitCspDirectives(std::string_view csp) {
  return base::SplitStringPiece(csp, ";", base::TRIM_WHITESPACE,
                                base::SPLIT_WANT_NONEMPTY);
}

std::vector<std::string_view> SplitDirectiveValues(std::string_view values) {
  return base::SplitStringPiece(values, base::kWhitespaceASCII,
                                base::TRIM_WHITESPACE,
                                base::SPLIT_WANT_NONEMPTY);
}

bool IsAllowedDefaultSrcValue(std::string_view value) {
  return value == "'none'" || value == "'self'";
}

bool IsAllowedObjectSrcValue(std::string_view value) {
  return value == "'none'";
}

bool IsAllowedScriptSrcValue(std::string_view value) {
  return value == "'none'" || value == "'self'" ||
         value == "'wasm-unsafe-eval'" ||
         base::StartsWith(value, "'sha256-");
}

bool IsAllowedStyleSrcValue(std::string_view value) {
  return value == "'none'" || value == "'self'" ||
         value == "'unsafe-inline'" || value == "'unsafe-hashes'" ||
         base::StartsWith(value, "'sha256-") ||
         base::StartsWith(value, "'sha384-") ||
         base::StartsWith(value, "'sha512-");
}

bool IsAllowedFrameSrcValue(std::string_view value) {
  return value == "'none'" || value == "'self'" || value == "blob:" ||
         value == "data:";
}

bool IsAllowedWorkerSrcValue(std::string_view value) {
  return value == "'none'" || value == "'self'";
}

bool ValidateDirective(std::string_view directive_name,
                       std::string_view values_str,
                       CspValidationResult& result) {
  auto values = SplitDirectiveValues(values_str);
  if (values.empty()) {
    result.error_detail =
        "Empty values for directive: " + std::string(directive_name);
    return false;
  }

  for (const auto& value : values) {
    if (directive_name == "default-src") {
      if (!IsAllowedDefaultSrcValue(value)) {
        result.error_detail = "Disallowed value in default-src: " +
                              std::string(value);
        return false;
      }
    } else if (directive_name == "object-src") {
      if (!IsAllowedObjectSrcValue(value)) {
        result.error_detail = "object-src must be 'none', got: " +
                              std::string(value);
        return false;
      }
    } else if (directive_name == "script-src" ||
               directive_name == "script-src-elem") {
      if (!IsAllowedScriptSrcValue(value)) {
        result.error_detail = "Disallowed value in " +
                              std::string(directive_name) + ": " +
                              std::string(value);
        return false;
      }
    } else if (directive_name == "style-src" ||
               directive_name == "style-src-elem") {
      if (!IsAllowedStyleSrcValue(value)) {
        result.error_detail = "Disallowed value in " +
                              std::string(directive_name) + ": " +
                              std::string(value);
        return false;
      }
    } else if (directive_name == "frame-src" ||
               directive_name == "child-src") {
      if (!IsAllowedFrameSrcValue(value)) {
        result.error_detail = "Disallowed value in " +
                              std::string(directive_name) + ": " +
                              std::string(value);
        return false;
      }
    } else if (directive_name == "worker-src") {
      if (!IsAllowedWorkerSrcValue(value)) {
        result.error_detail = "Disallowed value in worker-src: " +
                              std::string(value);
        return false;
      }
    }
  }
  return true;
}

}  // namespace

CspValidationResult ValidateCsp(const std::string& csp_string) {
  CspValidationResult result;

  if (csp_string.find(',') != std::string::npos) {
    result.error_detail = "CSP must not contain commas (multiple policies)";
    return result;
  }

  auto directives = SplitCspDirectives(csp_string);
  if (directives.empty()) {
    result.error_detail = "CSP is empty";
    return result;
  }

  bool has_default_src = false;
  bool default_src_is_none = false;

  for (const auto& directive : directives) {
    auto parts = SplitDirectiveValues(directive);
    if (parts.empty()) continue;

    auto directive_name = parts[0];
    std::string_view values_str = directive;
    values_str.remove_prefix(directive_name.size());

    if (directive_name == "default-src") {
      has_default_src = true;
      if (!ValidateDirective(directive_name, values_str, result)) {
        return result;
      }
      auto values = SplitDirectiveValues(values_str);
      for (const auto& v : values) {
        if (v == "'none'") {
          default_src_is_none = true;
        }
      }
    } else if (directive_name == "object-src") {
      if (!ValidateDirective(directive_name, values_str, result)) {
        return result;
      }
    } else if (directive_name == "script-src" ||
               directive_name == "script-src-elem") {
      if (!ValidateDirective(directive_name, values_str, result)) {
        return result;
      }
    } else if (directive_name == "style-src" ||
               directive_name == "style-src-elem") {
      if (!ValidateDirective(directive_name, values_str, result)) {
        return result;
      }
    } else if (directive_name == "frame-src" ||
               directive_name == "child-src") {
      if (!ValidateDirective(directive_name, values_str, result)) {
        return result;
      }
    } else if (directive_name == "worker-src") {
      if (!ValidateDirective(directive_name, values_str, result)) {
        return result;
      }
    }
  }

  if (!has_default_src) {
    result.error_detail = "CSP must include default-src directive";
    return result;
  }

  if (!default_src_is_none) {
    bool has_object_src = false;
    for (const auto& directive : directives) {
      auto parts = SplitDirectiveValues(directive);
      if (!parts.empty() && parts[0] == "object-src") {
        has_object_src = true;
        std::string_view values_str = directive;
        values_str.remove_prefix(std::string_view("object-src").size());
        auto values = SplitDirectiveValues(values_str);
        bool has_none = false;
        for (const auto& v : values) {
          if (v == "'none'") has_none = true;
        }
        if (!has_none) {
          result.error_detail =
              "object-src must be 'none' when default-src is not 'none'";
          return result;
        }
      }
    }
    if (!has_object_src) {
      result.error_detail =
          "CSP must include object-src 'none' when default-src is not 'none'";
      return result;
    }
  }

  result.is_valid = true;
  return result;
}

CspValidationResult ValidateDefaultCsp(const std::string& csp_string) {
  return ValidateCsp(csp_string);
}

std::string GetEffectiveCspForPath(
    const std::string& path,
    const std::string& default_csp,
    const std::map<std::string, std::string>& extra_csp) {
  if (extra_csp.empty()) {
    return default_csp;
  }

  std::string effective_path = path;
  if (effective_path.ends_with("/")) {
    effective_path += "index.html";
  }

  std::string best_match;
  size_t best_match_len = 0;

  for (const auto& [prefix, csp] : extra_csp) {
    if (base::StartsWith(effective_path, prefix) &&
        prefix.size() > best_match_len) {
      best_match = csp;
      best_match_len = prefix.size();
    }
  }

  return best_match.empty() ? default_csp : best_match;
}

}  // namespace webcat
