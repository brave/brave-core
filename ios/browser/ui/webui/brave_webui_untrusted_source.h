
#include <set>

#include "base/containers/flat_map.h"
#include "ios/web/public/webui/url_data_source_ios.h"
#include "services/network/public/mojom/content_security_policy.mojom.h"

class ChromeBrowserState;

class UntrustedSource : public web::URLDataSourceIOS {
 public:
  explicit UntrustedSource(ChromeBrowserState* browser_state);
  ~UntrustedSource() override;

  UntrustedSource(const UntrustedSource&) = delete;
  UntrustedSource& operator=(const UntrustedSource&) = delete;

  // UntrustedSource implementation
  virtual std::string GetContentSecurityPolicy(
      network::mojom::CSPDirectiveName directive) const;
  virtual void OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName directive,
      const std::string& value);
  virtual void AddFrameAncestor(const GURL& frame_ancestor);
  virtual void DisableTrustedTypesCSP();

  // web::URLDataSourceIOS overrides:
  std::string GetSource() const override;

  std::string GetContentSecurityPolicyObjectSrc() const override;

  std::string GetContentSecurityPolicyFrameSrc() const override;

  void StartDataRequest(const std::string& path,
                        GotDataCallback callback) override;

  std::string GetMimeType(const std::string& path) const override;

  bool AllowCaching() const override;

  bool ShouldDenyXFrameOptions() const override;

  bool ShouldServiceRequest(const GURL& url) const override;

 private:
  ChromeBrowserState* browser_state_;
  base::flat_map<network::mojom::CSPDirectiveName, std::string> csp_overrides_;
  std::set<GURL> frame_ancestors_;
};
