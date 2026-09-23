/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_EMAIL_ALIASES_EMAIL_ALIASES_METRICS_H_
#define BRAVE_COMPONENTS_EMAIL_ALIASES_EMAIL_ALIASES_METRICS_H_

#include "base/memory/raw_ref.h"
#include "base/timer/wall_clock_timer.h"
#include "brave/components/email_aliases/email_aliases.mojom.h"
#include "brave/components/time_period_storage/weekly_storage.h"
#include "components/prefs/pref_change_registrar.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/receiver_set.h"

class PrefRegistrySimple;
class PrefService;

namespace email_aliases {

inline constexpr char kClipboardCopyCountHistogramName[] =
    "Brave.EmailAliases.ClipboardCopyCount";
inline constexpr char kNotesCountHistogramName[] =
    "Brave.EmailAliases.NotesCount";
inline constexpr char kSettingsPageMethodHistogramName[] =
    "Brave.EmailAliases.SettingsPageMethod";

enum class SettingsPageMethod {
  kContextMenu = 0,
  kAutofillBubble = 1,
  kAppMenu = 2,
  kManualNavigation = 3,
  kMaxValue = kManualNavigation,
};

class EmailAliasesMetrics : public mojom::EmailAliasesMetrics {
 public:
  explicit EmailAliasesMetrics(PrefService& pref_service);
  ~EmailAliasesMetrics() override;

  EmailAliasesMetrics(const EmailAliasesMetrics&) = delete;
  EmailAliasesMetrics& operator=(const EmailAliasesMetrics&) = delete;

  static void RegisterProfilePrefs(PrefRegistrySimple* registry);

  void BindInterface(
      mojo::PendingReceiver<mojom::EmailAliasesMetrics> receiver);

  // mojom::EmailAliasesMetrics:
  void OnAliasCopied() override;

  void RecordSettingsPageNavigation(SettingsPageMethod method);
  void ReportEmailAliasPresence(bool is_present);

 private:
  void ReportAllMetrics();
  void ReportCopyCount();
  void ReportNotesCount();

  const raw_ref<PrefService> pref_service_;
  WeeklyStorage clipboard_copy_storage_;
  PrefChangeRegistrar pref_change_registrar_;
  base::WallClockTimer report_timer_;
  mojo::ReceiverSet<mojom::EmailAliasesMetrics> receivers_;
};

}  // namespace email_aliases

#endif  // BRAVE_COMPONENTS_EMAIL_ALIASES_EMAIL_ALIASES_METRICS_H_
