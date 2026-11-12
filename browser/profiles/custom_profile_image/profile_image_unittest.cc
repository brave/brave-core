// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/profiles/custom_profile_image/profile_image.h"

#include <memory>
#include <optional>
#include <utility>

#include "base/files/file_util.h"
#include "base/run_loop.h"
#include "base/scoped_observation.h"
#include "base/task/sequenced_task_runner.h"
#include "base/task/thread_pool/thread_pool_instance.h"
#include "base/test/bind.h"
#include "base/test/run_until.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/test_future.h"
#include "brave/browser/profiles/custom_profile_image/image_file.h"
#include "brave/browser/ui/webui/custom_profile_image/features.h"
#include "chrome/browser/profiles/profile_attributes_entry.h"
#include "chrome/browser/profiles/profile_attributes_init_params.h"
#include "chrome/browser/profiles/profile_attributes_storage.h"
#include "chrome/browser/profiles/profile_avatar_icon_util.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "chrome/test/base/testing_profile_manager.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/skia/include/core/SkImageInfo.h"

namespace custom_profile_image {
namespace {

class AvatarChangeObserver : public ProfileAttributesStorage::Observer {
 public:
  AvatarChangeObserver(ProfileAttributesStorage& storage,
                       base::RepeatingClosure changed)
      : changed_(std::move(changed)), observation_(this) {
    observation_.Observe(&storage);
  }
  void OnProfileAvatarChanged(const base::FilePath&) override {
    changed_.Run();
  }

 private:
  base::RepeatingClosure changed_;
  base::ScopedObservation<ProfileAttributesStorage,
                          ProfileAttributesStorage::Observer>
      observation_;
};

class CustomProfileImageTest : public testing::Test {
 protected:
  CustomProfileImageTest() : manager_(TestingBrowserProcess::GetGlobal()) {
    features_.InitAndEnableFeature(features::kBraveCustomProfileImage);
  }
  void SetUp() override {
    ASSERT_TRUE(manager_.SetUp());
    auto* profile = manager_.CreateTestingProfile("image-test");
    path_ = profile->GetPath();
    ASSERT_TRUE(base::CreateDirectory(path_));
    entry().SetAvatarIconIndex(26);
  }
  ProfileAttributesEntry& entry() {
    return *manager_.profile_manager()
                ->GetProfileAttributesStorage()
                .GetProfileAttributesWithPath(path_);
  }
  ProfileImage& image() { return ProfileImage::Get(entry()); }
  base::FilePath ImagePath() const { return path_.AppendASCII(kImageFileName); }
  base::FilePath PendingPath() const {
    return path_.AppendASCII(kPendingImageFileName);
  }
  // A second storage over the same Local State, as after a restart.
  std::unique_ptr<ProfileAttributesStorage> Reload() {
    return std::make_unique<ProfileAttributesStorage>(
        TestingBrowserProcess::GetGlobal()->local_state(),
        manager_.profile_manager()->user_data_dir());
  }
  size_t CountSelectedAvatars() {
    size_t count = 0;
    for (const auto& avatar :
         profiles::GetIconsAndLabelsForProfileAvatarSelector(path_)) {
      const std::optional<bool> selected =
          avatar.GetDict().FindBool("selected");
      EXPECT_TRUE(selected.has_value());
      count += selected.value_or(false);
    }
    return count;
  }
  void SelectPreset(size_t index) {
    profiles::SetDefaultProfileAvatarIndex(
        manager_.profile_manager()->GetProfileByPath(path_), index);
  }
  SkBitmap Bitmap(SkColor color) {
    SkBitmap bitmap;
    bitmap.allocN32Pixels(16, 16);
    bitmap.eraseColor(color);
    return bitmap;
  }
  void Save(SkColor color) {
    base::test::TestFuture<Result> result;
    image().Replace(Bitmap(color), result.GetCallback());
    ASSERT_EQ(result.Get(), Result::kSuccess);
  }
  base::test::ScopedFeatureList features_;
  content::BrowserTaskEnvironment task_environment_;
  TestingProfileManager manager_;
  base::FilePath path_;
};

TEST_F(CustomProfileImageTest, SaveResolvesThroughEntry) {
  Save(SK_ColorRED);
  EXPECT_EQ(entry().GetAvatarIcon().AsBitmap().getColor(8, 8), SK_ColorRED);
}

TEST_F(CustomProfileImageTest, EarlyResultsAreAsynchronous) {
  auto expect_async = [](base::OnceCallback<void(ProfileImage::Callback)> start,
                         Result expected) {
    bool replied = false;
    base::test::TestFuture<Result> result;
    std::move(start).Run(base::BindLambdaForTesting([&](Result value) {
      replied = true;
      result.SetValue(value);
    }));
    EXPECT_FALSE(replied);
    EXPECT_EQ(result.Get(), expected);
  };
  expect_async(base::BindLambdaForTesting([&](ProfileImage::Callback callback) {
                 image().Replace(SkBitmap(), std::move(callback));
               }),
               Result::kInvalidImage);
  expect_async(base::BindLambdaForTesting([&](ProfileImage::Callback callback) {
                 image().Select(std::move(callback));
               }),
               Result::kUnavailable);
  base::test::ScopedFeatureList disabled;
  disabled.InitAndDisableFeature(features::kBraveCustomProfileImage);
  expect_async(base::BindLambdaForTesting([&](ProfileImage::Callback callback) {
                 image().Remove(std::move(callback));
               }),
               Result::kUnavailable);
}

TEST_F(CustomProfileImageTest, UnfinishedFirstSaveIsDeletedOnLoad) {
  // A crash after the file was written but before Local State recorded it.
  ASSERT_TRUE(base::WriteFile(ImagePath(), "unsaved"));
  ASSERT_TRUE(base::WriteFile(PendingPath(), "unsaved"));
  auto reloaded = Reload();
  auto* reopened = reloaded->GetProfileAttributesWithPath(path_);
  ASSERT_TRUE(reopened);
  EXPECT_FALSE(ProfileImage::Get(*reopened).HasSavedImage());
  // Nothing reports the cleanup, so wait for the file sequence to drain.
  base::ThreadPoolInstance::Get()->FlushForTesting();
  EXPECT_FALSE(base::PathExists(ImagePath()));
  EXPECT_FALSE(base::PathExists(PendingPath()));
}

TEST_F(CustomProfileImageTest, UnfinishedFirstSaveIsDeletedWithEntry) {
  bool replied = false;
  {
    auto reloaded = Reload();
    auto* reopened = reloaded->GetProfileAttributesWithPath(path_);
    ASSERT_TRUE(reopened);
    ProfileImage::Get(*reopened).Replace(
        Bitmap(SK_ColorRED),
        base::BindLambdaForTesting([&](Result) { replied = true; }));
  }
  // The write finishes after its entry is gone, so it is never recorded.
  base::ThreadPoolInstance::Get()->FlushForTesting();
  EXPECT_FALSE(base::PathExists(ImagePath()));
  EXPECT_FALSE(replied);
  EXPECT_FALSE(image().HasSavedImage());
}

TEST_F(CustomProfileImageTest, UnusableBitmapIsRejected) {
  SkBitmap non_square;
  non_square.allocN32Pixels(16, 8);
  SkBitmap oversized;
  oversized.allocN32Pixels(kMaxImageSide + 1, kMaxImageSide + 1);
  for (const SkBitmap& bitmap : {SkBitmap(), non_square, oversized}) {
    base::test::TestFuture<Result> result;
    image().Replace(bitmap, result.GetCallback());
    EXPECT_EQ(result.Get(), Result::kInvalidImage);
  }
  EXPECT_FALSE(image().HasSavedImage());
}

TEST_F(CustomProfileImageTest, OtherColorTypesAreSavedAsN32) {
  SkBitmap wide;
  wide.allocPixels(
      SkImageInfo::Make(16, 16, kRGBA_F16_SkColorType, kPremul_SkAlphaType));
  wide.eraseColor(SK_ColorRED);
  base::test::TestFuture<Result> result;
  image().Replace(wide, result.GetCallback());
  ASSERT_EQ(result.Get(), Result::kSuccess);
  const SkBitmap saved = image().GetImage().AsBitmap();
  EXPECT_EQ(saved.colorType(), kN32_SkColorType);
  EXPECT_EQ(saved.getColor(8, 8), SK_ColorRED);
  auto reloaded = Reload();
  auto* reopened = reloaded->GetProfileAttributesWithPath(path_);
  ASSERT_TRUE(reopened);
  auto& restored = ProfileImage::Get(*reopened);
  ASSERT_TRUE(
      base::test::RunUntil([&] { return !restored.GetImage().IsEmpty(); }));
  EXPECT_EQ(restored.GetImage().AsBitmap().getColor(8, 8), SK_ColorRED);
}

TEST_F(CustomProfileImageTest, SamePresetDeactivatesWithoutDeletingSavedImage) {
  Save(SK_ColorRED);
  SelectPreset(entry().GetAvatarIconIndex());
  EXPECT_FALSE(image().IsActive());
  EXPECT_TRUE(image().HasSavedImage());
  base::test::TestFuture<Result> result;
  image().Select(result.GetCallback());
  EXPECT_EQ(result.Get(), Result::kSuccess);
  EXPECT_TRUE(image().IsActive());
}

TEST_F(CustomProfileImageTest, PresetChoiceWinsOverPendingWrite) {
  base::test::TestFuture<Result> result;
  image().Replace(Bitmap(SK_ColorRED), result.GetCallback());
  SelectPreset(entry().GetAvatarIconIndex());
  EXPECT_EQ(result.Get(), Result::kSuccess);
  EXPECT_FALSE(image().IsActive());
  EXPECT_TRUE(image().HasSavedImage());
}

TEST_F(CustomProfileImageTest, FailedWriteDoesNotCancelPendingSelection) {
  Save(SK_ColorRED);
  SelectPreset(entry().GetAvatarIconIndex());
  const auto pending = PendingPath();
  ASSERT_TRUE(base::CreateDirectory(pending));
  ASSERT_TRUE(base::WriteFile(pending.AppendASCII("keep"), "existing"));
  base::test::TestFuture<Result> selected;
  base::test::TestFuture<Result> replaced;
  image().Select(selected.GetCallback());
  image().Replace(Bitmap(SK_ColorBLUE), replaced.GetCallback());
  EXPECT_EQ(selected.Get(), Result::kSuccess);
  EXPECT_EQ(replaced.Get(), Result::kFileError);
  EXPECT_TRUE(image().IsActive());
  EXPECT_EQ(image().GetImage().AsBitmap().getColor(8, 8), SK_ColorRED);
}

TEST_F(CustomProfileImageTest, RemovalRunsAfterPendingWrite) {
  base::test::TestFuture<Result> saved;
  base::test::TestFuture<Result> removed;
  image().Replace(Bitmap(SK_ColorRED), saved.GetCallback());
  image().Remove(removed.GetCallback());
  EXPECT_EQ(saved.Get(), Result::kSuccess);
  EXPECT_EQ(removed.Get(), Result::kSuccess);
  EXPECT_FALSE(image().HasSavedImage());
  EXPECT_FALSE(base::PathExists(ImagePath()));
}

TEST_F(CustomProfileImageTest, NewProfileAtSamePathHasNoImage) {
  Save(SK_ColorRED);
  // A write still pending at removal must not reach the new profile.
  bool replied = false;
  image().Replace(Bitmap(SK_ColorBLUE),
                  base::BindLambdaForTesting([&](Result) { replied = true; }));
  auto& storage = manager_.profile_manager()->GetProfileAttributesStorage();
  storage.RemoveProfile(path_);
  ProfileAttributesInitParams params;
  params.profile_path = path_;
  params.profile_name = u"Reused";
  storage.AddProfile(std::move(params));
  base::ThreadPoolInstance::Get()->FlushForTesting();
  // Any reply was posted before this, so it would run first.
  base::RunLoop run_loop;
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, run_loop.QuitClosure());
  run_loop.Run();
  EXPECT_FALSE(replied);
  EXPECT_FALSE(image().HasSavedImage());
  EXPECT_FALSE(image().IsActive());
  EXPECT_TRUE(image().GetImage().IsEmpty());
  EXPECT_FALSE(base::PathExists(ImagePath()));
  EXPECT_FALSE(base::PathExists(PendingPath()));
}

TEST_F(CustomProfileImageTest, ReloadShowsPresetUntilSavedImageLoads) {
  Save(SK_ColorRED);
  auto reloaded = Reload();
  auto* reopened = reloaded->GetProfileAttributesWithPath(path_);
  ASSERT_TRUE(reopened);
  bool notified_with_image = false;
  AvatarChangeObserver observer(*reloaded, base::BindLambdaForTesting([&] {
    notified_with_image |=
        reopened->GetAvatarIcon().AsBitmap().getColor(8, 8) == SK_ColorRED;
  }));
  auto& saved = ProfileImage::Get(*reopened);
  EXPECT_TRUE(saved.GetImage().IsEmpty());
  EXPECT_FALSE(reopened->GetAvatarIcon().IsEmpty());
  // The picker must not mark the stored preset as selected meanwhile.
  EXPECT_TRUE(saved.IsSelected());
  ASSERT_TRUE(base::test::RunUntil([&] { return notified_with_image; }));
}

TEST_F(CustomProfileImageTest, FailedLoadKeepsSavedImageForRetry) {
  Save(SK_ColorRED);
  const auto file = ImagePath();
  const auto aside = path_.AppendASCII("aside.png");
  ASSERT_TRUE(base::Move(file, aside));
  {
    auto reloaded = Reload();
    auto* failed_entry = reloaded->GetProfileAttributesWithPath(path_);
    ASSERT_TRUE(failed_entry);
    auto& failed = ProfileImage::Get(*failed_entry);
    EXPECT_TRUE(failed.GetImage().IsEmpty());
    ASSERT_TRUE(base::test::RunUntil([&] { return !failed.IsSelected(); }));
    EXPECT_TRUE(failed.HasSavedImage());
    // An explicit selection retries the read.
    ASSERT_TRUE(base::CopyFile(aside, file));
    base::test::TestFuture<Result> selected;
    failed.Select(selected.GetCallback());
    EXPECT_EQ(selected.Get(), Result::kSuccess);
    EXPECT_TRUE(failed.IsSelected());
    EXPECT_EQ(failed.GetImage().AsBitmap().getColor(8, 8), SK_ColorRED);
  }
  // A later launch also retries.
  auto relaunched = Reload();
  auto* restored_entry = relaunched->GetProfileAttributesWithPath(path_);
  ASSERT_TRUE(restored_entry);
  auto& restored = ProfileImage::Get(*restored_entry);
  ASSERT_TRUE(
      base::test::RunUntil([&] { return !restored.GetImage().IsEmpty(); }));
  EXPECT_TRUE(restored.IsSelected());
  EXPECT_EQ(restored.GetImage().AsBitmap().getColor(8, 8), SK_ColorRED);
}

TEST_F(CustomProfileImageTest, FailedRemovalPreservesCachedSelection) {
  Save(SK_ColorRED);
  const auto destination = ImagePath();
  ASSERT_TRUE(base::DeleteFile(destination));
  ASSERT_TRUE(base::CreateDirectory(destination));
  ASSERT_TRUE(base::WriteFile(destination.AppendASCII("keep"), "existing"));
  base::test::TestFuture<Result> removed;
  image().Remove(removed.GetCallback());
  EXPECT_EQ(removed.Get(), Result::kFileError);
  EXPECT_TRUE(image().IsActive());
  EXPECT_EQ(image().GetImage().AsBitmap().getColor(8, 8), SK_ColorRED);
}

TEST_F(CustomProfileImageTest, CustomSelectionClearsPresetSelectionFlags) {
  Save(SK_ColorRED);
  EXPECT_EQ(CountSelectedAvatars(), 0u);
  SelectPreset(entry().GetAvatarIconIndex());
  EXPECT_EQ(CountSelectedAvatars(), 1u);
}

TEST_F(CustomProfileImageTest, SelectionNotificationContainsFinalState) {
  Save(SK_ColorRED);
  size_t notifications = 0;
  const size_t new_index = profiles::GetDefaultAvatarIconCount() - 1;
  AvatarChangeObserver observer(
      manager_.profile_manager()->GetProfileAttributesStorage(),
      base::BindLambdaForTesting([&] {
        ++notifications;
        if (image().IsActive()) {
          EXPECT_EQ(entry().GetAvatarIcon().AsBitmap().getColor(8, 8),
                    SK_ColorRED);
        } else {
          EXPECT_EQ(entry().GetAvatarIconIndex(), new_index);
        }
      }));
  SelectPreset(new_index);
  EXPECT_GT(notifications, 0u);
  EXPECT_FALSE(image().IsActive());
}

TEST_F(CustomProfileImageTest, MirroringFallbackDoesNotChangeCustomSelection) {
  Save(SK_ColorRED);
  entry().SetAvatarIconIndex(profiles::GetDefaultAvatarIconCount() - 1);
  entry().SetIsUsingGAIAPicture(true);
  EXPECT_TRUE(image().IsActive());
  EXPECT_EQ(entry().GetAvatarIcon().AsBitmap().getColor(8, 8), SK_ColorRED);
}

TEST_F(CustomProfileImageTest, DisabledFeatureRetainsSavedSelection) {
  Save(SK_ColorRED);
  {
    base::test::ScopedFeatureList disabled;
    disabled.InitAndDisableFeature(features::kBraveCustomProfileImage);
    EXPECT_FALSE(image().IsActive());
    EXPECT_TRUE(image().HasSavedImage());
    base::test::TestFuture<Result> result;
    image().Remove(result.GetCallback());
    EXPECT_EQ(result.Get(), Result::kUnavailable);
  }
  EXPECT_TRUE(image().IsActive());
}

TEST_F(CustomProfileImageTest, PresetChosenWhileDisabledSurvivesReenable) {
  Save(SK_ColorRED);
  {
    base::test::ScopedFeatureList disabled;
    disabled.InitAndDisableFeature(features::kBraveCustomProfileImage);
    SelectPreset(entry().GetAvatarIconIndex());
  }
  EXPECT_FALSE(image().IsActive());
  EXPECT_TRUE(image().HasSavedImage());
}

TEST_F(CustomProfileImageTest, MissingImageFallsBackAndCanBeReplaced) {
  Save(SK_ColorRED);
  ASSERT_TRUE(base::DeleteFile(ImagePath()));
  base::test::TestFuture<Result> selected;
  image().Select(selected.GetCallback());
  EXPECT_EQ(selected.Get(), Result::kFileError);
  EXPECT_TRUE(image().GetImage().IsEmpty());
  EXPECT_FALSE(entry().GetAvatarIcon().IsEmpty());
  EXPECT_EQ(CountSelectedAvatars(), 1u);
  base::test::TestFuture<Result> replaced;
  image().Replace(Bitmap(SK_ColorBLUE), replaced.GetCallback());
  ASSERT_EQ(replaced.Get(), Result::kSuccess);
  EXPECT_EQ(entry().GetAvatarIcon().AsBitmap().getColor(8, 8), SK_ColorBLUE);
}

}  // namespace
}  // namespace custom_profile_image
