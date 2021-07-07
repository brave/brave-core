/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/test/snapshot/widget_snapshot_checker.h"

#include <string>

#include "base/files/file_util.h"
#include "base/memory/ref_counted_memory.h"
#include "base/path_service.h"
#include "base/threading/thread_restrictions.h"
#include "brave/common/brave_paths.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/compositor/test/draw_waiter_for_test.h"
#include "ui/gfx/image/image.h"
#include "ui/snapshot/snapshot.h"
#include "ui/views/widget/widget.h"

#if defined(USE_AURA)
#include "ui/snapshot/snapshot_aura.h"
#endif

namespace {

constexpr char kSnapshotFileName[] = "snapshot.png";

base::StringPiece GetPlatformName() {
#if defined(OS_WIN)
  return "win";
#elif defined(OS_MAC)
  return "mac";
#elif defined(OS_LINUX)
  return "linux";
#endif
}

bool WriteFailedSnapshotFile(scoped_refptr<base::RefCountedMemory> data,
                             base::FilePath failed_snapshot_dir) {
  base::ScopedAllowBlockingForTesting allow_block;
  if (!base::CreateDirectoryAndGetError(failed_snapshot_dir, nullptr /*error*/))
    return false;
  auto failed_snapshot_path =
      failed_snapshot_dir.AppendASCII(kSnapshotFileName);

  if (!base::WriteFile(failed_snapshot_path, data->front_as<char>(),
                       data->size())) {
    return false;
  }

  return true;
}

void Capture(views::Widget* widget, gfx::Image* image) {
  // Wait for painting complete.
  auto* compositor = widget->GetCompositor();
  ui::DrawWaiterForTest::WaitForCompositingEnded(compositor);

  gfx::Rect widget_bounds = widget->GetRootView()->bounds();
  const auto on_got_snapshot = [](base::RunLoop* run_loop, gfx::Image* image,
                                  gfx::Image got_image) {
    *image = got_image;
    run_loop->Quit();
  };

  base::RunLoop run_loop;
#if defined(USE_AURA)
  ui::GrabWindowSnapshotAsyncAura(
#else
  ui::GrabWindowSnapshotAsync(
#endif  // defined(USE_AURA)
      widget->GetNativeWindow(), widget_bounds,
      base::BindOnce(on_got_snapshot, &run_loop, image));
  run_loop.Run();
}

bool CompareSnaphot(scoped_refptr<base::RefCountedMemory> png_bytes,
                    base::FilePath snapshot_path) {
  base::ScopedAllowBlockingForTesting allow_blocking;

  if (!png_bytes->size())
    return false;

  std::string file_contents;
  if (!base::ReadFileToString(snapshot_path, &file_contents))
    return false;

  if (file_contents.size() == 0)
    return false;

  if (png_bytes->size() != file_contents.size())
    return false;

  if (!std::equal(file_contents.begin(), file_contents.end(),
                  png_bytes->front_as<char>()))
    return false;

  return true;
}

base::FilePath GetTestDataDir() {
  base::ScopedAllowBlockingForTesting allow_block;
  base::FilePath test_data_dir;
  base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);
  return test_data_dir;
}

base::FilePath GetExecutableDataDir() {
  base::ScopedAllowBlockingForTesting allow_block;
  base::FilePath data_dir;
  base::PathService::Get(base::DIR_EXE, &data_dir);
  return data_dir;
}

}  // namespace

WidgetSnapshotChecker::WidgetSnapshotChecker() = default;
WidgetSnapshotChecker::~WidgetSnapshotChecker() = default;

void WidgetSnapshotChecker::CaptureAndCheckSnapshot(views::Widget* widget) {
  ++snapshot_index_;

  gfx::Image snapshot;
  Capture(widget, &snapshot);
  auto png_bytes = snapshot.As1xPNGBytes();

  auto snapshot_path = GetSnapshotPath();
  const bool is_equal = CompareSnaphot(png_bytes, snapshot_path);
  base::FilePath failed_snapshot_dir = GetFailedSnapshotDir();
  if (!is_equal) {
    ASSERT_TRUE(WriteFailedSnapshotFile(png_bytes, failed_snapshot_dir))
        << "Cannot write failed snapshot at: "
        << failed_snapshot_dir.AppendASCII(kSnapshotFileName)
        << "\nOriginal snapshot: " << snapshot_path.AsUTF8Unsafe();
  }

  ASSERT_TRUE(is_equal)
      << "Snapshots doesn't match.\nOriginal snapshot: "
      << snapshot_path.AsUTF8Unsafe() << "\nFailed snapshot: "
      << failed_snapshot_dir.AppendASCII(kSnapshotFileName).AsUTF8Unsafe();
}

base::FilePath WidgetSnapshotChecker::GetSnapshotPath() {
  return GetTestDataDir()
      .AppendASCII("ui")
      .AppendASCII("snapshots")
      .Append(GetTestRelativeDir())
      .AppendASCII(kSnapshotFileName);
}

base::FilePath WidgetSnapshotChecker::GetFailedSnapshotDir() {
  return GetExecutableDataDir()
      .AppendASCII("test")
      .AppendASCII("ui")
      .AppendASCII("failed_snapshots")
      .Append(GetTestRelativeDir());
}

base::FilePath WidgetSnapshotChecker::GetTestRelativeDir() {
  const ::testing::TestInfo* const test_info =
      ::testing::UnitTest::GetInstance()->current_test_info();

  return base::FilePath::FromUTF8Unsafe(
             base::ToLowerASCII(test_info->test_suite_name()))
      .AppendASCII(base::ToLowerASCII(test_info->name()) + "_" +
                   base::NumberToString(snapshot_index_))
      .AppendASCII(GetPlatformName());
}
