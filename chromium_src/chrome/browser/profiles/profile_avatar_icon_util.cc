// Copyright 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "base/values.h"
#include "brave/grit/brave_theme_resources.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/profiles/profile_avatar_icon_util.h"
#include "chrome/grit/theme_resources.h"

// First, define anything that patches will rely on.
namespace profiles {

struct IconResourceInfo;

#if !defined(OS_CHROMEOS) && !defined(OS_ANDROID)
  constexpr size_t kBraveDefaultAvatarIconsCount = 14;
#else
  constexpr size_t kBraveDefaultAvatarIconsCount = 0;
#endif

const IconResourceInfo* GetBraveDefaultAvatarIconResourceInfo(
      size_t chromium_index);

size_t GetBraveAvatarIconStartIndex();

#define BRAVE_GET_DEFAULT_AVATAR_ICON_RESOURCE_INFO          \
  size_t brave_start_index =                                        \
      kDefaultAvatarIconsCount - kBraveDefaultAvatarIconsCount;     \
  if (index >= brave_start_index) {                                 \
    size_t brave_icon_index = index - brave_start_index;            \
    const IconResourceInfo* brave_icon =                                  \
        GetBraveDefaultAvatarIconResourceInfo(brave_icon_index);    \
    if (brave_icon)                                                 \
      return brave_icon;                                            \
  }

#define BRAVE_GET_MODERN_AVATAR_ICON_START_INDEX  \
  return GetBraveAvatarIconStartIndex();

}  // namespace profiles

// Override some functions (see implementations for details).
#define GetDefaultProfileAvatarIconsAndLabels \
    GetDefaultProfileAvatarIconsAndLabels_ChromiumImpl
#define IsDefaultAvatarIconUrl IsDefaultAvatarIconUrl_ChromiumImpl

#include "../../../../../chrome/browser/profiles/profile_avatar_icon_util.cc"
#undef BRAVE_GET_DEFAULT_AVATAR_ICON_RESOURCE_INFO
#undef BRAVE_GET_MODERN_AVATAR_ICON_START_INDEX
#undef GetDefaultProfileAvatarIconsAndLabels
#undef IsDefaultAvatarIconUrl

namespace profiles {

size_t GetBraveAvatarIconStartIndex() {
  return kDefaultAvatarIconsCount - kBraveDefaultAvatarIconsCount;
}

const IconResourceInfo* GetBraveDefaultAvatarIconResourceInfo(
      size_t index) {
#if defined(OS_CHROMEOS) || defined(OS_ANDROID)
  return nullptr;
#else
  CHECK_LT(index, kBraveDefaultAvatarIconsCount);
  // Keep the chromium naming style for compatibility with format compare
  // methods, such as profile::IsDefaultAvatarIconUrl.
  // The ID suffixes should be the only thing that needs to change in this file
  // if the chromium list grows in size.
  // Generated via zsh:
  // NOLINT > declare -i a=56
  // NOLINT > for file in $(cat < avatarlist); do echo "{IDR_PROFILE_AVATAR_$a, \"$file\",
  //     IDS_BRAVE_AVATAR_LABEL_$a},"; a=a+1 done | pbcopy
  static constexpr IconResourceInfo
      resource_info[kBraveDefaultAvatarIconsCount] = {
    {IDR_PROFILE_AVATAR_56, "avatar_orangepeel.png",
          IDS_BRAVE_AVATAR_LABEL_56},
    {IDR_PROFILE_AVATAR_57, "avatar_nana.png",
          IDS_BRAVE_AVATAR_LABEL_57},
    {IDR_PROFILE_AVATAR_58, "avatar_envy.png",
          IDS_BRAVE_AVATAR_LABEL_58},
    {IDR_PROFILE_AVATAR_59, "avatar_clearskies.png",
          IDS_BRAVE_AVATAR_LABEL_59},
    {IDR_PROFILE_AVATAR_60, "avatar_bondyblue.png",
          IDS_BRAVE_AVATAR_LABEL_60},
    {IDR_PROFILE_AVATAR_61, "avatar_blurple.png",
          IDS_BRAVE_AVATAR_LABEL_61},
    {IDR_PROFILE_AVATAR_62, "avatar_darknight.png",
          IDS_BRAVE_AVATAR_LABEL_62},
    {IDR_PROFILE_AVATAR_63, "avatar_orangepeel_shape.png",
          IDS_BRAVE_AVATAR_LABEL_63},
    {IDR_PROFILE_AVATAR_64, "avatar_nana_shape.png",
          IDS_BRAVE_AVATAR_LABEL_64},
    {IDR_PROFILE_AVATAR_65, "avatar_envy_shape.png",
          IDS_BRAVE_AVATAR_LABEL_65},
    {IDR_PROFILE_AVATAR_66, "avatar_clearskies_shape.png",
          IDS_BRAVE_AVATAR_LABEL_66},
    {IDR_PROFILE_AVATAR_67, "avatar_bondyblue_shape.png",
          IDS_BRAVE_AVATAR_LABEL_67},
    {IDR_PROFILE_AVATAR_68, "avatar_blurple_shape.png",
          IDS_BRAVE_AVATAR_LABEL_68},
    {IDR_PROFILE_AVATAR_69, "avatar_darknight_shape.png",
          IDS_BRAVE_AVATAR_LABEL_69},
  };
  static_assert(
      (resource_info[kBraveDefaultAvatarIconsCount - 1].resource_id ==
      IDR_PROFILE_AVATAR_69),
      "IconResourceInfo entries for Brave avatars is less than"
      "kBraveDefaultAvatarIconsCount but should be the same.");
  return &resource_info[index];
#endif
}

std::unique_ptr<base::ListValue> GetDefaultProfileAvatarIconsAndLabels(
    size_t selected_avatar_idx) {
  auto avatars = GetDefaultProfileAvatarIconsAndLabels_ChromiumImpl(
      selected_avatar_idx);
#if !defined(OS_CHROMEOS) && !defined(OS_ANDROID)
  //  Insert the 'placeholder' item, so it is still selectable
  //  in the Settings and Profile Manager WebUI.
  std::unique_ptr<base::DictionaryValue> avatar_info(
        new base::DictionaryValue());
  avatar_info->SetString("url", profiles::GetPlaceholderAvatarIconUrl());
  avatar_info->SetString(
      "label", l10n_util::GetStringUTF16(IDS_BRAVE_AVATAR_LABEL_PLACEHOLDER));
  if (selected_avatar_idx == GetPlaceholderAvatarIndex())
    avatar_info->SetBoolean("selected", true);
  avatars->Insert(0, std::move(avatar_info));
#endif
  return avatars;
}

bool IsDefaultAvatarIconUrl(const std::string& url, size_t* icon_index) {
  // Brave supports user choosing the placeholder avatar, Chromium does not.
  if (url.compare(GetPlaceholderAvatarIconUrl()) == 0) {
    *icon_index = GetPlaceholderAvatarIndex();
    return true;
  }
  return IsDefaultAvatarIconUrl_ChromiumImpl(url, icon_index);
}

}  // namespace profiles
