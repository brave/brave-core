#ifndef BRAVE_COMPONENTS_BRAVE_SYNC_ACCESS_TOKEN_CONSUMER_H_
#define BRAVE_COMPONENTS_BRAVE_SYNC_ACCESS_TOKEN_CONSUMER_H_

#include "base/time/time.h"

namespace brave_sync {

class AccessTokenConsumer {
 public:
  // Structure representing information contained in access token.
  struct TokenResponse {
    TokenResponse() = default;
    TokenResponse(const std::string& access_token,
                  const base::Time& expiration_time,
                  const std::string& id_token);

    // access token.
    std::string access_token;

    // The date until which the |access_token| can be used.
    // This value has a built-in safety margin, so it can be used as-is.
    base::Time expiration_time;

    // Contains extra information regarding the user's currently registered
    // services.
    std::string id_token;
  };

  AccessTokenConsumer() = default;
  virtual ~AccessTokenConsumer();

  // Success callback.
  virtual void OnGetTokenSuccess(const TokenResponse& token_response);

  // Failure callback.
  // TODO(darkdh): define error messages
  virtual void OnGetTokenFailure(const std::string& error);

  DISALLOW_COPY_AND_ASSIGN(AccessTokenConsumer);
};

}   // namespace brave_sync
#endif  // BRAVE_COMPONENTS_BRAVE_SYNC_ACCESS_TOKEN_CONSUMER_H_
