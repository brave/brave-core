/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_sync/history.h"

// #include <memory>
// #include <string>
#include "base/logging.h"
#include "base/strings/utf_string_conversions.h"
#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_sync/can_send_history.h"
#include "brave/components/brave_sync/jslib_messages.h"
#include "brave/components/brave_sync/tools.h"
#include "brave/components/brave_sync/values_conv.h"
#include "brave/components/brave_sync/object_map.h"
#include "chrome/browser/history/history_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "components/history/core/browser/history_service.h"
#include "components/history/core/browser/url_row.h"

//( see src/chrome/browser/extensions/api/history/history_api.h as example)

namespace brave_sync {

History::History(Profile* profile/*,
                 history::HistoryService* history_service*/
                 ,
                  CanSendSyncHistory *send_history) :
  profile_(profile), history_service_observer_(this), send_history_(send_history), sync_obj_map_(nullptr) {
  DCHECK(profile);
  DCHECK(send_history);

  LOG(ERROR) << "TAGAB brave_sync::History::History CTOR profile_="<<profile_;

  history::HistoryService* history_service = HistoryServiceFactory::GetForProfile(
               profile, ServiceAccessType::EXPLICIT_ACCESS);


  history_service_observer_.Add(history_service);
;
}

History::~History() {
  LOG(ERROR) << "TAGAB brave_sync::History::History DTOR";
}

void History::SetObjectMap(storage::ObjectMap* sync_obj_map) {
  DCHECK(sync_obj_map);
  DCHECK(!sync_obj_map_);
  sync_obj_map_ = sync_obj_map;
}

void History::OnURLVisited(history::HistoryService* history_service,
                           ui::PageTransition transition,
                           const history::URLRow& row,
                           const history::RedirectList& redirects,
                           base::Time visit_time) {
  LOG(ERROR) << "TAGAB brave_sync::History::OnURLVisited " << row.url().spec();

  // std::unique_ptr<base::ListValue> args =
  //     OnVisited::Create(GetHistoryItem(row));
  // DispatchEvent(profile_, events::HISTORY_ON_VISITED,
  //               api::history::OnVisited::kEventName, std::move(args));
}

void History::OnURLsDeleted(
    history::HistoryService* history_service,
    const history::DeletionInfo& deletion_info) {
  LOG(ERROR) << "TAGAB brave_sync::History::OnURLsDeleted ";
  LOG(ERROR) << "TAGAB deletion_info.IsAllHistory()=" << deletion_info.IsAllHistory();
  for (const auto& row : deletion_info.deleted_rows()) {
    LOG(ERROR) << "TAGAB row.url()=" << row.url().spec();
  }


  // OnVisitRemoved::Removed removed;
  // removed.all_history = deletion_info.IsAllHistory();
  //
  // std::vector<std::string>* urls = new std::vector<std::string>();
  // for (const auto& row : deletion_info.deleted_rows())
  //   urls->push_back(row.url().spec());
  // removed.urls.reset(urls);
  //
  // std::unique_ptr<base::ListValue> args = OnVisitRemoved::Create(removed);
  // DispatchEvent(profile_, events::HISTORY_ON_VISIT_REMOVED,
  //               api::history::OnVisitRemoved::kEventName, std::move(args));
}


void History::GetAllHistory() {
  LOG(ERROR) << "TAGAB History::GetAllHistory";

  // Need several steps
  ///ExtensionFunction::ResponseAction HistorySearchFunction::Run() {


  history::QueryOptions options; // default options should allow to get all entries

  // options.SetRecentDayRange(1);
  // options.max_count = 100;
  //
  // if (params->query.start_time.get())
  //   options.begin_time = GetTime(*params->query.start_time);
  // if (params->query.end_time.get())
  //   options.end_time = GetTime(*params->query.end_time);
  // if (params->query.max_results.get())
  //   options.max_count = *params->query.max_results;

  history::HistoryService* hs = HistoryServiceFactory::GetForProfile(
      /*GetProfile()*/profile_, ServiceAccessType::EXPLICIT_ACCESS);
  hs->QueryHistory(/*search_text*/base::ASCIIToUTF16(""),
                   options,
                   base::Bind(&History::GetAllHistoryComplete,
                              base::Unretained(this)),
                   &task_tracker_);
//


// TODO: AB: time with
/*
double MilliSecondsFromTime(const base::Time& time) {
  return 1000 * time.ToDoubleT();
}
*/

    //AB: AddRef();               // Balanced in SearchComplete().
/*
AddRef and Release are from:
// knows how to dispatch to.
class ExtensionFunction
    : public base::RefCountedThreadSafe<ExtensionFunction,
                                        ExtensionFunctionDeleteTraits> {
*/
}

void History::GetAllHistoryComplete(history::QueryResults* results) {
  LOG(ERROR) << "TAGAB History::GetAllHistoryComplete";
  DCHECK(send_history_);

  //HistoryItemList history_item_vec;
  if (results && !results->empty()) {
    LOG(ERROR) << "TAGAB History::GetAllHistoryComplete results->size()=" << results->size();
    for (const auto& item : *results){
      //history_item_vec.push_back(GetHistoryItem(item));
      //item.visit_time();
      item.visit_time();
      LOG(ERROR) << "History::GetAllHistoryComplete item=" << item.url().spec();
    }
    // inform controller have result
    send_history_->HaveInitialHistory(results);
  }
  ;

  //AB: Release();


}

std::unique_ptr<RecordsList> History::NativeHistoryToSyncRecords(
  const history::QueryResults::URLResultVector &list,
  int action) {

  LOG(ERROR) << "TAGAB NativeHistoryToSyncRecords:";
  std::unique_ptr<RecordsList> records = std::make_unique<RecordsList>();

  for (const history::URLResult &history_entry : list) {
    LOG(ERROR) << "TAGAB NativeHistoryToSyncRecords: history_entry=" << &history_entry;

    // int64_t parent_folder_id = node->parent() ? node->parent()->id() : 0;
    // std::string parent_folder_object_sync_id;
    // if (parent_folder_id) {
    //   parent_folder_object_sync_id = GetOrCreateObjectByLocalId(parent_folder_id);
    // }

    //URLID id() const { return id_; }
    //typedef int64_t URLID;
    std::string object_id = GetOrCreateObjectByLocalId(history_entry.id());
    LOG(ERROR) << "TAGAB NativeBookmarksToSyncLV: object_id=<" << object_id << ">";
    CHECK(!object_id.empty());

    std::unique_ptr<jslib::SyncRecord> record = std::make_unique<jslib::SyncRecord>();
    record->action = ConvertEnum<brave_sync::jslib::SyncRecord::Action>(action,
        brave_sync::jslib::SyncRecord::Action::A_MIN,
        brave_sync::jslib::SyncRecord::Action::A_MAX,
        brave_sync::jslib::SyncRecord::Action::A_INVALID);
    record->deviceId = device_id_;
    record->objectId = object_id;
    record->objectData = "historySite";

    auto history_site = std::make_unique<jslib::Site>();
    history_site->location = history_entry.url().spec();
    history_site->title = base::UTF16ToUTF8(history_entry.title());
    //history_site->customTitle = base::UTF16ToUTF8(node->GetTitle());
    history_site->lastAccessedTime = history_entry.last_visit();
    //history_site->creationTime = node->date_added();
    //history_site->favicon = node->icon_url() ? node->icon_url()->spec() : "";
    record->SetHistorySite(std::move(history_site));

    record->syncTimestamp = base::Time::Now();
    records->emplace_back(std::move(record));
  }

  return records;
}

// TODO, AB: duplicate
std::string History::GetOrCreateObjectByLocalId(const int64_t &local_id) {
  CHECK(sync_obj_map_);
  const std::string s_local_id = base::Int64ToString(local_id);
  std::string object_id = sync_obj_map_->GetObjectIdByLocalId(s_local_id);
  if (!object_id.empty()) {
    return object_id;
  }

  object_id = tools::GenerateObjectId(); // TODO, AB: pack 8 bytes from s_local_id?
  sync_obj_map_->SaveObjectId(
        s_local_id,
        "",// order or empty
        object_id);

  return object_id;
}

void History::SetThisDeviceId(const std::string &device_id) {
  DCHECK(device_id_.empty());
  DCHECK(!device_id.empty());
  device_id_ = device_id;
}

std::unique_ptr<jslib::SyncRecord> History::GetResolvedHistoryValue(const std::string &object_id) {
  LOG(ERROR) << "TAGAB brave_sync::History::GetResolvedHistoryValue object_id=<"<<object_id<<">";
  std::string local_object_id = sync_obj_map_->GetLocalIdByObjectId(object_id);
  LOG(ERROR) << "TAGAB brave_sync::History::GetResolvedHistoryValue local_object_id=<"<<local_object_id<<">";
  if(local_object_id.empty()) {
    return nullptr;
  }

  int64_t id = 0;
  bool convert_result = base::StringToInt64(local_object_id, &id);
  DCHECK(convert_result);
  if (!convert_result) {
    return nullptr;
  }

  ;;;;
  // Get bookmark by ID
    // src/chrome/browser/extensions/api/history/history_api.h

    // Need to use bool HistoryBackend::GetURLByID(URLID url_id, URLRow* url_row) {

  history::URLRow* url_row = nullptr;

/*

similar to
hs->QueryHistory(search_text,
                 options,
                 base::Bind(&HistorySearchFunction::SearchComplete,
                            base::Unretained(this)),
                 &task_tracker_);

QueryRowsByIds(const std::vector<URLID>&vec_ids, std::vector<URLRow> &rows)
*/


  if (url_row) {
    LOG(ERROR) << "TAGAB brave_sync::History::GetResolvedHistoryValue node not found for local_object_id=<"<<local_object_id<<">";
    return nullptr;
  }


  // const bookmarks::BookmarkNode* node = bookmarks::GetBookmarkNodeByID(model_, id);
  // if (node == nullptr) {
  //   LOG(ERROR) << "TAGAB brave_sync::Bookmarks::GetResolvedBookmarkValue2 node not found for local_object_id=<"<<local_object_id<<">";
  //   // Node was removed
  //   // NOTREACHED() << "means we had not removed (object_id => local_id) pair from objects map";
  //   // Something gone wrong previously, no obvious way to fix
  //
  //   return nullptr;
  // }

  //std::unique_ptr<base::Value> value = BookmarkToValue(node, object_id);
  auto record = std::make_unique<jslib::SyncRecord>();
  record->action = jslib::SyncRecord::Action::CREATE;
  record->deviceId = device_id_;
  record->objectId = object_id;

  std::unique_ptr<jslib::Site> history_site = GetFromUrlRow(url_row);
  record->SetHistorySite(std::move(history_site));

  return record;
}

std::unique_ptr<jslib::Site> History::GetFromUrlRow(const history::URLRow* url_row) {
  auto history_site = std::make_unique<jslib::Site>();

  history_site->location = url_row->url().spec();;
  history_site->title = base::UTF16ToUTF8(url_row->title());
  //history_site->customTitle = base::UTF16ToUTF8(node->GetTitle());
  history_site->lastAccessedTime = url_row->last_visit();
  //history_site->creationTime = ??;
  //history_site->favicon = ??;

  return nullptr;
}



} // namespace brave_sync
