/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/browser/dat_file_web_request.h"

#include "base/files/file_path.h"
#include "brave/components/brave_shields/browser/dat_file_util.h"
#include "chrome/browser/browser_process.h"
#include "content/public/browser/browser_thread.h"
#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_request_context.h"

using content::BrowserThread;

namespace brave_shields {

DATFileWebRequest::DATFileWebRequest(
    const std::string &dat_file_name,
    const GURL& url,
    DATFileWebRequest::FetchCallback fetch_callback)
    : url_(url),
    fetch_callback_(fetch_callback), dat_file_name_(dat_file_name) {
  Init();
}

DATFileWebRequest::~DATFileWebRequest() {
}

void DATFileWebRequest::Init() {
  if (url_fetcher_)
    return;

  net::HttpRequestHeaders headers;
  url_fetcher_ = net::URLFetcher::Create(url_, net::URLFetcher::GET, this);
  url_fetcher_->SetRequestContext(g_browser_process->system_request_context());
  // url_fetcher_->SetLoadFlags(load_flags);
  url_fetcher_->AddExtraRequestHeader(
      "Cache-Control: no-cache, no-store, must-revalidate");
  url_fetcher_->AddExtraRequestHeader("Pragma: no-cache");
  url_fetcher_->AddExtraRequestHeader("Expires: 0");
  base::FilePath dat_file_path = GetDATFilePath(dat_file_name_);
  url_fetcher_->SaveResponseToFileAtPath(
    dat_file_path,
    BrowserThread::GetTaskRunnerForThread(BrowserThread::FILE));
}

void DATFileWebRequest::OnURLFetchComplete(
    const net::URLFetcher* source) {
  int response_code = source->GetResponseCode();
  if (response_code == net::URLFetcher::ResponseCode::RESPONSE_CODE_INVALID ||
      !source->GetStatus().is_success()) {
    fetch_callback_.Run(false);
  } else {
    fetch_callback_.Run(true);
  }
}

void DATFileWebRequest::Start() {
  if (url_fetcher_)
    url_fetcher_->Start();
}

}  // namespace brave_shields
