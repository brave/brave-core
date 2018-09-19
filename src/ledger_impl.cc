/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <ctime>
#include <random>

#include "ledger_impl.h"

#include "bat_client.h"
#include "bat_get_media.h"
#include "bat_helper.h"
#include "bat_publishers.h"
#include "static_values.h"

#include "rapidjson_bat_helper.h"

using namespace braveledger_bat_client;
using namespace braveledger_bat_publishers;
using namespace braveledger_bat_get_media;
using namespace std::placeholders;

namespace bat_ledger {

LedgerImpl::LedgerImpl(ledger::LedgerClient* client) :
    ledger_client_(client),
    bat_client_(new BatClient(this)),
    bat_publishers_(new BatPublishers(this)),
    bat_get_media_(new BatGetMedia(this)),
    initialized_(false),
    initializing_(false),
    last_tab_active_time_(0),
    last_shown_tab_id_(-1),
    last_pub_load_timer_id_ (0u){
}

LedgerImpl::~LedgerImpl() {
}

void LedgerImpl::Initialize() {
  DCHECK(!initializing_);
  initializing_ = true;
  LoadLedgerState(this);
}

bool LedgerImpl::CreateWallet() {
  if (initializing_)
    return false;

  initializing_ = true;
  if (initialized_) {
    OnWalletInitialized(ledger::Result::LEDGER_ERROR);
    return false;
  }
  bat_client_->registerPersona();
  return true;
}

void LedgerImpl::AddRecurringPayment(const std::string& publisher_id, const double& value) {
  bat_publishers_->AddRecurringPayment(publisher_id, value);
}

void LedgerImpl::MakePayment(const ledger::PaymentData& payment_data) {
  bat_publishers_->MakePayment(payment_data);
}

void LedgerImpl::OnLoad(const ledger::VisitData& visit_data, const uint64_t& current_time) {
  if (visit_data.domain.empty()) {
    // Skip the same domain name
    return;
  }
  visit_data_iter iter = current_pages_.find(visit_data.tab_id);
  if (iter != current_pages_.end() && iter->second.domain == visit_data.domain) {
    DCHECK(iter == current_pages_.end());
    return;
  }
  if (last_shown_tab_id_ == visit_data.tab_id) {
    last_tab_active_time_ = current_time;
  }
  current_pages_[visit_data.tab_id] = visit_data;
}

void LedgerImpl::OnUnload(uint32_t tab_id, const uint64_t& current_time) {
  //LOG(ERROR) << "!!!LedgerImpl::OnUnload tab_id == " << tab_id;
  OnHide(tab_id, current_time);
  visit_data_iter iter = current_pages_.find(tab_id);
  if (iter != current_pages_.end()) {
    current_pages_.erase(iter);
  }
}

void LedgerImpl::OnShow(uint32_t tab_id, const uint64_t& current_time) {
  //LOG(ERROR) << "!!!LedgerImpl::OnShow tab_id == " << tab_id;
  last_tab_active_time_ = current_time;
  last_shown_tab_id_ = tab_id;
}

void LedgerImpl::OnHide(uint32_t tab_id, const uint64_t& current_time) {
  if (tab_id != last_shown_tab_id_) {
    return;
  }
  visit_data_iter iter = current_pages_.find(tab_id);
  if (iter == current_pages_.end() || 0 == last_tab_active_time_) {
    return;
  }
  //LOG(ERROR) << "!!!LedgerImpl::OnHide tab_id == " << tab_id << ", time == " << (current_time - last_tab_active_time_);
  DCHECK(last_tab_active_time_);
  bat_publishers_->saveVisit(iter->second.tld, iter->second, current_time - last_tab_active_time_);
  last_tab_active_time_ = 0;
}

void LedgerImpl::OnForeground(uint32_t tab_id, const uint64_t& current_time) {
  // TODO media resources could have been played in the background
  if (last_shown_tab_id_ != tab_id) {
    return;
  }
  OnShow(tab_id, current_time);
}

void LedgerImpl::OnBackground(uint32_t tab_id, const uint64_t& current_time) {
  // TODO media resources could stay and be active in the background
  OnHide(tab_id, current_time);
}

void LedgerImpl::OnMediaStart(uint32_t tab_id, const uint64_t& current_time) {
  // TODO
}

void LedgerImpl::OnMediaStop(uint32_t tab_id, const uint64_t& current_time) {
  // TODO
}

void LedgerImpl::OnXHRLoad(
    uint32_t tab_id,
    const std::string& url,
    const std::map<std::string, std::string>& parts,
    const std::string& first_party_url,
    const std::string& referrer,
    const ledger::VisitData& visit_data) {
  std::string type = bat_get_media_->GetLinkType(url, first_party_url, referrer);
  if (type.empty()) {
    // It is not a media supported type
    return;
  }
  bat_get_media_->processMedia(parts, type, visit_data);
}

void LedgerImpl::OnPostData(
      const std::string& url,
      const std::string& first_party_url,
      const std::string& referrer,
      const std::string& post_data,
      const ledger::VisitData& visit_data) {
  std::string type = bat_get_media_->GetLinkType(url, first_party_url, referrer);
  if (type.empty()) {
    // It is not a media supported type
    return;
  }
  //LOG(ERROR) << "!!!LedgerImpl::OnXHRLoad url == " << url;
  LOG(ERROR) << "!!!type == " << type;
  std::vector<std::map<std::string, std::string>> twitchParts;
  if (TWITCH_MEDIA_TYPE == type) {
    braveledger_bat_helper::getTwitchParts(post_data, twitchParts);
    for (size_t i = 0; i < twitchParts.size(); i++) {
      bat_get_media_->processMedia(twitchParts[i], type, visit_data);
    }
  }
}

void LedgerImpl::LoadLedgerState(ledger::LedgerCallbackHandler* handler) {
  ledger_client_->LoadLedgerState(handler);
}

void LedgerImpl::OnLedgerStateLoaded(ledger::Result result,
                                        const std::string& data) {
  if (result == ledger::Result::LEDGER_OK) {
    if (!bat_client_->loadState(data)) {
      result = ledger::Result::INVALID_LEDGER_STATE;
    }
    OnWalletInitialized(result);
  }
  LoadPublisherState(this);
}

void LedgerImpl::LoadPublisherState(ledger::LedgerCallbackHandler* handler) {
  ledger_client_->LoadPublisherState(handler);
}

void LedgerImpl::OnPublisherStateLoaded(ledger::Result result,
                                        const std::string& data) {
  if (result == ledger::Result::LEDGER_OK) {
    if (!bat_publishers_->loadState(data)) {
      result = ledger::Result::INVALID_PUBLISHER_STATE;
    }
  }

  OnWalletInitialized(result);
}

void LedgerImpl::SaveLedgerState(const std::string& data) {
  ledger_client_->SaveLedgerState(data, this);
}

void LedgerImpl::SavePublisherState(const std::string& data,
                                    ledger::LedgerCallbackHandler* handler) {
  ledger_client_->SavePublisherState(data, handler);
}


void LedgerImpl::SavePublishersList(const std::string& data) {
  ledger_client_->SavePublishersList(data, this);
}

void LedgerImpl::LoadPublisherList(ledger::LedgerCallbackHandler* handler) {
  ledger_client_->LoadPublisherList(handler);
}

void LedgerImpl::OnPublisherListLoaded(ledger::Result result,
                                       const std::string& data) {
  if (result == ledger::Result::LEDGER_OK) {
    bat_publishers_->loadPublisherList(data);
  }

  RefreshPublishersList(false);
}

std::string LedgerImpl::GenerateGUID() const {
  return ledger_client_->GenerateGUID();
}

void LedgerImpl::OnWalletInitialized(ledger::Result result) {
  initializing_ = false;
  ledger_client_->OnWalletInitialized(result);

  if (result == ledger::Result::LEDGER_OK) {
    initialized_ = true;
    RefreshPublishersList(false);
  }
}

std::unique_ptr<ledger::LedgerURLLoader> LedgerImpl::LoadURL(const std::string& url,
    const std::vector<std::string>& headers,
    const std::string& content,
    const std::string& contentType,
    const ledger::URL_METHOD& method,
    ledger::LedgerCallbackHandler* handler) {
  return ledger_client_->LoadURL(
      url, headers, content, contentType, method, handler);
}

void LedgerImpl::RunIOTask(LedgerTaskRunnerImpl::Task io_task) {
  std::unique_ptr<LedgerTaskRunnerImpl> task_runner(
      new LedgerTaskRunnerImpl(io_task));
  ledger_client_->RunIOTask(std::move(task_runner));
}

void LedgerImpl::RunTask(LedgerTaskRunnerImpl::Task task) {
  std::unique_ptr<LedgerTaskRunnerImpl> task_runner(
      new LedgerTaskRunnerImpl(task));
  ledger_client_->RunTask(std::move(task_runner));
}

std::string LedgerImpl::URIEncode(const std::string& value) {
  return ledger_client_->URIEncode(value);
}

void LedgerImpl::SetPublisherInfo(std::unique_ptr<ledger::PublisherInfo> info,
                                  ledger::PublisherInfoCallback callback) {
  ledger_client_->SavePublisherInfo(std::move(info),
      std::bind(&LedgerImpl::OnSetPublisherInfo, this, callback, _1, _2));
}

void LedgerImpl::SetMediaPublisherInfo(const std::string& media_key,
                                const std::string& publisher_id) {
  if (!media_key.empty() && !publisher_id.empty()) {
    ledger_client_->SaveMediaPublisherInfo(media_key, publisher_id);
  }
}

void LedgerImpl::SaveMediaVisit(const std::string& publisher_id, const ledger::VisitData& visit_data, const uint64_t& duration) {
  if (bat_publishers_->getPublisherAllowVideos()) {
    bat_publishers_->saveVisit(publisher_id, visit_data, duration);
  }
}

void LedgerImpl::SetPublisherExclude(const std::string& publisher_id, const ledger::PUBLISHER_EXCLUDE& exclude) {
  bat_publishers_->setExclude(publisher_id, exclude);
}

void LedgerImpl::RestorePublishers() {
  bat_publishers_->restorePublishers();
}

void LedgerImpl::OnSetPublisherInfo(ledger::PublisherInfoCallback callback,
                                    ledger::Result result,
                                    std::unique_ptr<ledger::PublisherInfo> info) {
  info = bat_publishers_->onPublisherInfoUpdated(result, std::move(info));
  callback(result, std::move(info));
}

std::vector<ledger::ContributionInfo> LedgerImpl::GetRecurringDonationPublisherInfo() {
  return bat_publishers_->GetRecurringDonationList();
}

void LedgerImpl::GetPublisherInfo(
    const ledger::PublisherInfoFilter& filter,
    ledger::PublisherInfoCallback callback) {
  ledger_client_->LoadPublisherInfo(filter, callback);
}

void LedgerImpl::GetMediaPublisherInfo(const std::string& media_key,
                                ledger::PublisherInfoCallback callback) {
  ledger_client_->LoadMediaPublisherInfo(media_key, callback);
}

void LedgerImpl::GetPublisherInfoList(uint32_t start, uint32_t limit,
                                const ledger::PublisherInfoFilter& filter,
                                ledger::GetPublisherInfoListCallback callback) {
  ledger_client_->LoadPublisherInfoList(start, limit, filter, callback);
}

void LedgerImpl::SetRewardsMainEnabled(bool enabled) {
  bat_client_->setRewardsMainEnabled(enabled);
}

void LedgerImpl::SetPublisherMinVisitTime(uint64_t duration) { // In seconds
  bat_publishers_->setPublisherMinVisitTime(duration);
}

void LedgerImpl::SetPublisherMinVisits(unsigned int visits) {
  bat_publishers_->setPublisherMinVisits(visits);
}

void LedgerImpl::SetPublisherAllowNonVerified(bool allow) {
  bat_publishers_->setPublisherAllowNonVerified(allow);
}

void LedgerImpl::SetPublisherAllowVideos(bool allow) {
  bat_publishers_->setPublisherAllowVideos(allow);
}

void LedgerImpl::SetContributionAmount(double amount) {
  bat_client_->setContributionAmount(amount);
}

void LedgerImpl::SetUserChangedContribution() {
  bat_client_->setUserChangedContribution();
}

void LedgerImpl::SetAutoContribute(bool enabled) {
  bat_client_->setAutoContribute(enabled);
}

bool LedgerImpl::GetRewardsMainEnabled() const {
  return bat_client_->getRewardsMainEnabled();
}

uint64_t LedgerImpl::GetPublisherMinVisitTime() const {
  return bat_publishers_->getPublisherMinVisitTime();
}

unsigned int LedgerImpl::GetPublisherMinVisits() const {
  return bat_publishers_->getPublisherMinVisits();
}

bool LedgerImpl::GetPublisherAllowNonVerified() const {
  return bat_publishers_->getPublisherAllowNonVerified();
}

bool LedgerImpl::GetPublisherAllowVideos() const {
  return bat_publishers_->getPublisherAllowVideos();
}

double LedgerImpl::GetContributionAmount() const {
  return bat_client_->getContributionAmount();
}

bool LedgerImpl::GetAutoContribute() const {
  return bat_client_->getAutoContribute();
}

const std::string& LedgerImpl::GetBATAddress() const {
  return bat_client_->getBATAddress();
}

const std::string& LedgerImpl::GetBTCAddress() const {
  return bat_client_->getBTCAddress();
}

const std::string& LedgerImpl::GetETHAddress() const {
  return bat_client_->getETHAddress();
}

const std::string& LedgerImpl::GetLTCAddress() const {
  return bat_client_->getLTCAddress();
}

uint64_t LedgerImpl::GetReconcileStamp() const {
  return bat_client_->getReconcileStamp();
}

void LedgerImpl::Reconcile() {
  // That function should be triggeres from the main process periodically to make payments
  if (bat_client_->isReadyForReconcile()) {
    bat_client_->reconcile(GenerateGUID());
  }
}

void LedgerImpl::OnReconcileComplete(ledger::Result result,
                                    const std::string& viewing_id) {
  ledger_client_->OnReconcileComplete(result, viewing_id);
  if (result != ledger::Result::LEDGER_OK) {
    // error handling
    return;
  }
  LOG(ERROR) << "!!!in reconcile callback";
  unsigned int ballotsCount = bat_client_->ballots("");
  LOG(ERROR) << "!!!ballotsCount == " << ballotsCount;
  std::vector<braveledger_bat_helper::WINNERS_ST> winners = bat_publishers_->winners(ballotsCount);
  std::vector<std::string> publishers;
  for (size_t i = 0; i < winners.size(); i++) {
    publishers.push_back(winners[i].publisher_data_.id_);
  }
  bat_client_->votePublishers(publishers, viewing_id);
  // TODO call prepareBallots by timeouts like in js library
  bat_client_->prepareBallots();
}

void LedgerImpl::OnWalletProperties(ledger::Result result,
    const braveledger_bat_helper::WALLET_PROPERTIES_ST& properties) {
  std::unique_ptr<ledger::WalletInfo> info;

  if (result == ledger::Result::LEDGER_OK) {
    info.reset(new ledger::WalletInfo);
    info->altcurrency_ = properties.altcurrency_;
    info->probi_ = properties.probi_;
    info->balance_ = properties.balance_;
    info->rates_ = properties.rates_;
    info->parameters_choices_ = properties.parameters_choices_;

    if (!bat_client_->didUserChangeContributionAmount()) {
      info->fee_amount_ = properties.fee_amount_;
    } else if (std::find(info->parameters_choices_.begin(),
      info->parameters_choices_.end(),
      bat_client_->getContributionAmount()) ==
      info->parameters_choices_.end()) {
        info->parameters_choices_.push_back
          (bat_client_->getContributionAmount());
        std::sort(info->parameters_choices_.begin(),
          info->parameters_choices_.end());
    }

    info->parameters_range_ = properties.parameters_range_;
    info->parameters_days_ = properties.parameters_days_;

    for (size_t i = 0; i < properties.grants_.size(); i ++) {
      ledger::Grant grant;

      grant.altcurrency = properties.grants_[i].altcurrency;
      grant.probi = properties.grants_[i].probi;
      grant.expiryTime = properties.grants_[i].expiryTime;

      info->grants_.push_back(grant);
    }
  }

  ledger_client_->OnWalletProperties(result, std::move(info));
}

void LedgerImpl::GetWalletProperties() const {
  bat_client_->getWalletProperties();
}

void LedgerImpl::GetGrant(const std::string& lang,
                              const std::string& payment_id) const {
  bat_client_->getGrant(lang, payment_id);
}

void LedgerImpl::OnGrant(ledger::Result result, const braveledger_bat_helper::GRANT& properties) {
  ledger::Grant grant;

  grant.promotionId = properties.promotionId;

  ledger_client_->OnGrant(result, grant);
}

void LedgerImpl::GetGrantCaptcha() const {
  bat_client_->getGrantCaptcha();
}

void LedgerImpl::OnGrantCaptcha(const std::string& image, const std::string& hint) {
  ledger_client_->OnGrantCaptcha(image, hint);
}

std::string LedgerImpl::GetWalletPassphrase() const {
  return bat_client_->getWalletPassphrase();
}

void LedgerImpl::RecoverWallet(const std::string& passPhrase) const {
  bat_client_->recoverWallet(passPhrase);
}

void LedgerImpl::OnRecoverWallet(ledger::Result result, double balance, const std::vector<braveledger_bat_helper::GRANT>& grants) {
  std::vector<ledger::Grant> ledgerGrants;

  for (size_t i = 0; i < grants.size(); i ++) {
    ledger::Grant tempGrant;

    tempGrant.altcurrency = grants[i].altcurrency;
    tempGrant.probi = grants[i].probi;
    tempGrant.expiryTime = grants[i].expiryTime;

    ledgerGrants.push_back(tempGrant);
  }

  ledger_client_->OnRecoverWallet(result ? ledger::Result::LEDGER_ERROR :
                                          ledger::Result::LEDGER_OK,
                                  balance,
                                  ledgerGrants);
}

void LedgerImpl::SolveGrantCaptcha(const std::string& solution) const {
  bat_client_->setGrant(solution, "");
}

void LedgerImpl::OnGrantFinish(ledger::Result result, const braveledger_bat_helper::GRANT& grant) {
  ledger::Grant newGrant;

  newGrant.altcurrency = grant.altcurrency;
  newGrant.probi = grant.probi;
  newGrant.expiryTime = grant.expiryTime;

  ledger_client_->OnGrantFinish(result, newGrant);
}

bool LedgerImpl::GetBalanceReport(ledger::PUBLISHER_MONTH month,
                                int year,
                                ledger::BalanceReportInfo* report_info) const {
  return bat_publishers_->getBalanceReport(month, year, report_info);
}

std::map<std::string, ledger::BalanceReportInfo> LedgerImpl::GetAllBalanceReports() const {
  return bat_publishers_->getAllBalanceReports();
}

void LedgerImpl::SetBalanceReport(ledger::PUBLISHER_MONTH month,
                                int year,
                                const ledger::BalanceReportInfo& report_info) {
  bat_publishers_->setBalanceReport(month, year, report_info);
}

void LedgerImpl::OnTimer(uint32_t timer_id) {
  if (timer_id == last_pub_load_timer_id_) {
    last_pub_load_timer_id_ = 0;

    //download the list
    std::string url = braveledger_bat_helper::buildURL(GET_PUBLISHERS_LIST_V1, "", braveledger_bat_helper::SERVER_TYPES::PUBLISHER);
    auto url_loader = LoadURL(url, std::vector<std::string>(), "", "", ledger::URL_METHOD::GET, &handler_);
    handler_.AddRequestHandler(std::move(url_loader),
      std::bind(&LedgerImpl::LoadPublishersListCallback,this,_1,_2,_3));
  }
}



void LedgerImpl::LoadPublishersListCallback(bool result, const std::string& response, const std::map<std::string, std::string>& headers) {
  if (result && !response.empty()) {
    //no error so far
    bat_publishers_->RefreshPublishersList(response);
  }
  else {
    //error: retry downloading again
    RefreshPublishersList(true);
  }
}

void LedgerImpl::RefreshPublishersList(bool retryAfterError) {
  uint64_t start_timer_in{ 0ull };

  if (last_pub_load_timer_id_ != 0) {
    //timer in progress
    return;
  }

  if (retryAfterError) {
    std::random_device seeder;
    const auto seed = seeder.entropy() ? seeder() : time(nullptr);
    std::mt19937 eng(static_cast<std::mt19937::result_type> (seed));
    std::uniform_int_distribution <> dist(300, 3600); //retry loading publishers list in 300-3600 seconds if failed
    start_timer_in = dist(eng);
  }
  else {
    uint64_t now = std::time(nullptr);
    uint64_t lastLoadTimestamp = bat_publishers_->getLastPublishersListLoadTimestamp();

    //check if lastLoadTimestamp doesn't exist or have erroneous value.
    //(start_timer_in == 0) is expected to call callback function immediately.

    //time since last succesfull download
    uint64_t  time_since_last_download = (lastLoadTimestamp == 0ull || lastLoadTimestamp > now) ? 0ull : now - lastLoadTimestamp;

    if (now == lastLoadTimestamp) {
      start_timer_in = braveledger_ledger::_publishers_list_load_interval;
    }
    else if (time_since_last_download > 0 && time_since_last_download < braveledger_ledger::_publishers_list_load_interval) {
      start_timer_in = braveledger_ledger::_publishers_list_load_interval - time_since_last_download;
    }
    else {
      start_timer_in = 0ull;
    }
  }

  //start timer
  ledger_client_->SetTimer(start_timer_in, last_pub_load_timer_id_);
}

void LedgerImpl::OnPublishersListSaved(ledger::Result result) {
  bool retryAfterError = (ledger::Result::LEDGER_OK == result) ? false : true;
  bat_publishers_->OnPublishersListSaved(result);
  RefreshPublishersList(retryAfterError);
}

bool LedgerImpl::IsWalletCreated() const {
  return bat_client_->isWalletCreated();
}

}  // namespace bat_ledger
