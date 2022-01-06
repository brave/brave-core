/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/contributions/contribution_data.h"

#include <string>

#include "bat/ledger/internal/core/value_converters.h"

namespace ledger {

std::string StringifyEnum(ContributionType value) {
  switch (value) {
    case ContributionType::kOneTime:
      return "one-time";
    case ContributionType::kRecurring:
      return "recurring";
    case ContributionType::kAutoContribute:
      return "auto-contribute";
  }
}

absl::optional<ContributionType> ParseEnum(
    const EnumString<ContributionType>& s) {
  return s.Match({ContributionType::kOneTime, ContributionType::kRecurring,
                  ContributionType::kAutoContribute});
}

std::string StringifyEnum(ContributionSource value) {
  switch (value) {
    case ContributionSource::kBraveVG:
      return "brave-vg";
    case ContributionSource::kBraveSKU:
      return "brave-sku";
    case ContributionSource::kExternal:
      return "external";
  }
}

absl::optional<ContributionSource> ParseEnum(
    const EnumString<ContributionSource>& s) {
  return s.Match({ContributionSource::kBraveVG, ContributionSource::kBraveSKU,
                  ContributionSource::kExternal});
}

base::Value PublisherActivity::ToValue() const {
  ValueWriter w;
  w.Write("publisher_id", publisher_id);
  w.Write("visits", visits);
  w.Write("duration", duration);
  return w.Finish();
}

absl::optional<PublisherActivity> PublisherActivity::FromValue(
    const base::Value& value) {
  StructValueReader<PublisherActivity> r(value);
  r.Read("publisher_id", &PublisherActivity::publisher_id);
  r.Read("visits", &PublisherActivity::visits);
  r.Read("duration", &PublisherActivity::duration);
  return r.Finish();
}

base::Value RecurringContribution::ToValue() const {
  ValueWriter w;
  w.Write("publisher_id", publisher_id);
  w.Write("amount", amount);
  return w.Finish();
}

absl::optional<RecurringContribution> RecurringContribution::FromValue(
    const base::Value& value) {
  StructValueReader<RecurringContribution> r(value);
  r.Read("publisher_id", &RecurringContribution::publisher_id);
  r.Read("amount", &RecurringContribution::amount);
  return r.Finish();
}

base::Value Contribution::ToValue() const {
  ValueWriter w;
  w.Write("type", type);
  w.Write("publisher_id", publisher_id);
  w.Write("amount", amount);
  w.Write("source", source);
  return w.Finish();
}

}  // namespace ledger
