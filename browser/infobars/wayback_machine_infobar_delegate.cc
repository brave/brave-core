// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/infobars/wayback_machine_infobar_delegate.h"

#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/i18n/message_formatter.h"
#include "base/i18n/unicodestring.h"
#include "base/location.h"
#include "base/memory/ptr_util.h"
#include "base/memory/weak_ptr.h"
#include "base/task/sequenced_task_runner.h"
#include "base/time/time.h"
#include "brave/browser/infobars/wayback_machine_infobar_creator.h"
#include "brave/components/brave_wayback_machine/brave_wayback_machine_tab_helper.h"
#include "brave/grit/brave_generated_resources.h"
#include "components/infobars/content/content_infobar_manager.h"
#include "components/infobars/core/infobar.h"
#include "content/public/browser/page_navigator.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/referrer.h"
#include "third_party/icu/source/i18n/unicode/datefmt.h"
#include "third_party/icu/source/i18n/unicode/timezone.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/l10n/time_format.h"
#include "ui/base/page_transition_types.h"
#include "ui/base/window_open_disposition.h"
#include "url/gurl.h"

namespace {

constexpr char kLearnMoreURL[] = "https://support.brave.app/hc/en-us";

// Formats |time| as a date in UTC, the time zone of Wayback Machine timestamps.
std::u16string FormatSnapshotDate(base::Time time) {
  std::unique_ptr<icu::DateFormat> formatter(
      icu::DateFormat::createDateInstance(icu::DateFormat::kMedium));
  formatter->adoptTimeZone(icu::TimeZone::getGMT()->clone());
  icu::UnicodeString date;
  formatter->format(time.InMillisecondsFSinceUnixEpoch(), date);
  return base::i18n::UnicodeStringToString16(date);
}

}  // namespace

// static
void WaybackMachineInfoBarDelegate::Create(
    content::WebContents* contents,
    const WaybackSnapshotInfo& snapshot_info) {
  auto* infobar_manager =
      infobars::ContentInfoBarManager::FromWebContents(contents);
  if (!infobar_manager) {
    return;
  }
  infobar_manager->AddInfoBar(CreateWaybackMachineInfoBar(
      base::WrapUnique(new WaybackMachineInfoBarDelegate(snapshot_info))));
}

// static
void WaybackMachineInfoBarDelegate::Remove(content::WebContents* contents) {
  auto* infobar_manager =
      infobars::ContentInfoBarManager::FromWebContents(contents);
  if (!infobar_manager) {
    return;
  }
  for (infobars::InfoBar* infobar : infobar_manager->infobars()) {
    if (infobar->delegate()->GetIdentifier() ==
        BRAVE_WAYBACK_MACHINE_INFOBAR_DELEGATE) {
      auto remove = [](base::WeakPtr<infobars::InfoBar> infobar) {
        if (infobar) {
          infobar->RemoveSelf();
        }
      };
      base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
          FROM_HERE, base::BindOnce(std::move(remove), infobar->AsWeakPtr()));
      return;
    }
  }
}

WaybackMachineInfoBarDelegate::WaybackMachineInfoBarDelegate(
    const WaybackSnapshotInfo& snapshot_info)
    : snapshot_info_(snapshot_info) {
  StartCountdownTimer();
}

WaybackMachineInfoBarDelegate::~WaybackMachineInfoBarDelegate() = default;

void WaybackMachineInfoBarDelegate::SetCountdownChangedCallback(
    base::RepeatingClosure callback) {
  countdown_changed_callback_ = std::move(callback);
}

std::u16string WaybackMachineInfoBarDelegate::GetTitleText() const {
  return l10n_util::GetStringUTF16(IDS_BRAVE_WAYBACK_MACHINE_INFOBAR_TITLE);
}

std::u16string WaybackMachineInfoBarDelegate::GetDescriptionText(
    size_t* learn_more_offset) const {
  CHECK(learn_more_offset);
  const std::u16string learn_more = GetLearnMoreText();
  if (snapshot_info_.time.is_null()) {
    return l10n_util::GetStringFUTF16(
        IDS_BRAVE_WAYBACK_MACHINE_INFOBAR_DESCRIPTION_NO_DATE, learn_more,
        learn_more_offset);
  }
  const base::TimeDelta age =
      std::max(base::Time::Now() - snapshot_info_.time, base::TimeDelta());
  const std::u16string time_ago = ui::TimeFormat::SimpleWithMonthAndYear(
      ui::TimeFormat::FORMAT_ELAPSED, ui::TimeFormat::LENGTH_LONG, age,
      /*use_month_and_year=*/true);
  std::vector<size_t> offsets;
  std::u16string text = l10n_util::GetStringFUTF16(
      IDS_BRAVE_WAYBACK_MACHINE_INFOBAR_DESCRIPTION,
      {FormatSnapshotDate(snapshot_info_.time), time_ago, learn_more},
      &offsets);
  *learn_more_offset = offsets.back();
  return text;
}

std::u16string WaybackMachineInfoBarDelegate::GetLearnMoreText() const {
  return l10n_util::GetStringUTF16(
      IDS_BRAVE_WAYBACK_MACHINE_INFOBAR_LEARN_MORE);
}

std::u16string WaybackMachineInfoBarDelegate::GetRedirectButtonText() const {
  return base::i18n::MessageFormatter::FormatWithNamedArgs(
      l10n_util::GetStringUTF16(
          IDS_BRAVE_WAYBACK_MACHINE_INFOBAR_REDIRECT_BUTTON),
      "SECONDS", seconds_remaining_);
}

std::u16string WaybackMachineInfoBarDelegate::GetCancelButtonText() const {
  return l10n_util::GetStringUTF16(
      IDS_BRAVE_WAYBACK_MACHINE_BUBBLE_DISMISS_BUTTON_TEXT);
}

void WaybackMachineInfoBarDelegate::Accept() {
  countdown_timer_.Stop();
  NavigateToSnapshot();
}

void WaybackMachineInfoBarDelegate::Cancel() {
  countdown_timer_.Stop();
}

void WaybackMachineInfoBarDelegate::OpenLearnMore() {
  countdown_timer_.Stop();
  if (auto* contents =
          infobars::ContentInfoBarManager::WebContentsFromInfoBar(infobar())) {
    contents->OpenURL(
        content::OpenURLParams(GURL(kLearnMoreURL), content::Referrer(),
                               WindowOpenDisposition::NEW_FOREGROUND_TAB,
                               ui::PAGE_TRANSITION_LINK,
                               /*is_renderer_initiated=*/false),
        /*navigation_handle_callback=*/{});
  }
}

infobars::InfoBarDelegate::InfoBarIdentifier
WaybackMachineInfoBarDelegate::GetIdentifier() const {
  return BRAVE_WAYBACK_MACHINE_INFOBAR_DELEGATE;
}

bool WaybackMachineInfoBarDelegate::EqualsDelegate(
    infobars::InfoBarDelegate* delegate) const {
  return delegate->GetIdentifier() == GetIdentifier();
}

bool WaybackMachineInfoBarDelegate::IsCloseable() const {
  return false;
}

void WaybackMachineInfoBarDelegate::StartCountdownTimer() {
  countdown_timer_.Start(FROM_HERE, base::Seconds(1), this,
                         &WaybackMachineInfoBarDelegate::OnCountdownTick);
}

void WaybackMachineInfoBarDelegate::OnCountdownTick() {
  // The infobar stays alive while its removal animates; it has no owner then.
  if (!infobar() || !infobar()->owner()) {
    return;
  }
  if (--seconds_remaining_ > 0) {
    StartCountdownTimer();
    if (countdown_changed_callback_) {
      countdown_changed_callback_.Run();
    }
    return;
  }
  NavigateToSnapshot();
}

void WaybackMachineInfoBarDelegate::NavigateToSnapshot() {
  auto* contents =
      infobars::ContentInfoBarManager::WebContentsFromInfoBar(infobar());
  auto* tab_helper =
      contents ? BraveWaybackMachineTabHelper::FromWebContents(contents)
               : nullptr;
  if (tab_helper) {
    tab_helper->MaybeNavigateToSnapshot();
  }
}
