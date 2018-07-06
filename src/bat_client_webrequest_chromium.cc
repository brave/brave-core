/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat_client_webrequest_chromium.h"

#include "base/logging.h"
#include "bat_helper.h"
#include "chrome/browser/browser_process.h"
#include "content/public/browser/browser_thread.h"
#include "net/base/elements_upload_data_stream.h"
#include "net/base/upload_bytes_element_reader.h"
#include "net/base/upload_data_stream.h"
#include "net/base/upload_element_reader.h"
#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_builder.h"
#include "net/url_request/url_request_context_getter.h"

namespace braveledger_bat_client_webrequest {

  BatClientWebRequestChromium::BatClientWebRequestChromium() : running_(false) {
  }

  BatClientWebRequestChromium::~BatClientWebRequestChromium() {
  }

  BatClientWebRequestChromium::URL_FETCH_REQUEST::URL_FETCH_REQUEST() {}
  BatClientWebRequestChromium::URL_FETCH_REQUEST::~URL_FETCH_REQUEST() {}

  std::unique_ptr<net::UploadDataStream>  BatClientWebRequestChromium::CreateUploadStream(const std::string& stream) {
    std::vector<char> buffer(stream.begin(),stream.end());

    return net::ElementsUploadDataStream::CreateWithReader(
      std::unique_ptr<net::UploadElementReader>(new net::UploadOwnedBytesElementReader(&buffer)), 0);
  }

  void BatClientWebRequestChromium::Start() {
    running_ = true;
  }

  void BatClientWebRequestChromium::Stop() {
    running_ = false;
    url_fetchers_.clear();
  }

  void BatClientWebRequestChromium::runOnThread(const std::string& url,
      FetchCallback callback,
      const std::vector<std::string>& headers,
      const std::string& content, const std::string& contentType,
      const braveledger_bat_helper::FETCH_CALLBACK_EXTRA_DATA_ST& extraData,
      const URL_METHOD& method) {
    if (!running_)
      return;

    LOG(ERROR) << "BatClientWebRequestChromium::runOnThread";
    url_fetchers_.push_back(std::make_unique<URL_FETCH_REQUEST>());
    net::URLFetcher::RequestType requestType = net::URLFetcher::GET;
    switch (method)
    {
      case URL_METHOD::GET:
        requestType = net::URLFetcher::GET;
        break;
      case URL_METHOD::POST:
        requestType = net::URLFetcher::POST;
        break;
      case URL_METHOD::PUT:
        LOG(ERROR) << "!!!in PUT";
        requestType = net::URLFetcher::PUT;
        break;
    }
    /*if (!content.empty()) {
      requestType = net::URLFetcher::POST;
    }*/
    url_fetchers_.back()->url_fetcher_ = net::URLFetcher::Create(GURL(url), requestType, this);
    url_fetchers_.back()->callback_ = callback;
    url_fetchers_.back()->extraData_.reset(new braveledger_bat_helper::FETCH_CALLBACK_EXTRA_DATA_ST(extraData));
    //LOG(ERROR) << "!!!on runOnThread == " + url;
    url_fetchers_.back()->url_fetcher_->SetRequestContext(g_browser_process->system_request_context());
    for (size_t i = 0; i < headers.size(); i++) {
      url_fetchers_.back()->url_fetcher_->AddExtraRequestHeader(headers[i]);
    }
    if (!content.empty()) {
      url_fetchers_.back()->url_fetcher_->SetUploadStreamFactory(
          contentType,
          base::Bind(&BatClientWebRequestChromium::CreateUploadStream, base::Unretained(this), content));
    }
    url_fetchers_.back()->url_fetcher_->Start();
  }

  void BatClientWebRequestChromium::run(const std::string& url,
        FetchCallback callback,
        const std::vector<std::string>& headers, const std::string& content,
        const std::string& contentType, const braveledger_bat_helper::FETCH_CALLBACK_EXTRA_DATA_ST& extraData,
        const URL_METHOD& method) {

    LOG(ERROR) << "!!!web_request URL == " + url;
    content::BrowserThread::PostTask(
      content::BrowserThread::UI, FROM_HERE,
      base::Bind(&BatClientWebRequestChromium::runOnThread,
          base::Unretained(this), url, callback, headers, content, contentType,
          extraData, method));
  }

  void BatClientWebRequestChromium::OnURLFetchComplete(const net::URLFetcher* source) {
    //LOG(ERROR) << "!!!OnURLFetchComplete";
    int response_code = source->GetResponseCode();
    bool failure = response_code == net::URLFetcher::ResponseCode::RESPONSE_CODE_INVALID ||
      !source->GetStatus().is_success();
    if (failure) {
      LOG(ERROR) << "Ledger fetcher HTTP error: " << response_code;
    }
    std::string response;
    source->GetResponseAsString(&response);

    if (url_fetchers_.size()) {
      url_fetchers_.front()->callback_.Run(!failure, response, *url_fetchers_.front()->extraData_);
      url_fetchers_.pop_front();
    }
  }

}  // namespace braveledger_bat_client_webrequest
