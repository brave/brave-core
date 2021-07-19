/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_TEST_SNAPSHOT_WIDGET_SNAPSHOT_CHECKER_H_
#define BRAVE_TEST_SNAPSHOT_WIDGET_SNAPSHOT_CHECKER_H_

#include "base/files/file_path.h"

namespace views {
class Widget;
}

class WidgetSnapshotChecker {
 public:
  WidgetSnapshotChecker();
  ~WidgetSnapshotChecker();

  void CaptureAndCheckSnapshot(views::Widget* widget);

 private:
  WidgetSnapshotChecker(const WidgetSnapshotChecker&) = delete;
  WidgetSnapshotChecker& operator=(const WidgetSnapshotChecker&) = delete;

  base::FilePath GetSnapshotPath();
  base::FilePath GetFailedSnapshotDir();
  base::FilePath GetTestRelativeDir();

  int snapshot_index_ = -1;
};

#endif  // BRAVE_TEST_SNAPSHOT_WIDGET_SNAPSHOT_CHECKER_H_
