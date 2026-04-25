/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_TEST_VIEWS_SNAPSHOT_WIDGET_SNAPSHOT_CHECKER_H_
#define BRAVE_TEST_VIEWS_SNAPSHOT_WIDGET_SNAPSHOT_CHECKER_H_

#include "base/files/file_path.h"

namespace views {
class Widget;
}

// Snapshot tests check visual appearance of widget on different platforms.
// Tests are performed by comparison of widget snapshot and image file from
// repo. Original snapshots are stored in this directory:
// "brave\test\data\ui\snapshots\".
// If actual snapshot doesn't match with original one, then actual snapshot is
// stored in output directory at path: "test\ui\failed_snapshots\".
// Note that linux snapshot tests should be executed using xvfb-run script to
// make the same snapshots with infra build machines.
class WidgetSnapshotChecker final {
 public:
  WidgetSnapshotChecker();
  WidgetSnapshotChecker(const WidgetSnapshotChecker&) = delete;
  WidgetSnapshotChecker& operator=(const WidgetSnapshotChecker&) = delete;
  ~WidgetSnapshotChecker();

  void CaptureAndCheckSnapshot(views::Widget* widget);

 private:
  base::FilePath GetSnapshotPath();
  base::FilePath GetFailedSnapshotDir();
  base::FilePath GetTestRelativeDir();

  int snapshot_index_ = -1;
};

#endif  // BRAVE_TEST_VIEWS_SNAPSHOT_WIDGET_SNAPSHOT_CHECKER_H_
