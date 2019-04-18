/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_stats_updater_util.h"

#include "base/strings/string_number_conversions.h"
#include "base/strings/string_piece.h"
#include "base/strings/string_split.h"
#include "base/strings/stringprintf.h"
#include "chrome/common/channel_info.h"

namespace brave {

std::string GetDateAsYMD(const base::Time& time) {
  base::Time::Exploded exploded;
  time.LocalExplode(&exploded);
  return base::StringPrintf("%d-%02d-%02d", exploded.year, exploded.month,
                            exploded.day_of_month);
}

std::string GetChannelName() {
  std::string channel = chrome::GetChannelName();
  if (channel.empty())
    channel = "release";
  return channel;
}

std::string GetPlatformIdentifier() {
#if defined(OS_WIN)
  if (base::SysInfo::OperatingSystemArchitecture() == "x86")
    return "winia32-bc";
  else
    return "winx64-bc";
#elif defined(OS_MACOSX)
  return "osx-bc";
#elif defined(OS_ANDROID)
  return "android-bc";
#elif defined(OS_LINUX)
  return "linux-bc";
#else
  return std::string();
#endif
}

int GetIsoWeekNumber(base::Time time) {
  base::Time::Exploded now_exploded;
  time.LocalExplode(&now_exploded);
  now_exploded.hour = 0;
  now_exploded.minute = 0;
  now_exploded.second = 0;
  now_exploded.millisecond = 0;
  now_exploded.day_of_month =
      now_exploded.day_of_month + 3 - ((now_exploded.day_of_week + 6) % 7);

  base::Time now_adjusted;
  if (!base::Time::FromLocalExploded(now_exploded, &now_adjusted))
    return 0;

  base::Time::Exploded jan4_exploded = {0};
  jan4_exploded.year = now_exploded.year;
  jan4_exploded.month = 1;
  jan4_exploded.day_of_week = 0;
  jan4_exploded.day_of_month = 4;
  jan4_exploded.hour = 0;
  jan4_exploded.minute = 0;
  jan4_exploded.second = 0;
  jan4_exploded.millisecond = 0;

  base::Time jan4_time;
  if (!base::Time::FromLocalExploded(jan4_exploded, &jan4_time))
    return 0;

  return 1 + std::round(
                 ((now_adjusted.ToJsTime() - jan4_time.ToJsTime()) / 86400000 -
                  3 + (jan4_exploded.day_of_month + 6) % 7) /
                 7);
}

base::Time GetYMDAsDate(base::StringPiece ymd) {
  const auto pieces = base::SplitStringPiece(ymd, "-", base::TRIM_WHITESPACE,
                                             base::SPLIT_WANT_NONEMPTY);
  DCHECK(pieces.size() == 3);
  base::Time::Exploded time;
  base::Time().LocalExplode(&time);

  bool ok = base::StringToInt(pieces[0], &time.year);
  DCHECK(ok);
  ok = base::StringToInt(pieces[1], &time.month);
  DCHECK(ok);
  ok = base::StringToInt(pieces[2], &time.day_of_month);
  DCHECK(ok);

  DCHECK(time.HasValidValues());

  base::Time result;
  ok = base::Time::FromLocalExploded(time, &result);
  DCHECK(ok);

  return result;
}


}  // namespace brave
