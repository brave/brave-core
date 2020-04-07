#ifndef BRAVE_COMPONENTS_BRAVE_SYNC_ACCESS_TOKEN_FETCHER_IMPL_H_
#define BRAVE_COMPONENTS_BRAVE_SYNC_ACCESS_TOKEN_FETCHER_IMPL_H_

#include <memory>
#include <string>
#include <vector>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "brave/components/brave_sync/access_token_fetcher.h"
#include "brave/components/brave_sync/access_token_consumer.h"
#include "url/gurl.h"

namespace network {
class SimpleURLLoader;
class SharedURLLoaderFactory;
}

namespace brave_sync {

class AccessTokenFetcherImpl : public AccessTokenFetcher {
 public:
  AccessTokenFetcherImpl(
      AccessTokenConsumer* consumer,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      const std::string& refresh_token);
  ~AccessTokenFetcherImpl() override;

  // Implementation of AccessTokenFetcher
  void Start(const std::string& client_id,
             const std::string& client_secret,
             const std::string& timestamp) override;

  void StartGetTimestamp() override;

  void CancelRequest() override;

 private:
  enum State {
    INITIAL,
    GET_ACCESS_TOKEN_STARTED,
    GET_ACCESS_TOKEN_DONE,
    ERROR_STATE,
  };

  void OnURLLoadComplete(std::unique_ptr<std::string> response_body);
  void OnTimestampLoadComplete(std::unique_ptr<std::string> response_body);

  // Helper methods for the flow.
  void StartGetAccessToken();
  void EndGetAccessToken(std::unique_ptr<std::string> response_body);

  // Helper mehtods for reporting back results.
  void OnGetTokenSuccess(
      const AccessTokenConsumer::TokenResponse& token_response);
  void OnGetTokenFailure(const std::string& error);

  // Other helpers.
  static GURL MakeGetAccessTokenUrl();
  static std::string MakeGetAccessTokenBody(
      const std::string& client_id,
      const std::string& client_secret,
      const std::string& timestamp,
      const std::string& refresh_token);

  static bool ParseGetAccessTokenSuccessResponse(
      std::unique_ptr<std::string> response_body,
      std::string* access_token,
      int* expires_in,
      std::string* id_token);

  static bool ParseGetAccessTokenFailureResponse(
      std::unique_ptr<std::string> response_body,
      std::string* error);

  // State that is set during construction.
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  const std::string refresh_token_;
  State state_;

  // While a fetch is in progress.
  std::unique_ptr<network::SimpleURLLoader> url_loader_;
  std::unique_ptr<network::SimpleURLLoader> ts_url_loader_;
  std::string client_id_;
  std::string client_secret_;
  std::string timestamp_;

  DISALLOW_COPY_AND_ASSIGN(AccessTokenFetcherImpl);
};
}   // namespace brave_sync
#endif  // BRAVE_COMPONENTS_BRAVE_SYNC_ACCESS_TOKEN_FETCHER_IMPL_H_
