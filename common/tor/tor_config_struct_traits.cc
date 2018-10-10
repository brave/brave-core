/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/ */

#include "brave/common/tor/tor_config_struct_traits.h"

#include "mojo/public/cpp/base/file_path_mojom_traits.h"

namespace mojo {

// static
bool StructTraits<tor::mojom::TorConfigDataView,
                  tor::TorConfig>::
    Read(tor::mojom::TorConfigDataView in,
         tor::TorConfig* out) {
    base::FilePath binary_path;
    std::string proxy_string;
    if (!in.ReadBinaryPath(&binary_path) || !in.ReadProxyString(&proxy_string))
      return false;

    *out = tor::TorConfig(binary_path, proxy_string);
    if (out->empty())
      return false;
    return true;
}

}  // namespace mojo
