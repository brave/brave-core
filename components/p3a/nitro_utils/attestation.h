/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_P3A_NITRO_UTILS_ATTESTATION_H_
#define BRAVE_COMPONENTS_P3A_NITRO_UTILS_ATTESTATION_H_

#include <vector>

#include "base/functional/callback.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "url/gurl.h"

namespace net {
class X509Certificate;
}  // namespace net

namespace nitro_utils {

// Requests a Nitro Enclave attestation document for a given URL,
// parses the COSE payload, and verifies the authenticity of the document.
// An X509 certificate will be provided as a result of successful
// attestation verification.
void RequestAndVerifyAttestationDocument(
    const GURL& attestation_url,
    network::mojom::URLLoaderFactory* url_loader_factory,
    base::OnceCallback<void(scoped_refptr<net::X509Certificate>)>
        result_callback);

// Functions used for unit tests
//
// These take the cbor serialization of just the attestation document
// portion of the COSE Sign1 object returned by the AWS Nitro enclave
// in response to remote attestation requests.
//
// Verify the nonce value passed with the attestation request.
bool VerifyNonceForTesting(const std::vector<uint8_t>& attestation_bytes,
                           const std::vector<uint8_t>& orig_nonce);
// Verify the TLS certificate fingerprint returned with the attestation request.
bool VerifyUserDataKeyForTesting(
    const std::vector<uint8_t>& attestation_bytes,
    scoped_refptr<net::X509Certificate> server_cert);

}  // namespace nitro_utils

#endif  // BRAVE_COMPONENTS_P3A_NITRO_UTILS_ATTESTATION_H_
