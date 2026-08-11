#include "brave/bnes/bns_security.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/self_deleting_url_loader_factory.h"
#include "services/network/public/mojom/url_loader.mojom.h"
#include "services/network/public/mojom/url_response_head.mojom.h"
#include "url/gurl.h"

namespace content {
class BrowserContext;
}  // namespace content

namespace bns {

class BnesURLLoaderFactory : public network::SelfDeletingURLLoaderFactory {
 public:
  static mojo::PendingRemote<network::mojom::URLLoaderFactory> Create(
      content::BrowserContext* browser_context);

  BnesURLLoaderFactory(
      content::BrowserContext* browser_context,
      mojo::PendingReceiver<network::mojom::URLLoaderFactory> factory_receiver,
      base::SelfDeletingPassKey key);

 private:
  ~BnesURLLoaderFactory() override = default;

  void CreateLoaderAndStart(
      mojo::PendingReceiver<network::mojom::URLLoader> loader,
      int32_t request_id,
      uint32_t options,
      const network::ResourceRequest& request,
      mojo::PendingRemote<network::mojom::URLLoaderClient> client,
      const net::MutableNetworkTrafficAnnotationTag& traffic_annotation)
      override;

  raw_ptr<content::BrowserContext> browser_context_;
};

inline constexpr std::string_view kBnesScheme = "bnes";

BnesURLLoaderFactory::BnesURLLoaderFactory(
    content::BrowserContext* browser_context,
    mojo::PendingReceiver<network::mojom::URLLoaderFactory> factory_receiver,
    base::SelfDeletingPassKey key)
    : network::SelfDeletingURLLoaderFactory(std::move(factory_receiver), key),
      browser_context_(browser_context) {}

mojo::PendingRemote<network::mojom::URLLoaderFactory>
BnesURLLoaderFactory::Create(content::BrowserContext* browser_context) {
  mojo::PendingRemote<network::mojom::URLLoaderFactory> pending_remote;
  base::MakeSelfDeleting<BnesURLLoaderFactory>(
      browser_context, pending_remote.InitWithNewPipeAndPassReceiver());
  return pending_remote;
}

void BnesURLLoaderFactory::CreateLoaderAndStart(
    mojo::PendingReceiver<network::mojom::URLLoader> loader,
    int32_t request_id,
    uint32_t options,
    const network::ResourceRequest& request,
    mojo::PendingRemote<network::mojom::URLLoaderClient> client,
    const net::MutableNetworkTrafficAnnotationTag& traffic_annotation) {
  // [BNES] Route bnes:// requests through resolver pipeline
  if (!IsAllowedNavigationUrl(request.url)) {
    mojo::Remote<network::mojom::URLLoaderClient> std_client(
        std::move(client));
    std_client->OnComplete(
        network::URLLoaderCompletionStatus(net::ERR_INVALID_URL));
    return;
  }
  // TODO: ResolveBnesContent(browser_context_, request.url);
}

bool IsBnesScheme(const GURL& url) {
  return url.is_valid() && url.SchemeIs(kBnesScheme);
}

}  // namespace bns
