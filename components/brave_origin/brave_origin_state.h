/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ORIGIN_BRAVE_ORIGIN_STATE_H_
#define BRAVE_COMPONENTS_BRAVE_ORIGIN_BRAVE_ORIGIN_STATE_H_

#include "base/files/file_path.h"
#include "base/values.h"

class BraveOriginState {
 public:
  BraveOriginState();
  static BraveOriginState* GetInstance();

  // Initialize the Brave Origin state by checking the user data directory.
  // Should be called once during browser startup.
  void Initialize(const base::FilePath& user_data_dir);

  // Returns true if the user is considered a Brave Origin user.
  // Must be called after Initialize().
  bool IsBraveOriginUser() const;

 private:
  ~BraveOriginState();
  bool LoadStateFromJsonFile(const base::FilePath& user_data_dir);

  bool is_brave_origin_user_;
  bool initialized_;
};

#endif  // BRAVE_COMPONENTS_BRAVE_ORIGIN_BRAVE_ORIGIN_STATE_H_
