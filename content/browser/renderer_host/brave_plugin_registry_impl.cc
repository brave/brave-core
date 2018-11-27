/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/content/browser/renderer_host/brave_plugin_registry_impl.h"

#include "base/strings/utf_string_conversions.h"
#include "content/public/common/content_constants.h"

namespace content {

BravePluginRegistryImpl::BravePluginRegistryImpl(ResourceContext* resource_context)
    : PluginRegistryImpl(resource_context) {}

BravePluginRegistryImpl::~BravePluginRegistryImpl() {}

void BravePluginRegistryImpl::GetPluginsComplete(GetPluginsCallback callback, 
  std::vector<blink::mojom::PluginInfoPtr> plugins) {
  std::vector<blink::mojom::PluginInfoPtr> filtered_plugins;

  for(std::vector<blink::mojom::PluginInfoPtr>::iterator it = plugins.begin(); 
    it != plugins.end(); ++it) {
    if ((*it) && (*it)->name == base::ASCIIToUTF16(kFlashPluginName)) {
      filtered_plugins.push_back(std::move(*it));
    }
  }
  std::move(callback).Run(std::move(filtered_plugins));
}

void BravePluginRegistryImpl::GetPlugins(bool refresh,
                                    const url::Origin& main_frame_origin,
                                    GetPluginsCallback callback) {
  PluginRegistryImpl::GetPlugins(refresh, main_frame_origin,
    base::BindOnce(
      &BravePluginRegistryImpl::GetPluginsComplete, base::Unretained(this), 
      std::move(callback)));
}

}  // namespace content