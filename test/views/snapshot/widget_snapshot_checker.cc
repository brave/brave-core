/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/test/views/snapshot/widget_snapshot_checker.h"

#include <string>
#include <string_view>

#include "base/files/file_util.h"
#include "base/path_service.h"
#include "base/threading/thread_restrictions.h"
#include "brave/components/constants/brave_paths.h"
#include "build/build_config.h"
#include "cc/test/pixel_comparator.h"
#include "cc/test/pixel_test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/compositor/test/draw_waiter_for_test.h"
#include "ui/gfx/image/image.h"
#include "ui/views/widget/widget.h"

#if defined(USE_AURA)
#include "ui/snapshot/snapshot_aura.h"
#endif

namespace {

constexpr char kSnapshotFileName[] = "snapshot.png";

std::string_view GetPlatformName() {
#if BUILDFLAG(IS_WIN)
  return "win";
#elif BUILDFLAG(IS_MAC)
  return "mac";
#elif BUILDFLAG(IS_LINUX)
  return "linux";
#endif  // BUILDFLAG(IS_WIN)
}

bool IsSnapshotCheckingSupported() {
  // TODO(https://github.com/brave/brave-browser/issues/17024): Add snapshots
  // checking support for MacOS.
#if BUILDFLAG(IS_MAC)
  return false;
#else
  return true;
#endif  // BUILDFLAG(IS_MAC)
}

bool WriteFailedSnapshotFile(const SkBitmap& png_bitmap,
                             base::FilePath failed_snapshot_dir) {
  base::ScopedAllowBlockingForTesting allow_block;
  if (!base::CreateDirectoryAndGetError(failed_snapshot_dir,
                                        nullptr /*error*/)) {
    return false;
  }
  const base::FilePath failed_snapshot_path =
      failed_snapshot_dir.AppendASCII(kSnapshotFileName);
  return cc::WritePNGFile(png_bitmap, failed_snapshot_path,
                          /*discard_transparency=*/false);
}

void Capture(views::Widget* widget, gfx::Image* image) {
  // Wait for painting complete.
  ui::Compositor* compositor = widget->GetCompositor();
  ui::DrawWaiterForTest::WaitForCompositingEnded(compositor);

#if defined(USE_AURA)
  const gfx::Rect widget_bounds = widget->GetRootView()->bounds();
  const auto on_got_snapshot = [](base::RunLoop* run_loop, gfx::Image* image,
                                  gfx::Image got_image) {
    *image = got_image;
    run_loop->Quit();
  };
  base::RunLoop run_loop;
  ui::GrabWindowSnapshotAura(widget->GetNativeWindow(), widget_bounds,
                             base::BindOnce(on_got_snapshot, &run_loop, image));
  run_loop.Run();
#endif  // defined(USE_AURA)
}

bool CompareSnaphot(const SkBitmap& png_bitmap, base::FilePath snapshot_path) {
  base::ScopedAllowBlockingForTesting allow_blocking;
  return cc::MatchesPNGFile(
      png_bitmap, snapshot_path,
      cc::FuzzyPixelComparator().DiscardAlpha().SetErrorPixelsPercentageLimit(
          10.f));
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
  if (!IsSnapshotCheckingSupported()) {
    return;
  }

  ++snapshot_index_;

  gfx::Image snapshot;
  Capture(widget, &snapshot);
  SkBitmap png_bitmap = snapshot.AsBitmap();

  const base::FilePath snapshot_path = GetSnapshotPath();
  const bool is_equal = CompareSnaphot(png_bitmap, snapshot_path);
  base::FilePath failed_snapshot_dir = GetFailedSnapshotDir();
  if (!is_equal) {
    ASSERT_TRUE(WriteFailedSnapshotFile(png_bitmap, failed_snapshot_dir))
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
