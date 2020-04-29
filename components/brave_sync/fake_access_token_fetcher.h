#ifndef BRAVE_COMPONENTS_BRAVE_SYNC_FAKE_ACCESS_TOKEN_FETCHER_H_
#define BRAVE_COMPONENTS_BRAVE_SYNC_FAKE_ACCESS_TOKEN_FETCHER_H_

#include "base/memory/weak_ptr.h"
#include "brave/components/brave_sync/access_token_fetcher.h"

namespace brave_sync {
class FakeAccessTokenFetcher : public AccessTokenFetcher {
 public:
   FakeAccessTokenFetcher(AccessTokenConsumer* consumer);
   ~FakeAccessTokenFetcher() override;

  void Start(const std::string& client_id,
                     const std::string& client_secret,
                     const std::string& timestamp) override;

  void StartGetTimestamp() override;

  // Cancels the current request and informs the consumer.
  void CancelRequest() override;
 private:
  void OnGetTokenSuccess(
    const AccessTokenConsumer::TokenResponse& token_response);

  base::WeakPtrFactory<FakeAccessTokenFetcher> weak_ptr_factory_{this};

  DISALLOW_COPY_AND_ASSIGN(FakeAccessTokenFetcher);
};
}   // namespace brave_sync
#endif  // BRAVE_COMPONENTS_BRAVE_SYNC_FAKE_ACCESS_TOKEN_FETCHER_H_
