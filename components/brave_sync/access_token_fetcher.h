#ifndef BRAVE_COMPONENTS_BRAVE_SYNC_ACCESS_TOKEN_FETCHER_H_
#define BRAVE_COMPONENTS_BRAVE_SYNC_ACCESS_TOKEN_FETCHER_H_

#include "base/macros.h"
#include "base/time/time.h"
#include "brave/components/brave_sync/access_token_consumer.h"

namespace brave_sync {

class AccessTokenFetcher {
 public:
   explicit AccessTokenFetcher(AccessTokenConsumer* consumer);
   virtual ~AccessTokenFetcher();

  virtual void Start(const std::string& client_id,
                     const std::string& client_secret) = 0;

  // Cancels the current request and informs the consumer.
  virtual void CancelRequest() = 0;

 protected:
  // Fires |OnGetTokenSuccess| on |consumer_|.
  void FireOnGetTokenSuccess(
      const AccessTokenConsumer::TokenResponse& token_response);

  // Fires |OnGetTokenFailure| on |consumer_|.
  void FireOnGetTokenFailure(const std::string& error);
 private:

  AccessTokenConsumer* const consumer_;

  DISALLOW_COPY_AND_ASSIGN(AccessTokenFetcher);
};

}   // namespace brave_sync
#endif  // BRAVE_COMPONENTS_BRAVE_SYNC_ACCESS_TOKEN_FETCHER_H_
