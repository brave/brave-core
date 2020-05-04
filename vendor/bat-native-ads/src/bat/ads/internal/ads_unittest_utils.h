/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_ADS_UNITTEST_UTILS_H_
#define BAT_ADS_INTERNAL_ADS_UNITTEST_UTILS_H_

#include <deque>
#include <string>

#include "bat/ads/result.h"

#include "base/files/file_path.h"
#include "brave/components/l10n/browser/locale_helper_mock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ads {

class AdConversions;
class AdsClientMock;
class AdsImpl;
class Client;

template<class T>
void Initialize(
    T object) {
  object->Initialize(
      [](const Result result) {
    ASSERT_EQ(Result::SUCCESS, result);
  });
}

base::FilePath GetTestPath();

base::FilePath GetResourcesPath();

void MockLoad(
    AdsClientMock* mock);

void MockSave(
    AdsClientMock* mock);

void MockLoadUserModelForLanguage(
    AdsClientMock* mock);

void MockLoadJsonSchema(
    AdsClientMock* mock);

template <class T>
bool CompareDequesAsSets(
    const std::deque<T>& vec1,
    const std::deque<T>& vec2) {
  if (vec1.size() != vec2.size()) {
    return false;
  }

  for (size_t i = 0; i < vec1.size(); i++) {
    bool found = false;

    for (size_t j = 0; (j < vec2.size()) && !found; j++) {
      found = found || (vec1[i] == vec2[j]);
    }

    if (!found) {
      return false;
    }
  }

  return true;
}

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_ADS_UNITTEST_UTILS_H_
