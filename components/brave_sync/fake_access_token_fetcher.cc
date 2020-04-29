#include "brave/components/brave_sync/fake_access_token_fetcher.h"

#include "base/threading/thread_task_runner_handle.h"
#include "base/time/time.h"

namespace brave_sync {

FakeAccessTokenFetcher::FakeAccessTokenFetcher(AccessTokenConsumer* consumer)
    : AccessTokenFetcher(consumer) {}

FakeAccessTokenFetcher::~FakeAccessTokenFetcher() {}

void FakeAccessTokenFetcher::Start(const std::string& client_id,
                                   const std::string& client_secret,
                                   const std::string& timestamp) {
  AccessTokenConsumer::TokenResponse response(
      "dummpy_access_token",
      base::Time::Now() + base::TimeDelta::FromSeconds(60), "dummy_id_token");
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(&FakeAccessTokenFetcher::OnGetTokenSuccess,
                                weak_ptr_factory_.GetWeakPtr(), response));
}
void FakeAccessTokenFetcher::OnGetTokenSuccess(
    const AccessTokenConsumer::TokenResponse& token_response) {
  FireOnGetTokenSuccess(token_response);
}

void FakeAccessTokenFetcher::StartGetTimestamp() {
  FireOnGetTimestampSuccess("dummy_timestamp");
}

void FakeAccessTokenFetcher::CancelRequest() {}

}  // namespace brave_sync
