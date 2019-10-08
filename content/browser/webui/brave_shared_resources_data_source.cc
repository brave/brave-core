// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "brave/content/browser/webui/brave_shared_resources_data_source.h"

#include <stddef.h>

#include <map>
#include <string>
#include <unordered_map>
#include <utility>

#include "base/feature_list.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/memory/ref_counted_memory.h"
#include "base/strings/string_piece.h"
#include "base/strings/string_util.h"
#include "base/task/post_task.h"
#include "build/build_config.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/common/content_client.h"
#include "content/public/common/content_features.h"
#include "content/public/common/url_constants.h"
#include "ui/base/layout.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/webui/web_ui_util.h"
#include "brave/ui/webui/resources/grit/brave_webui_resources.h"
#include "brave/ui/webui/resources/grit/brave_webui_resources_map.h"

#if defined(OS_WIN)
#include "base/strings/utf_string_conversions.h"
#endif

namespace brave_content {

namespace {

using ResourcesMap = std::unordered_map<std::string, int>;

const std::map<std::string, std::string> CreatePathPrefixAliasesMap() {
  // Map of GRD-relative path prefixes to incoming request path, e.g.
  // brave://brave-resources/blah/X could be mapped to
  // GRD:../../resources/web/blah/X.
  // See chromium's SharedResourcesDataSource for an example
  // Format is {"../../somewhere/in/src/tree/", "request/path" }
  std::map<std::string, std::string> aliases = { };
  return aliases;
}

void AddResource(const std::string& path,
                 int resource_id,
                 ResourcesMap* resources_map) {
  if (!resources_map->insert(std::make_pair(path, resource_id)).second)
    NOTREACHED() << "Redefinition of '" << path << "'";
}

void AddResourcesToMap(ResourcesMap* resources_map) {
  const std::map<std::string, std::string> aliases =
      CreatePathPrefixAliasesMap();

  for (size_t i = 0; i < kBraveWebuiResourcesSize; ++i) {
    const auto& resource = kBraveWebuiResources[i];

    AddResource(resource.name, resource.value, resources_map);

    for (auto it = aliases.begin(); it != aliases.end(); ++it) {
      if (base::StartsWith(resource.name, it->first,
                           base::CompareCase::SENSITIVE)) {
        std::string resource_name(resource.name);
        AddResource(it->second + resource_name.substr(it->first.length()),
                    resource.value, resources_map);
      }
    }
  }
}

const ResourcesMap* CreateResourcesMap() {
  ResourcesMap* result = new ResourcesMap();
  AddResourcesToMap(result);
  return result;
}

const ResourcesMap& GetResourcesMap() {
  // This pointer will be intentionally leaked on shutdown.
  static const ResourcesMap* resources_map = CreateResourcesMap();
  return *resources_map;
}

int GetIdrForPath(const std::string& path) {
  const ResourcesMap& resources_map = GetResourcesMap();
  auto it = resources_map.find(path);
  return it != resources_map.end() ? it->second : -1;
}

}  // namespace

BraveSharedResourcesDataSource::BraveSharedResourcesDataSource() {
}

BraveSharedResourcesDataSource::~BraveSharedResourcesDataSource() {
}

std::string BraveSharedResourcesDataSource::GetSource() {
  return "brave-resources";
}

void BraveSharedResourcesDataSource::StartDataRequest(
    const std::string& path,
    const content::WebContents::Getter& wc_getter,
    const content::URLDataSource::GotDataCallback& callback) {
  int idr = GetIdrForPath(path);
  DCHECK_NE(-1, idr) << " path: " << path;
  scoped_refptr<base::RefCountedMemory> bytes;

  // Cannot access GetContentClient() from here as that is //content/public
  // only. Therefore, cannot access ContentClient::GetDataResourceBytes,
  // so go to the bundle directly. This will work for all content clients apart
  // from in a test environment, where this shoudl be mocked.
  bytes = ui::ResourceBundle::GetSharedInstance().LoadDataResourceBytes(idr);
  callback.Run(bytes.get());
}

bool BraveSharedResourcesDataSource::AllowCaching() {
  // Should not be cached to reflect dynamically-generated contents that may
  // depend on the current locale.
  return true;
}

std::string BraveSharedResourcesDataSource::GetMimeType(
    const std::string& path) {
  if (path.empty())
    return "text/html";

#if defined(OS_WIN)
  base::FilePath file(base::UTF8ToWide(path));
  std::string extension = base::WideToUTF8(file.FinalExtension());
#else
  base::FilePath file(path);
  std::string extension = file.FinalExtension();
#endif

  if (!extension.empty())
    extension.erase(0, 1);

  if (extension == "html")
    return "text/html";

  if (extension == "css")
    return "text/css";

  if (extension == "js")
    return "application/javascript";

  if (extension == "png")
    return "image/png";

  if (extension == "gif")
    return "image/gif";

  if (extension == "svg")
    return "image/svg+xml";

  if (extension == "woff2")
    return "application/font-woff2";

  NOTREACHED() << path;
  return "text/plain";
}

bool BraveSharedResourcesDataSource::ShouldServeMimeTypeAsContentTypeHeader() {
  return true;
}

scoped_refptr<base::SingleThreadTaskRunner>
BraveSharedResourcesDataSource::TaskRunnerForRequestPath(
    const std::string& path) {
  return nullptr;
}

std::string
BraveSharedResourcesDataSource::GetAccessControlAllowOriginForOrigin(
    const std::string& origin) {
  // For now we give access only for "chrome://*" origins.
  // According to CORS spec, Access-Control-Allow-Origin header doesn't support
  // wildcards, so we need to set its value explicitly by passing the |origin|
  // back.
  std::string allowed_origin_prefix = content::kChromeUIScheme;
  allowed_origin_prefix += "://";
  if (!base::StartsWith(origin, allowed_origin_prefix,
                        base::CompareCase::SENSITIVE)) {
    return "null";
  }
  return origin;
}

bool BraveSharedResourcesDataSource::IsGzipped(const std::string& path) {
  // Cannot access GetContentClient() from here as that is //content/public
  // only. Therefore cannot access ContentClient::IsDataResourceGzipped, so go
  // to the bundle directly.
  return ui::ResourceBundle::GetSharedInstance().IsGzipped(GetIdrForPath(path));
}

}  // namespace brave_content
