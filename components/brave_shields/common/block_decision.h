/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_COMMON_BLOCK_DECISION_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_COMMON_BLOCK_DECISION_H_

#include <string>

#include "brave/components/brave_shields/common/brave_shield_constants.h"

namespace brave_shields {

class AdBlockDecision;
class TrackerBlockDecision;
class HTTPUpgradableResourceBlockDecision;

class BlockDecision {
 public:
  virtual ~BlockDecision();

  virtual const char* BlockType() const = 0;

  virtual bool IsAdBlockDecision() const;
  virtual bool IsTrackerBlockDecision() const;
  virtual bool IsHTTPUpgradableResourceBlockDecision() const;

  AdBlockDecision* AsAdBlockDecision() {
    return IsAdBlockDecision() ? reinterpret_cast<AdBlockDecision*>(this)
                               : nullptr;
  }
  const AdBlockDecision* AsAdBlockDecision() const {
    return IsAdBlockDecision() ? reinterpret_cast<const AdBlockDecision*>(this)
                               : nullptr;
  }

  TrackerBlockDecision* AsTrackerBlockDecision() {
    return IsTrackerBlockDecision()
               ? reinterpret_cast<TrackerBlockDecision*>(this)
               : nullptr;
  }
  const TrackerBlockDecision* AsTrackerBlockDecision() const {
    return IsTrackerBlockDecision()
               ? reinterpret_cast<const TrackerBlockDecision*>(this)
               : nullptr;
  }

  HTTPUpgradableResourceBlockDecision* AsHTTPUpgradableResourceBlockDecision() {
    return IsHTTPUpgradableResourceBlockDecision()
               ? reinterpret_cast<HTTPUpgradableResourceBlockDecision*>(this)
               : nullptr;
  }
  const HTTPUpgradableResourceBlockDecision*
  AsHTTPUpgradableResourceBlockDecision() const {
    return IsHTTPUpgradableResourceBlockDecision()
               ? reinterpret_cast<const HTTPUpgradableResourceBlockDecision*>(
                     this)
               : nullptr;
  }
};

class AdBlockDecision : public BlockDecision {
 public:
  explicit AdBlockDecision(const std::string& rule);
  ~AdBlockDecision() override;

  const std::string& Rule() const { return rule_; }

  const char* BlockType() const override;

  bool IsAdBlockDecision() const override;

 private:
  const std::string rule_;
};

class TrackerBlockDecision : public BlockDecision {
 public:
  explicit TrackerBlockDecision(const std::string& host);
  ~TrackerBlockDecision() override;

  const std::string& Host() const { return host_; }

  const char* BlockType() const override;

  bool IsTrackerBlockDecision() const override;

 private:
  const std::string host_;
};

class HTTPUpgradableResourceBlockDecision : public BlockDecision {
 public:
  HTTPUpgradableResourceBlockDecision();
  ~HTTPUpgradableResourceBlockDecision() override;

  const char* BlockType() const override;

  bool IsHTTPUpgradableResourceBlockDecision() const override;
};

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_COMMON_BLOCK_DECISION_H_
