#include "../../../../../chrome/common/importer/profile_import_process_param_traits_macros.h"

#include "brave/common/importer/brave_ledger.h"
#include "brave/common/importer/brave_referral.h"
#include "brave/common/importer/brave_stats.h"
#include "brave/common/importer/imported_browser_window.h"

IPC_STRUCT_TRAITS_BEGIN(BraveStats)
  IPC_STRUCT_TRAITS_MEMBER(adblock_count)
  IPC_STRUCT_TRAITS_MEMBER(trackingProtection_count)
  IPC_STRUCT_TRAITS_MEMBER(httpsEverywhere_count)
IPC_STRUCT_TRAITS_END()

IPC_STRUCT_TRAITS_BEGIN(BravePublisher)
  IPC_STRUCT_TRAITS_MEMBER(key)
  IPC_STRUCT_TRAITS_MEMBER(verified)
  IPC_STRUCT_TRAITS_MEMBER(name)
  IPC_STRUCT_TRAITS_MEMBER(url)
  IPC_STRUCT_TRAITS_MEMBER(provider)
  IPC_STRUCT_TRAITS_MEMBER(pin_percentage)
IPC_STRUCT_TRAITS_END()

IPC_STRUCT_TRAITS_BEGIN(SessionStoreSettings::PaymentSettings)
  IPC_STRUCT_TRAITS_MEMBER(allow_media_publishers)
  IPC_STRUCT_TRAITS_MEMBER(allow_non_verified)
  IPC_STRUCT_TRAITS_MEMBER(enabled)
  IPC_STRUCT_TRAITS_MEMBER(contribution_amount)
  IPC_STRUCT_TRAITS_MEMBER(min_visit_time)
  IPC_STRUCT_TRAITS_MEMBER(min_visits)
IPC_STRUCT_TRAITS_END()

IPC_STRUCT_TRAITS_BEGIN(SessionStoreSettings)
  IPC_STRUCT_TRAITS_MEMBER(payments)
  IPC_STRUCT_TRAITS_MEMBER(default_search_engine)
  IPC_STRUCT_TRAITS_MEMBER(use_alternate_private_search_engine)
  IPC_STRUCT_TRAITS_MEMBER(use_alternate_private_search_engine_tor)
IPC_STRUCT_TRAITS_END()

IPC_STRUCT_TRAITS_BEGIN(BraveLedger)
  IPC_STRUCT_TRAITS_MEMBER(passphrase)
  IPC_STRUCT_TRAITS_MEMBER(excluded_publishers)
  IPC_STRUCT_TRAITS_MEMBER(pinned_publishers)
  IPC_STRUCT_TRAITS_MEMBER(settings)
IPC_STRUCT_TRAITS_END()

IPC_STRUCT_TRAITS_BEGIN(BraveReferral)
  IPC_STRUCT_TRAITS_MEMBER(promo_code)
  IPC_STRUCT_TRAITS_MEMBER(download_id)
  IPC_STRUCT_TRAITS_MEMBER(finalize_timestamp)
  IPC_STRUCT_TRAITS_MEMBER(week_of_installation)
IPC_STRUCT_TRAITS_END()

IPC_STRUCT_TRAITS_BEGIN(ImportedBrowserTab)
  IPC_STRUCT_TRAITS_MEMBER(key)
  IPC_STRUCT_TRAITS_MEMBER(location)
IPC_STRUCT_TRAITS_END()

IPC_STRUCT_TRAITS_BEGIN(ImportedBrowserWindow)
  IPC_STRUCT_TRAITS_MEMBER(top)
  IPC_STRUCT_TRAITS_MEMBER(left)
  IPC_STRUCT_TRAITS_MEMBER(width)
  IPC_STRUCT_TRAITS_MEMBER(height)
  IPC_STRUCT_TRAITS_MEMBER(focused)
  IPC_STRUCT_TRAITS_MEMBER(state)
  IPC_STRUCT_TRAITS_MEMBER(activeFrameKey)
  IPC_STRUCT_TRAITS_MEMBER(tabs)
IPC_STRUCT_TRAITS_END()

IPC_STRUCT_TRAITS_BEGIN(ImportedWindowState)
  IPC_STRUCT_TRAITS_MEMBER(windows)
  IPC_STRUCT_TRAITS_MEMBER(pinnedTabs)
IPC_STRUCT_TRAITS_END()
