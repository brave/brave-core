#ifndef BRAVE_CONTENT_BROWSER_WEBUI_BRAVE_SHARED_RESOURCES_DATA_SOURCE_H_
#define BRAVE_CONTENT_BROWSER_WEBUI_BRAVE_SHARED_RESOURCES_DATA_SOURCE_H_

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/single_thread_task_runner.h"
#include "content/public/browser/url_data_source.h"

namespace brave_content {

using namespace content;

// A DataSource for chrome://brave-resources/ URLs.
class BraveSharedResourcesDataSource : public URLDataSource {
 public:
  BraveSharedResourcesDataSource();
  ~BraveSharedResourcesDataSource() override;

  // URLDataSource implementation.
  std::string GetSource() const override;
  void StartDataRequest(
      const std::string& path,
      const ResourceRequestInfo::WebContentsGetter& wc_getter,
      const URLDataSource::GotDataCallback& callback) override;
  bool AllowCaching() const override;
  std::string GetMimeType(const std::string& path) const override;
  bool ShouldServeMimeTypeAsContentTypeHeader() const override;
  scoped_refptr<base::SingleThreadTaskRunner> TaskRunnerForRequestPath(
      const std::string& path) const override;
  std::string GetAccessControlAllowOriginForOrigin(
      const std::string& origin) const override;
  bool IsGzipped(const std::string& path) const override;

 private:
  DISALLOW_COPY_AND_ASSIGN(BraveSharedResourcesDataSource);
};

}  // namespace brave_content

#endif
