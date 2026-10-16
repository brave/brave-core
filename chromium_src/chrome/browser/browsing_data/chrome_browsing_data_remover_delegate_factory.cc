/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/browsing_data/chrome_browsing_data_remover_delegate_factory.h"

#include "brave/browser/browsing_data/brave_browsing_data_remover_delegate.h"
#include "brave/browser/misc_metrics/profile_misc_metrics_service_factory.h"
#include "brave/browser/serp_metrics/serp_metrics_service_factory.h"
#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"
#include "brave/components/brave_news/common/buildflags/buildflags.h"

#if BUILDFLAG(ENABLE_AI_CHAT)
#include "brave/browser/ai_chat/ai_chat_service_factory.h"
#endif

#if BUILDFLAG(ENABLE_BRAVE_NEWS)
#include "brave/browser/brave_news/brave_news_controller_factory.h"
#endif

#include <chrome/browser/browsing_data/chrome_browsing_data_remover_delegate_factory.cc>
