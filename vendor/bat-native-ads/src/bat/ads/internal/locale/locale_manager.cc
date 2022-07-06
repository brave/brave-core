/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/locale/locale_manager.h"

#include "base/check_op.h"
#include "bat/ads/internal/base/logging_util.h"
#include "brave/components/l10n/browser/locale_helper.h"

namespace ads {

namespace {
LocaleManager* g_locale_manager_instance = nullptr;
}  // namespace

LocaleManager::LocaleManager() {
  DCHECK(!g_locale_manager_instance);
  g_locale_manager_instance = this;

  locale_ = GetLocale();
}

LocaleManager::~LocaleManager() {
  DCHECK_EQ(this, g_locale_manager_instance);
  g_locale_manager_instance = nullptr;
}

// static
LocaleManager* LocaleManager::GetInstance() {
  DCHECK(g_locale_manager_instance);
  return g_locale_manager_instance;
}

// static
bool LocaleManager::HasInstance() {
  return !!g_locale_manager_instance;
}

void LocaleManager::AddObserver(LocaleManagerObserver* observer) {
  DCHECK(observer);
  observers_.AddObserver(observer);
}

void LocaleManager::RemoveObserver(LocaleManagerObserver* observer) {
  DCHECK(observer);
  observers_.RemoveObserver(observer);
}

void LocaleManager::OnLocaleDidChange(const std::string& locale) {
  if (locale_ == locale) {
    return;
  }

  BLOG(1, "Locale changed from " << locale_ << " to " << locale);

  locale_ = locale;

  NotifyLocaleDidChange(locale);
}

std::string LocaleManager::GetLocale() const {
  return brave_l10n::LocaleHelper::GetInstance()->GetLocale();
}

///////////////////////////////////////////////////////////////////////////////

void LocaleManager::NotifyLocaleDidChange(const std::string& locale) const {
  for (LocaleManagerObserver& observer : observers_) {
    observer.OnLocaleDidChange(locale);
  }
}

}  // namespace ads
