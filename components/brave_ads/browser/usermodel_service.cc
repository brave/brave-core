#include "usermodel_service.h"
#include "base/task_runner_util.h"
#include "base/bind.h"
#include "base/guid.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/important_file_writer.h"
#include "base/i18n/time_formatting.h"
#include "base/sequenced_task_runner.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task_runner_util.h"
#include "base/task/post_task.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "base/memory/ref_counted.h"
#include "base/memory/singleton.h"
#include "base/memory/weak_ptr.h"
#include "user_model.h"
#include "user_model_callback_handler.h"
#include "content/public/common/isolated_world_ids.h"
#include "components/sessions/core/session_id.h"
#include "ui/gfx/image/image.h"
#include "ui/message_center/message_center.h"
#include "ui/message_center/public/cpp/message_center_constants.h"
#include "ui/message_center/public/cpp/notification_delegate.h"
#include "ui/message_center/public/cpp/notification_types.h"
#include "ui/message_center/public/cpp/notification.h"
#include "ui/message_center/public/cpp/notifier_id.h"
#include "base/strings/utf_string_conversions.h"
#include "third_party/re2/src/re2/re2.h"

#include "ads_relevance.h"

#include <algorithm>

#define NOTIFICATION_TIMEOUT 10 //seconds

using message_center::MessageCenter;
using message_center::Notification;

class AdNotificationDelegate : public message_center::NotificationDelegate {
 public:
 explicit AdNotificationDelegate(brave_ads::UsermodelService* service) :
  user_model_service_(service) {}

  void Close(bool by_user) override {
    if (by_user) {
      user_model_service_->OnNotificationEvent(usermodel::NotificationEventType::CLOSED);
    } else {
      user_model_service_->OnNotificationEvent(usermodel::NotificationEventType::TIMED_OUT);
    }
  }

  void Click(const base::Optional<int>& button_index,
             const base::Optional<base::string16>& reply) override {
    LOG(INFO) << reply.value_or(base::string16());
    user_model_service_->OnNotificationEvent(usermodel::NotificationEventType::CLICKED);
  }

 private:
  ~AdNotificationDelegate() override = default;
  brave_ads::UsermodelService* user_model_service_;
  DISALLOW_COPY_AND_ASSIGN(AdNotificationDelegate);
};

std::string LoadFileTaskRunner(
    const base::FilePath& path) {
  std::string data;
  bool success = base::ReadFileToString(path, &data);

  // Make sure the file isn't empty.
  if (!success || data.empty()) {
    LOG(ERROR) << "Failed to read file: " << path.MaybeAsASCII();
    return std::string();
  }
  return data;
}

time_t GetCurrentTimestamp() {
  return base::Time::NowFromSystemTime().ToTimeT();
}

time_t TimstampFromString(std::string time_str) {
  base::Time time = base::Time();
  if (base::Time::FromString(time_str.c_str(), &time)) {
    return time.ToTimeT();
  } else {
    return 0;
  }
}

void brave_ads::UsermodelService::OnNotificationEvent(usermodel::NotificationEventType event) {
  switch(event) {
      case usermodel::CLICKED  : LOG(INFO) << "CLICKED";   break;
      case usermodel::TIMED_OUT: LOG(INFO) << "TIMED_OUT"; break;
      case usermodel::CLOSED : LOG(INFO) << "CLOSED";  break;
      default:
      break;
  }
}

brave_ads::UsermodelService::UsermodelService(Profile* profile) :
    usermodel_state_(new UserModelState(profile->GetPath().AppendASCII("user_profile"))),
    ads_agent(new usermodel::AdsAgent(&usermodel_)),
    file_task_runner_(base::CreateSequencedTaskRunnerWithTraits(
        {base::MayBlock(), base::TaskPriority::BEST_EFFORT,
        base::TaskShutdownBehavior::BLOCK_SHUTDOWN})),
    usermodel_state_path_(profile->GetPath().AppendASCII("user_profile")),
    taxonomy_model_path_(profile->GetPath().AppendASCII("taxonomy_model.json")),
    ads_feed_path_(profile->GetPath().AppendASCII("bat-ads-feed.json")),
    ads_database_path_(profile->GetPath().AppendASCII("ads_database")),
    initialized_(false),
    last_focused_timestamp_(GetCurrentTimestamp()) {

    // load models
    base::PostTaskAndReplyWithResult(file_task_runner_.get(), FROM_HERE,
      base::Bind(&LoadFileTaskRunner, taxonomy_model_path_),
      base::Bind(&UsermodelService::OnModelLoaded, AsWeakPtr()));

    // load ads
    base::PostTaskAndReplyWithResult(file_task_runner_.get(), FROM_HERE,
      base::Bind(&LoadFileTaskRunner, ads_feed_path_),
      base::Bind(&UsermodelService::OnAdsLoaded, AsWeakPtr()));

    // TODO(ptigas): load from file the relevance model
    ads_agent->LoadRelevanceModel("{\"features\":[\"long_term_interest\", \"short_term_interest\", \"search_intent\"], \"weights\":[0.2, 0.4, 0.9], \"bias\": 0.5}");

    // initialize ads database
    ads_database_ = new AdsDatabase(ads_database_path_);

    LOG(INFO) << "FILE: " << usermodel_state_path_;
}

void brave_ads::UsermodelService::OnModelLoaded(const std::string& data) {
  usermodel_.initializePageClassifier(data);
  initialized_ = true;
}

void brave_ads::UsermodelService::OnAdsLoaded(const std::string& data) {
  ad_catalog_.load(data);
  LOG(INFO) << "Ads: Loaded " << ad_catalog_.ads_.size() << " Ads";
}

void brave_ads::UsermodelService::OnUserProfileLoaded(const std::string& data) {
  user_profile_ = usermodel::UserProfile::FromJSON(data);
}

void PostWriteCallback(
    const base::Callback<void(bool success)>& callback,
    scoped_refptr<base::SequencedTaskRunner> reply_task_runner,
    bool write_success) {
  reply_task_runner->PostTask(FROM_HERE,
                              base::Bind(callback, write_success));
}

void brave_ads::UsermodelService::OnUsermodelStateSaved(bool success) {

}

void brave_ads::UsermodelService::SaveUsermodelState(const std::string& state) {
  base::ImportantFileWriter writer(
      usermodel_state_path_, file_task_runner_);

  writer.RegisterOnNextWriteCallbacks(
    base::Closure(),
    base::Bind(
      &PostWriteCallback,
      base::Bind(&brave_ads::UsermodelService::OnUsermodelStateSaved, AsWeakPtr()),
      base::SequencedTaskRunnerHandle::Get()));

  writer.WriteNow(std::make_unique<std::string>(state));
}


void write_database(const std::string& key, const std::string& value, brave_ads::UserModelState *db) {
  db->Put(key, value);
}

void brave_ads::UsermodelService::UpdateState(const std::string& key, const std::string& value) {
  file_task_runner_->PostTask(FROM_HERE,
                              base::BindOnce(&write_database,
                              key,
                              value,
                              usermodel_state_));
}

std::set<std::string> brave_ads::UsermodelService::GetAdsHistory(int timestamp) {
  std::set<std::string> seen_ads_ids; // filter catalog
  ads_database_->AdsSeen(timestamp, &seen_ads_ids);
  return seen_ads_ids;
}

void brave_ads::UsermodelService::RankAdsAndShow(const std::set<std::string>& seen_ads_ids) {
  for (auto id : seen_ads_ids ) {
    LOG(INFO) << "Seen - " << id;
  }

  std::vector<usermodel::Ad> not_seen_ads;
  for (auto ad : ad_catalog_.ads_) {
    if (seen_ads_ids.count(ad.uuid) == 0) {
      not_seen_ads.push_back(ad);
    }
  }

  LOG(INFO) << "Not seen ads: " << not_seen_ads.size();
  auto index = ads_agent->AdsScoreAndSample(not_seen_ads, *user_profile_);

  LOG(INFO) << "Sampled ad index: " << index;
  if (index != -1) {
    ShowAd(not_seen_ads.at(index));
  }

  LOG(INFO) << (GetCurrentTimestamp()-last_focused_timestamp_);
}

void brave_ads::UsermodelService::OnTabFocused(SessionID tab_id) {
  if ( tab_cache_.find(tab_id) == tab_cache_.end() ) {
    // not found
    LOG(INFO) << "Visited url not classified yet";
  } else {
    LOG(INFO) << "Visited url: "  << usermodel::UserModel::winningCategory(tab_cache_.at(tab_id), usermodel_.page_classifier.Classes());
  }

  if ((GetCurrentTimestamp()-last_focused_timestamp_) > 60*30) { // 30 minutes

  }

  base::PostTaskAndReplyWithResult(file_task_runner_.get(), FROM_HERE,
    base::Bind(&brave_ads::UsermodelService::GetAdsHistory,
                base::Unretained(this),
                GetCurrentTimestamp() - 60*30),
    base::Bind(&brave_ads::UsermodelService::RankAdsAndShow, AsWeakPtr()));

  // updated timestamp
  last_focused_timestamp_ = GetCurrentTimestamp();
}

bool IsSearchUrl(const std::string& url) {
  return (url.find("www.google.") != std::string::npos) ||
         (url.find("www.bing.") != std::string::npos)   ||
         (url.find("duckduckgo.") != std::string::npos) ||
         (url.find("search.yahoo.") != std::string::npos);
}

void brave_ads::UsermodelService::Classify(const std::string& html, const std::string& url, SessionID tab_id) {
  LOG(INFO) << "Start classification";
  auto scores = usermodel_.classifyPage(html);

  // update tabs cache
  if (tab_cache_.find(tab_id) != tab_cache_.end()) {
    tab_cache_[tab_id] = scores;
  } else {
    tab_cache_.insert(std::pair<SessionID, std::vector<double>>(tab_id, scores));
  }

  // update profiles
  std::string profile_json;
  std::string updated_timestamp_s;
  time_t time_since_last_update;

  if (!usermodel_state_->Get("user_profile_update_timestamp", &updated_timestamp_s)) {
    time_since_last_update  = GetCurrentTimestamp(); // very high number
  } else {
    time_since_last_update = (GetCurrentTimestamp() - TimstampFromString(updated_timestamp_s));
  }

  if (!usermodel_state_->Get("user_profile", &profile_json)) {
    profile_json = std::string("{}");
  }

  user_profile_ = usermodel::UserProfile::FromJSON(profile_json);
  user_profile_->Update(scores, time_since_last_update, IsSearchUrl(url));

  if (!usermodel_state_->Put("user_profile", user_profile_->ToJSON())) {
    LOG(WARNING) << "Could not update user_profile";
  }

  usermodel_state_->Put(
    "user_profile_update_timestamp",
    std::to_string(GetCurrentTimestamp())
  );

  auto predicted = usermodel::UserModel::winningCategory(scores, usermodel_.page_classifier.Classes());
  LOG(INFO) << "Predicted class: "  << predicted;
}

void brave_ads::UsermodelService::OnDataReceived(SessionID tab_id, const std::string& url, const base::Value* val) {
  std::string html;
  val->GetAsString(&html);
  file_task_runner_->PostTask(FROM_HERE,
                 base::BindOnce(&brave_ads::UsermodelService::Classify,
                 base::Unretained(this),
                 html,
                 url,
                 tab_id));
}

void brave_ads::UsermodelService::AddToHistory(const usermodel::Ad& ad) {
  ads_database_->PushToHistory(ad);
}

void brave_ads::UsermodelService::ShowAd(const usermodel::Ad& ad) {
  //
  // TODO: implement ShowAd(with id)
  // - check if allowed to show ad
  // - show notification
  // - add Ad to list of shown ads
  //

  // checkConstraints()
  // - doNotDisturbFlag

  std::unique_ptr<message_center::Notification> notification
    = message_center::Notification::CreateSystemNotification(
        message_center::NOTIFICATION_TYPE_IMAGE,
        "ads",
        base::ASCIIToUTF16(ad.advertiser),
        base::ASCIIToUTF16(ad.notification_text),
        base::ASCIIToUTF16(ad.advertiser),
        GURL(ad.notification_url),
        message_center::NotifierId(
                  message_center::NotifierId::SYSTEM_COMPONENT, "bat.ads"),
        message_center::RichNotificationData(),
        new AdNotificationDelegate(this),
        gfx::kNoneIcon,
        message_center::SystemNotificationWarningLevel::NORMAL);

  notification->SetSystemPriority();

  message_center::MessageCenter::Get()->AddNotification(std::move(notification));

  base::SequencedTaskRunnerHandle::Get()->PostDelayedTask(
    FROM_HERE, base::BindOnce([] () {
      LOG(INFO) << "Time out";
      message_center::MessageCenter::Get()->RemoveNotification("ads", false);
    }), base::TimeDelta::FromSeconds(NOTIFICATION_TIMEOUT));

  // push to history
  file_task_runner_->PostTask(FROM_HERE,
                              base::BindOnce(&brave_ads::UsermodelService::AddToHistory,
                              base::Unretained(this),
                              ad));
}

void brave_ads::UsermodelService::OnPageVisited(SessionID tab_id, content::RenderFrameHost* render_frame_host, const std::string& url) {
  LOG(INFO) << "Fetching the html";
  std::string js("document.getElementsByTagName('html')[0].innerHTML");
  render_frame_host->ExecuteJavaScriptInIsolatedWorld(
    base::UTF8ToUTF16(js),
    base::Bind(&brave_ads::UsermodelService::OnDataReceived, base::Unretained(this), tab_id, url),
    content::ISOLATED_WORLD_ID_USERMODEL
  );

  file_task_runner_->PostTask(FROM_HERE,
                              base::BindOnce(&write_database,
                              "initialized",
                              "true",
                              usermodel_state_));
}

brave_ads::UsermodelService::~UsermodelService() {
  //file_task_runner_->DeleteSoon(FROM_HERE, ads_database_->release());
}
