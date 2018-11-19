#include "../../../../../chrome/common/importer/profile_import_process_param_traits_macros.h"

// TODO necessary?
#include <map>

#include "brave/common/importer/brave_ledger.h"
#include "brave/common/importer/brave_stats.h"

IPC_STRUCT_TRAITS_BEGIN(BraveStats)
  IPC_STRUCT_TRAITS_MEMBER(adblock_count)
  IPC_STRUCT_TRAITS_MEMBER(trackingProtection_count)
  IPC_STRUCT_TRAITS_MEMBER(httpsEverywhere_count)
IPC_STRUCT_TRAITS_END()

IPC_STRUCT_TRAITS_BEGIN(BraveLedger::SessionStoreSettings::PaymentSettings)
  IPC_STRUCT_TRAITS_MEMBER(allow_media_publishers)
  IPC_STRUCT_TRAITS_MEMBER(allow_non_verified)
  IPC_STRUCT_TRAITS_MEMBER(enabled)
  IPC_STRUCT_TRAITS_MEMBER(contribution_amount)
  IPC_STRUCT_TRAITS_MEMBER(min_visit_time)
  IPC_STRUCT_TRAITS_MEMBER(min_visits)
IPC_STRUCT_TRAITS_END()

IPC_STRUCT_TRAITS_BEGIN(BraveLedger::SessionStoreSettings)
  IPC_STRUCT_TRAITS_MEMBER(payments)
IPC_STRUCT_TRAITS_END()

IPC_STRUCT_TRAITS_BEGIN(BraveLedger)
  IPC_STRUCT_TRAITS_MEMBER(passphrase)
  IPC_STRUCT_TRAITS_MEMBER(clobber_wallet)
  IPC_STRUCT_TRAITS_MEMBER(excluded_publishers)
  IPC_STRUCT_TRAITS_MEMBER(pinned_publishers)
  IPC_STRUCT_TRAITS_MEMBER(settings)
IPC_STRUCT_TRAITS_END()
