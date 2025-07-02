/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_origin/brave_origin_state.h"

#include "base/files/file_util.h"
#include "base/json/json_reader.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/no_destructor.h"
#include "base/strings/string_util.h"
#include "base/threading/thread_restrictions.h"

namespace {

constexpr char kBraveOriginStateFilename[] = "brave_origin_state.json";
constexpr char kBraveOriginKey[] = "is_brave_origin_user";

}  // namespace

BraveOriginState::BraveOriginState()
    : is_brave_origin_user_(false), initialized_(false) {}

BraveOriginState::~BraveOriginState() = default;

BraveOriginState* BraveOriginState::GetInstance() {
  static base::NoDestructor<BraveOriginState> instance;
  return instance.get();
}

void BraveOriginState::Initialize(const base::FilePath& user_data_dir) {
  if (initialized_ || user_data_dir.empty()) {
    return;
  }

  // Load the state from file system once
  if (LoadStateFromJsonFile(user_data_dir)) {
    initialized_ = true;
  } else {
    // If we can't load from file, default to false
    is_brave_origin_user_ = false;
    initialized_ = true;
  }
}

bool BraveOriginState::IsBraveOriginUser() const {
  if (!initialized_) {
    return false;
  }
  return is_brave_origin_user_;
}

bool BraveOriginState::LoadStateFromJsonFile(
    const base::FilePath& user_data_dir) {
  if (user_data_dir.empty()) {
    return false;
  }

  base::ScopedAllowBlockingForTesting allow_blocking;

  // Stored for example here:
  // ~/.config/BraveSoftware/Brave-Browser-Development/brave_origin_state.json
  // ~/Library/Application\ Support/BraveSoftware/Brave-Browser-Development/brave_origin_state.json
  // With a simple JSON file to start:
  // {
  //   "is_brave_origin_user": true
  // }
  base::FilePath file_path = user_data_dir.Append(kBraveOriginStateFilename);
  std::string content;
  if (!base::ReadFileToString(file_path, &content)) {
    return false;
  }

  auto parsed = base::JSONReader::Read(content);
  if (!parsed || !parsed->is_dict()) {
    return false;
  }

  const base::Value::Dict& dict = parsed->GetDict();
  is_brave_origin_user_ = dict.FindBool(kBraveOriginKey).value_or(false);

  return true;
}
