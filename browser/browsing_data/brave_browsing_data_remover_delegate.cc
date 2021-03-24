/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/browsing_data/brave_browsing_data_remover_delegate.h"

#include <memory>
#include <utility>

#include "brave/components/content_settings/core/browser/brave_content_settings_pref_provider.h"
#include "brave/components/content_settings/core/browser/brave_content_settings_utils.h"
#include "chrome/browser/browsing_data/chrome_browsing_data_remover_constants.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/buildflags.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "brave/common/extensions/api/brave_today.h"
#include "extensions/browser/event_router.h"
#endif

#if BUILDFLAG(IPFS_ENABLED)
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/process/launch.h"
#include "base/process/process.h"
#include "base/task/task_traits.h"
#include "base/task/thread_pool.h"
#include "base/threading/thread_restrictions.h"
#include "base/time/time.h"
#include "brave/browser/ipfs/ipfs_service_factory.h"
#include "brave/components/ipfs/ipfs_service.h"
#endif

BraveBrowsingDataRemoverDelegate::BraveBrowsingDataRemoverDelegate(
    content::BrowserContext* browser_context)
    : ChromeBrowsingDataRemoverDelegate(browser_context),
      profile_(Profile::FromBrowserContext(browser_context)) {}

BraveBrowsingDataRemoverDelegate::~BraveBrowsingDataRemoverDelegate() = default;

void BraveBrowsingDataRemoverDelegate::RemoveEmbedderData(
    const base::Time& delete_begin,
    const base::Time& delete_end,
    uint64_t remove_mask,
    content::BrowsingDataFilterBuilder* filter_builder,
    uint64_t origin_type_mask,
    base::OnceCallback<void(/*failed_data_types=*/uint64_t)> callback) {
  ChromeBrowsingDataRemoverDelegate::RemoveEmbedderData(delete_begin,
                                                        delete_end,
                                                        remove_mask,
                                                        filter_builder,
                                                        origin_type_mask,
                                                        std::move(callback));

  // We do this because ChromeBrowsingDataRemoverDelegate::RemoveEmbedderData()
  // doesn't clear shields settings with non all time range.
  // The reason is upstream assumes that plugins type only as empty string
  // resource ids with plugins type. but we use plugins type to store our
  // shields settings with non-empty resource ids.
  if (remove_mask & chrome_browsing_data_remover::DATA_TYPE_CONTENT_SETTINGS)
    ClearShieldsSettings(delete_begin, delete_end);

#if BUILDFLAG(IPFS_ENABLED)
  if (remove_mask & content::BrowsingDataRemover::DATA_TYPE_CACHE)
    ClearIPFSCache();
#endif

#if BUILDFLAG(ENABLE_EXTENSIONS)
  if (remove_mask & chrome_browsing_data_remover::DATA_TYPE_HISTORY) {
    auto* event_router = extensions::EventRouter::Get(profile_);
    if (event_router) {
      std::unique_ptr<base::ListValue> args(
          extensions::api::brave_today::OnClearHistory::Create().release());
      std::unique_ptr<extensions::Event> event(new extensions::Event(
          extensions::events::BRAVE_START,
          extensions::api::brave_today::OnClearHistory::kEventName,
          std::move(args)));
      event_router->BroadcastEvent(std::move(event));
    }
  }
#endif
}

void BraveBrowsingDataRemoverDelegate::ClearShieldsSettings(
    base::Time begin_time,
    base::Time end_time) {
  if (begin_time.is_null() && (end_time.is_null() || end_time.is_max())) {
    // For all time range, we don't need to do anything here because
    // ChromeBrowsingDataRemoverDelegate::RemoveEmbedderData() nukes whole
    // plugins type.
    return;
  }

  auto* map = HostContentSettingsMapFactory::GetForProfile(profile_);
  auto* provider =
      static_cast<content_settings::BravePrefProvider*>(map->GetPrefProvider());
  for (const auto& content_type :
       content_settings::GetShieldsContentSettingsTypes()) {
    ContentSettingsForOneType settings;
    map->GetSettingsForOneType(content_type, &settings);
    for (const ContentSettingPatternSource& setting : settings) {
      base::Time last_modified = provider->GetWebsiteSettingLastModified(
          setting.primary_pattern, setting.secondary_pattern, content_type);
      if (last_modified >= begin_time &&
          (last_modified < end_time || end_time.is_null())) {
        provider->SetWebsiteSetting(setting.primary_pattern,
                                    setting.secondary_pattern, content_type,
                                    nullptr, {});
      }
    }
  }
}

#if BUILDFLAG(IPFS_ENABLED)
void BraveBrowsingDataRemoverDelegate::WaitForIPFSRepoGC(
    base::Process process) {
  bool exited = false;

  {
    base::ScopedAllowBaseSyncPrimitives scoped_allow_base_sync_primitives;

    // Because we set maximum IPFS storage size as 1GB in Brave, ipfs repo gc
    // command should be finished in just a few seconds and we do not expect
    // this child process would hang forever. To be safe, we will wait for 30
    // seconds max here.
    exited = process.WaitForExitWithTimeout(base::TimeDelta::FromSeconds(30),
                                            nullptr);
  }

  if (!exited)
    process.Terminate(0, false /* wait */);
}

// Run ipfs repo gc command to clear IPFS cache when IPFS executable path is
// available. Because the command does not support time ranged cleanup, we will
// always clear the whole cache expect for pinned files when clearing browsing
// data.
void BraveBrowsingDataRemoverDelegate::ClearIPFSCache() {
  auto* service =
      ipfs::IpfsServiceFactory::GetInstance()->GetForContext(profile_);
  if (!service)
    return;

  base::FilePath path = service->GetIpfsExecutablePath();
  if (path.empty())
    return;

  base::CommandLine cmdline(path);
  cmdline.AppendArg("repo");
  cmdline.AppendArg("gc");

  base::FilePath data_path = service->GetDataPath();
  base::LaunchOptions options;
#if defined(OS_WIN)
  options.environment[L"IPFS_PATH"] = data_path.value();
#else
  options.environment["IPFS_PATH"] = data_path.value();
#endif
#if defined(OS_LINUX)
  options.kill_on_parent_death = true;
#endif
#if defined(OS_WIN)
  options.start_hidden = true;
#endif

  base::Process process = base::LaunchProcess(cmdline, options);
  if (!process.IsValid()) {
    return;
  }

  base::ThreadPool::PostTaskAndReply(
      FROM_HERE,
      {base::TaskPriority::USER_VISIBLE, base::MayBlock(),
       base::TaskShutdownBehavior::CONTINUE_ON_SHUTDOWN},
      base::BindOnce(&BraveBrowsingDataRemoverDelegate::WaitForIPFSRepoGC,
                     weak_ptr_factory_.GetWeakPtr(), std::move(process)),
      CreateTaskCompletionClosure(TracingDataType::kIPFSCache));
}
#endif  // BUILDFLAG(IPFS_ENABLED)
