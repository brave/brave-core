/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_NTP_BACKGROUND_NTP_BACKGROUND_PREFS_H_
#define BRAVE_BROWSER_NTP_BACKGROUND_NTP_BACKGROUND_PREFS_H_

#include <string>
#include <variant>
#include <vector>

#include "base/values.h"
#include "url/gurl.h"

namespace user_prefs {
class PrefRegistrySyncable;
}  // namespace user_prefs

class PrefService;

// NTPBackgroundPrefs gives easy access to values for NTP background from prefs.
// The data is stored in following format:
//
//  kNTPBackground: {
//     type: ‘brave’ | ‘custom_image’ | 'color' | ‘solid_color’
//     random: bool  // indicates that we should pick one every time
//                      from |selected_type| collection
//     selected_value?: string  // url or css value
// }
//
//  Types:
//   'brave': mapped to Type::kBrave. Default backgrounds provided by us.
//            |selected_value| is empty.
//
//   'custom_image': mapped to |Type::kCustomImage.| custom image uploaded by
//                   user. |selected_value| is name of image file.
//
//   'color' | 'solid_color': mapped to |Type::kColor|. |selected_value| is css
//                            value or either one of 'gradient' or ' solid' in
//                            case |ShouldUseRandomValue()| is true. Value name
//                            is kept as 'solid_color' for backward
//                            compatibility.
//
class NTPBackgroundPrefs final {
 public:
  static constexpr char kDeprecatedPrefName[] =
      "brave.new_tab_page.custom_background_enabled";
  static constexpr char kPrefName[] = "brave.new_tab_page.background";

  static constexpr char kCustomImageListPrefName[] =
      "brave.new_tab_page.custom_background_image_list";

  enum class Type {
    kBrave,  // Images that we supply.
    kCustomImage,
    kColor,
  };

  explicit NTPBackgroundPrefs(PrefService* service);
  NTPBackgroundPrefs(const NTPBackgroundPrefs& pref) = delete;
  NTPBackgroundPrefs& operator=(const NTPBackgroundPrefs& pref) = delete;
  ~NTPBackgroundPrefs();

  static void RegisterPref(user_prefs::PrefRegistrySyncable* registry);

  // Try to migrate the old pref for custom background into this new pref.
  void MigrateOldPref();

  // Types
  Type GetType() const;
  void SetType(Type type);
  bool IsBraveType() const;
  bool IsCustomImageType() const;
  bool IsColorType() const;

  // Returns true when we should pick one item of selected type every time NTP
  // opens.
  bool ShouldUseRandomValue() const;
  void SetShouldUseRandomValue(bool random);

  void SetSelectedValue(const std::string& value);

  // Return a value to use as NTP background.
  std::string GetSelectedValue() const;

  void AddCustomImageToList(const std::string& file_name);
  void RemoveCustomImageFromList(const std::string& file_name);
  std::vector<std::string> GetCustomImageList() const;

 private:
  const base::Value::Dict* GetPrefValue() const;

  raw_ptr<PrefService> service_ = nullptr;
};

#endif  // BRAVE_BROWSER_NTP_BACKGROUND_NTP_BACKGROUND_PREFS_H_
