/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat_client_webrequest_chromium.h"
#include "url_fetcher.h"
#include "url_request_context.h"
#include "url_request_context_getter.h"
#include "url_request_context_builder.h"
#include "logging.h"
#include "chrome/browser/browser_process.h"
#include "browser_thread.h"
#include "net/base/upload_data_stream.h"
#include "net/base/upload_element_reader.h"
#include "net/base/elements_upload_data_stream.h"
#include "net/base/upload_bytes_element_reader.h"

namespace bat_client {

namespace braveledger_bat_client_webrequest {  



  BatClientWebRequest::BatClientWebRequest() {
  }

  BatClientWebRequest::~BatClientWebRequest() {
  }

  BatClientWebRequest::URL_FETCH_REQUEST::URL_FETCH_REQUEST(){}
  BatClientWebRequest::URL_FETCH_REQUEST::~URL_FETCH_REQUEST(){}

  std::unique_ptr<net::UploadDataStream>  BatClientWebRequest::CreateUploadStream(const std::string& stream) {
    std::vector<char> buffer(
        stream.begin(),
        stream.end());
    return net::ElementsUploadDataStream::CreateWithReader(
        std::unique_ptr<net::UploadElementReader>(
            new net::UploadOwnedBytesElementReader(&buffer)),
        0);
  }

  void BatClientWebRequest::runOnThread(const std::string& url,
        braveledger_bat_helper::FetchCallback callback, const std::vector<std::string>& headers,
        const std::string& content, const std::string& contentType,
        const braveledger_bat_helper::FETCH_CALLBACK_EXTRA_DATA_ST& extraData, const braveledger_bat_helper::URL_METHOD& method) {

    LOG(ERROR) << "BatClientWebRequest::runOnThread";
    std::lock_guard<std::mutex> guard(fetcher_mutex_);
    url_fetchers_.push_back(std::make_unique<URL_FETCH_REQUEST>());
    net::URLFetcher::RequestType requestType = net::URLFetcher::GET;
    switch (method)
    {
      case braveledger_bat_helper::GET:
        requestType = net::URLFetcher::GET;
        break;
      case braveledger_bat_helper::POST:
        requestType = net::URLFetcher::POST;
        break;
      case braveledger_bat_helper::PUT:
        LOG(ERROR) << "!!!in PUT";
        requestType = net::URLFetcher::PUT;
        break;
    }
    /*if (!content.empty()) {
      requestType = net::URLFetcher::POST;
    }*/
    url_fetchers_.back()->url_fetcher_ = net::URLFetcher::Create(GURL(url), requestType, this);
    url_fetchers_.back()->callback_ = callback;
    url_fetchers_.back()->extraData_ = extraData;
    //LOG(ERROR) << "!!!on runOnThread == " + url;
    url_fetchers_.back()->url_fetcher_->SetRequestContext(g_browser_process->system_request_context());
    for (size_t i = 0; i < headers.size(); i++) {
      url_fetchers_.back()->url_fetcher_->AddExtraRequestHeader(headers[i]);
    }
    if (!content.empty()) {
      url_fetchers_.back()->url_fetcher_->SetUploadStreamFactory(
          contentType,
          base::Bind(&BatClientWebRequest::CreateUploadStream, base::Unretained(this), content));
    }
    url_fetchers_.back()->url_fetcher_->Start();
  }

  void BatClientWebRequest::run(const std::string& url,
        braveledger_bat_helper::FetchCallback callback,
        const std::vector<std::string>& headers, const std::string& content,
        const std::string& contentType, const braveledger_bat_helper::FETCH_CALLBACK_EXTRA_DATA_ST& extraData,
        const braveledger_bat_helper::URL_METHOD& method) {

    LOG(ERROR) << "!!!web_request URL == " + url;
    content::BrowserThread::PostTask(
      content::BrowserThread::UI, FROM_HERE,
      base::Bind(&BatClientWebRequest::runOnThread,
          base::Unretained(this), url, callback, headers, content, contentType,
          extraData, method));
  }

  void BatClientWebRequest::OnURLFetchComplete(const net::URLFetcher* source) {
    //LOG(ERROR) << "!!!OnURLFetchComplete";
    int response_code = source->GetResponseCode();
    bool failure = response_code == net::URLFetcher::ResponseCode::RESPONSE_CODE_INVALID ||
      !source->GetStatus().is_success();
    if (failure) {
      LOG(ERROR) << "Ledger fetcher HTTP error: " << response_code;
    }
    std::string response;
    source->GetResponseAsString(&response);

    std::lock_guard<std::mutex> guard(fetcher_mutex_);
    if (url_fetchers_.size()) {
      url_fetchers_.front()->callback_.Run(!failure, response, url_fetchers_.front()->extraData_);
      url_fetchers_.pop_front();
    }
  }

}
