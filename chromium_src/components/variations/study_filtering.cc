/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/variations/study_filtering.h"

#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"

namespace variations::internal {

namespace {

class VersionStringWithMajorPartFromVersion {
 public:
  VersionStringWithMajorPartFromVersion(std::string_view version_string,
                                        const base::Version& version)
      : version_string_(version_string) {
    if (!version.IsValid()) {
      return;
    }

    auto splitted_version_string = base::SplitStringPiece(
        version_string, ".", base::WhitespaceHandling::KEEP_WHITESPACE,
        base::SplitResult::SPLIT_WANT_ALL);

    // If the version filter is empty, "MAJOR" or "MAJOR.*", we compare it as
    // is.
    if (splitted_version_string.size() < 2 ||
        (splitted_version_string.size() == 2 &&
         splitted_version_string[1] == "*")) {
      return;
    }

    // Otherwise set MAJOR in the filter to the MAJOR of the current version.
    // This effectively skips the MAJOR part during version comparison, allowing
    // to compare the version parts after the MAJOR (ex. 1.50.10) no matter what
    // the MAJOR version is (ex. 130.1.50.10 or 131.1.50.10).
    const std::string major_version_string =
        base::NumberToString(version.components()[0]);
    if (major_version_string == splitted_version_string[0]) {
      return;
    }

    splitted_version_string[0] = major_version_string;
    version_string_with_replaced_major_ =
        base::JoinString(splitted_version_string, ".");
  }

  std::string_view version_string() const {
    return version_string_with_replaced_major_.empty()
               ? version_string_
               : version_string_with_replaced_major_;
  }

 private:
  const std::string_view version_string_;
  std::string version_string_with_replaced_major_;
};

}  // namespace

}  // namespace variations::internal

#define min_version()                                                        \
  min_version().empty()                                                      \
      ? filter.min_version()                                                 \
      : VersionStringWithMajorPartFromVersion(filter.min_version(), version) \
            .version_string()

#define max_version()                                                        \
  max_version().empty()                                                      \
      ? filter.max_version()                                                 \
      : VersionStringWithMajorPartFromVersion(filter.max_version(), version) \
            .version_string()

#include "src/components/variations/study_filtering.cc"

#undef min_version
#undef max_version
