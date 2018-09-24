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

/*
std::unique_ptr<message_center::Notification> CreateNotification() {
  std::unique_ptr<message_center::Notification> notification 
  = message_center::Notification::CreateSystemNotification(
      "ads",
      base::string16(),
      base::string16(),
      "bat_ads",
      base::BindRepeating(&HandleNotificationClick));

  
  return notification;
}*/

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
    file_task_runner_(base::CreateSequencedTaskRunnerWithTraits(
        {base::MayBlock(), base::TaskPriority::BEST_EFFORT,
        base::TaskShutdownBehavior::BLOCK_SHUTDOWN})),
    usermodel_state_path_(profile->GetPath().AppendASCII("user_profile")),
    taxonomy_model_path_(profile->GetPath().AppendASCII("taxonomy_model.json")),
    ads_feed_path_(profile->GetPath().AppendASCII("bat-ads-feed.json")),
    initialized_(false),
    last_focused_timestamp_(GetCurrentTimestamp()) {

    // load models
    base::PostTaskAndReplyWithResult(file_task_runner_.get(), FROM_HERE,
      base::Bind(&LoadFileTaskRunner, taxonomy_model_path_),
      base::Bind(&UsermodelService::OnModelLoaded, AsWeakPtr()));

    // load models
    base::PostTaskAndReplyWithResult(file_task_runner_.get(), FROM_HERE,
      base::Bind(&LoadFileTaskRunner, ads_feed_path_),
      base::Bind(&UsermodelService::OnAdsLoaded, AsWeakPtr()));

    LOG(INFO) << "FILE: " << usermodel_state_path_;
}

void brave_ads::UsermodelService::OnModelLoaded(const std::string& data) {
  usermodel_.initializePageClassifier(data);
  initialized_ = true;
}

void brave_ads::UsermodelService::OnAdsLoaded(const std::string& data) {
  ad_catalog_.load(data);
  LOG(INFO) << "Ads: Loaded " << ad_catalog_.ads_.size() << " Ads"; 

  ShowAd();
}

void brave_ads::UsermodelService::OnUserProfileLoaded(const std::string& data) {
  user_profile_ = usermodel::UserProfile::FromJSON(data);
}

// `callback` has a WeakPtr so this won't crash if the file finishes
// writing after RewardsServiceImpl has been destroyed
void PostWriteCallback(
    const base::Callback<void(bool success)>& callback,
    scoped_refptr<base::SequencedTaskRunner> reply_task_runner,
    bool write_success) {
  // We can't run |callback| on the current thread. Bounce back to
  // the |reply_task_runner| which is the correct sequenced thread.
  reply_task_runner->PostTask(FROM_HERE,
                              base::Bind(callback, write_success));
}

void brave_ads::UsermodelService::OnUsermodelStateSaved(bool success) {
  //handler->OnLedgerStateSaved(success ? ledger::Result::LEDGER_OK
  //                                    : ledger::Result::NO_LEDGER_STATE);
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

void brave_ads::UsermodelService::OnTabFocused(SessionID tab_id) {
  if ( tab_cache_.find(tab_id) == tab_cache_.end() ) {
    // not found
    LOG(INFO) << "Visited url not classified yet";
  } else {
    LOG(INFO) << "Visited url: "  << usermodel_.winningCategory(tab_cache_.at(tab_id));
  }

  if ((GetCurrentTimestamp()-last_focused_timestamp_) > 60*30) { // 30 minutes
    
  }

  LOG(INFO) << (GetCurrentTimestamp()-last_focused_timestamp_);

  // updated timestamp
  last_focused_timestamp_ = GetCurrentTimestamp();
}

void brave_ads::UsermodelService::Classify(const std::string& html, const std::string& url, SessionID tab_id) {
  LOG(INFO) << "Start classification";
  auto scores = usermodel_.classifyPage(html);

  // update profiles
  std::string profile_json;

  //usermodel_service_->usermodel_state_->Get("user_profile", &profile_json);
  
//  auto profile = usermodel::UserProfile::FromJSON(profile_json);
  LOG(INFO) << "Profile: "  << profile_json;

  if (tab_cache_.find(tab_id) != tab_cache_.end()) {
    tab_cache_[tab_id] = scores;
  } else {
    tab_cache_.insert(std::pair<SessionID, std::vector<double>>(tab_id, scores));
  }
//  profile->Update(scores, url);

  auto predicted = usermodel_.winningCategory(scores);
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

void brave_ads::UsermodelService::ShowAd() {
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
    base::ASCIIToUTF16("test title"),
    base::ASCIIToUTF16("test message"),
    base::ASCIIToUTF16("test display source"),
    GURL("https://ptigas.com"),
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
    }), base::TimeDelta::FromSeconds(10));

  // ads_history_->push(uuid)
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
  //file_task_runner_->DeleteSoon(FROM_HERE, publisher_info_backend_.release());
}
