#ifndef BRAVE_BNES_BNS_RESOLVER_H_
#define BRAVE_BNES_BNS_RESOLVER_H_

#include <memory>
#include <string>

#include "base/functional/callback.h"
#include "url/gurl.h"

namespace content {
class BrowserContext;
}  // namespace content

namespace bns {

struct ResolveResult {
  GURL gateway_url;
  std::string contenthash;
  std::string cid;
};

using ResolveCallback = base::OnceCallback<void(std::optional<ResolveResult>)>;

class BnsResolver {
 public:
  explicit BnsResolver(content::BrowserContext* browser_context);
  ~BnsResolver() = default;

  void Resolve(const GURL& url, ResolveCallback callback);

 private:
  void OnRpcResponse(ResolveCallback callback,
                     std::optional<base::Value> result,
                     int response_code,
                     int error_code);

  bool ParseContenthash(const base::Value& value,
                        std::string* out_contenthash);
  bool ValidateAndBuildGateway(const std::string& contenthash,
                               ResolveResult* out_result);

  raw_ptr<content::BrowserContext> browser_context_;
  std::unique_ptr<api_request_helper::APIRequestHelper> api_request_helper_;
};

}  // namespace bns

#endif  // BRAVE_BNES_BNS_RESOLVER_H_
