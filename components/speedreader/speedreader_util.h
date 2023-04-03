/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SPEEDREADER_SPEEDREADER_UTIL_H_
#define BRAVE_COMPONENTS_SPEEDREADER_SPEEDREADER_UTIL_H_

#include <string>

#include "base/functional/callback_forward.h"

class GURL;
class HostContentSettingsMap;

namespace speedreader {

class SpeedreaderService;
class SpeedreaderRewriterService;

// DistillState is an enum for the current state of a speedreader WebContents
enum class DistillState {
  // Used as an initialization state
  kUnknown,

  // The web contents is not distilled
  kNone,

  // -------------------------------------------------------------------------
  // Pending states. The user requested the page be distilled.
  //
  // TODO(keur): Should we use bitstrings and make pending a bit both
  // speedreader and reader mode can share?

  // Reader mode state that can only be reached when Speedreader is disabled
  // The Speedreader icon will pop up in the address bar, and the user clicks
  // it. It runs Speedreader is "Single Shot Mode".  The Speedreader throttle
  // is created for the following request, then deactivated.
  //
  // The first time a user activates reader mode on a page, a bubble drops
  // down asking them to enable the Speedreader feature for automatic
  // distillation.
  kReaderModePending,

  // Speedreader is enabled and the page was automatically distilled.
  kSpeedreaderModePending,
  // -------------------------------------------------------------------------

  // kReaderModePending was ACKed
  kReaderMode,

  // kSpeedreaderModePending was ACKed
  kSpeedreaderMode,

  // Speedreader is enabled, but the page was blacklisted by the user.
  kSpeedreaderOnDisabledPage,

  // Speedreader is disabled, the URL passes the heuristic.
  kPageProbablyReadable,
};

enum class DistillationResult : int {
  kNone,
  kSuccess,
  kFail,
};

// Page is in reader mode or speedreader mode.
bool PageStateIsDistilled(DistillState state);

// Page can be distilled.
bool PageSupportsDistillation(DistillState state);

// Page is in reader mode, speedreader mode, or a pending state.
bool PageWantsDistill(DistillState state);

// Enable or disable Speedreader using a ContentSettingPattern derived from the
// url.
void SetEnabledForSite(HostContentSettingsMap* map,
                       const GURL& url,
                       bool enable);

// Checks content settings if Speedreader is disabled for the URL
bool IsEnabledForSite(HostContentSettingsMap* map, const GURL& url);

using DistillationResultCallback =
    base::OnceCallback<void(DistillationResult result,
                            std::string original_data,
                            std::string transformed)>;
void DistillPage(const GURL& url,
                 std::string body,
                 SpeedreaderService* speedreader_service,
                 SpeedreaderRewriterService* rewriter_service,
                 DistillationResultCallback callback);

}  // namespace speedreader

#endif  // BRAVE_COMPONENTS_SPEEDREADER_SPEEDREADER_UTIL_H_
