/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_NITRO_UTILS_ATTESTATION_H_
#define BRAVE_COMPONENTS_NITRO_UTILS_ATTESTATION_H_

#include "base/callback.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "url/gurl.h"

namespace net {
class X509Certificate;
}  // namespace net

namespace nitro_utils {

void RequestAndVerifyAttestationDocument(
    const GURL& attestation_url,
    network::mojom::URLLoaderFactory* url_loader_factory,
    base::OnceCallback<void(scoped_refptr<net::X509Certificate>)>
        result_callback);

}  // namespace nitro_utils

#endif  // BRAVE_COMPONENTS_NITRO_UTILS_ATTESTATION_H_
