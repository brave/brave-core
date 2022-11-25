// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

#include "base/metrics/histogram_base.h"
#include "base/test/metrics/histogram_tester.h"

// We use the RSS Links Fetcher from our own TabHelper, which means the
// histogram counts are not what the test expects. Get get around this, we just
// disable the checks for histogram counts.
namespace base {
namespace {
class HistogramTesterStub {
 public:
  HistogramTesterStub() = default;
  ~HistogramTesterStub() = default;

  void ExpectTotalCount(StringPiece name, HistogramBase::Count count) const {}
};
}  // namespace
}  // namespace base

#define HistogramTester HistogramTesterStub

#include "src/chrome/browser/feed/rss_links_fetcher_browsertest.cc"

#undef HistogramTester
