/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/utility/refill_confirmation_tokens/refill_confirmation_tokens_test_util.h"

namespace brave_ads::test {

std::string BuildRequestSignedTokensUrlResponseBody() {
  return R"(
      {
        "nonce": "2f0e2891-e7a5-4262-835b-550b13e58e5c"
      })";
}

std::string BuildGetSignedTokensUrlResponseBody() {
  return R"(
      {
        "batchProof": "zFKE/klGjgSJjgrc74uzy2cdc0dXPD3WTIbxraoQegTVi+Q0/YpI1olCDXoLf6FPMPwktEt1e3YWaMFlXn0vCw==",
        "signedTokens": [
          "Ktsii4pOKOjfmqziUsdlV3lJETaGXG0rksRujn1i1Qs=",
          "Mv5M6UhufCEmF7u9pBaGQJAckuwpkSpOJvJq1F+y9VE=",
          "MpysVJMYUI4n0NcY2CdZyZxQxADOHJal1pZI2uPiqWs=",
          "5stC+5v3eo3tADEkTtxESlCBIkoYI8aRDLJ8VVtcLyE=",
          "MBSxmUfpp5p8FI+86A4+0zFYw5dPhre1YYw+sRFWkTs=",
          "TPtzCHdDbaW3vAApNFc6nyC806fmdNqG9HaoZMQWGx8=",
          "dt0Xmblg7vU0TEzE4yaBtNnQ8igZJcdlNj1CDVxXVHA=",
          "RBJGJT9Pf2T2yzi+hfPXnXIjxZ6WbJ7UhZ1f1BGDpHc=",
          "NN/FZ/sbjvhGAKzBQdtUrbjkDuCKbpwNCT3cKURvzB8=",
          "BCoCHDIFRVyZqgGfB1SJpTsHa8tJ9VFOq0PUPubLBk8=",
          "EAjQxthZt6mR2+wgMV35B4Bq61e16PCyX5JkhRwdxXM=",
          "3LPp0kUrPjeKVkBFJSOL9b7/qjfUCc8ylqZyjcSFhVs=",
          "zpjmculV506uDhkXJ4k6lCCFhVja5EkA2uXkefE3ZlY=",
          "nEzbQkRVPl6H9+AF/OBcfLf61o1xO/jbB+7a1amJ3Eg=",
          "NgSQDmucmraSK8QZve64DvMv1l01jQEpK9RjSR7J91Q=",
          "vCB5ksvHTXCYtbCdBe+xynPo3T6ybVgLfHf0Od8PEEQ=",
          "PlkLA8DFzpMQQaCsKICK7zxDRJ10YEdFGvFFfx9Fn08=",
          "jm2usDGsU+/KSy2+6Q8X5cwhFr7pZ+tJ6GRCue2ywl0=",
          "SD8rfdIV3Hp3PbiQ29KmNN4eYd0kMA6fVr4ZKb6McRg=",
          "DKPADls0HZ0m0MtQb2qk5A/w/bUWAgf5gQI+F/9JMQQ=",
          "5phPdMP/HWQCMGkXvgW8BDLgKZRTni2eQft8oDOlUhY=",
          "LDkKHokr0ydfPuj3cFaXsI1Il1pFEZUrX3G2IM30Il8=",
          "FFbRg7fgraPLTOBKkiepCl0ukp6fYHrh/E715Gsxn0Q=",
          "aNBW1Qn9y0WejDJJzU/4FmfBnfOipeDOD0qo2u8u2xQ=",
          "KGIhrGVhTah7Liwgphh6EidivlOVKbnWMQRrKItcnVY=",
          "Ag+u6UoZ/AJnVmLH659K8zLlaFVgEV4TSWiCIo/1RGk=",
          "Bjsr97vTdcPRFb8rVZ96yGKGEWqBRobalK09hSjnckk=",
          "TLdnTFqBqDBI4LhGQrQLFIP9hYBXjvRIAj58YuCkrG8=",
          "nLTmiuYHyKudXYxXSwT5fOLmIJOiuB5nsrvzRr+STlI=",
          "sBYEmkom7ay5PR0JI4J9pohdAsT00BneDcBlk9XJAXc=",
          "loZ/wr35u1XaMQXPi2KDCXbMoqCR4qSrihf8hGFGFnk=",
          "Ethq6jJOlvciMq9CxnvCvPiS6dpQTSJAfGG3JAMTcn0=",
          "uKGcjL8+ja5g+kg1GXmoKqB1UxsJjHFYyJsErIxRTDI=",
          "zGNHYNpUX9fdDENSlT5a0MFzssn9nJwRs9Fh5NM1GDw=",
          "xHnOdiSKGvOr7qzIXgP0epsFPy0STQy2+6nd6NZEeXo=",
          "wuFVfI9geQ+U+R7LxM+SFiz15Fj7rpIRMOMNSt43PAM=",
          "agMSIk+buHMb4qUc0veGzAUXdcVkHqc+Oy+Zt7IOKzA=",
          "mj7RPYebPDz859w61Ce5HEQh3NGQ1xYyp5xthZtpym8=",
          "jkmYEUw+dU8ypn03nXl6Fj+OzctOLlG8CSYEI15zNAg=",
          "UAuc8dnFor4mva7EGD5bV/nqZz8wq2/RAte8r0iuNUY=",
          "9OxHQt1/VR9JpMB4XrC0Yf7MScmHANkpSzcb1XujtkM=",
          "Mv5V7+I0BBwNrVYf/012S+esEnqh3gAXZtlE8ClbOg0=",
          "doMba7NOQ+4doCwCQdBGWbvI+qofpgNr1VHqrM8vMCw=",
          "QIl+CO4G3g6Tbfre2g6TOkKOgZ4TgIoXeOoRqSV4jGU=",
          "LBjAPlawmwRldmRDpKk0wJRQBpkNqhzDiDNBNYmzg1E=",
          "QlKUaOO/znQjVtwXNGiwUDrV+GW3/a9wZXvvjvib+3I=",
          "CAHB+5LsyE/ntr8d4Ll1eFnowotytI+ZksaSdiiIa00=",
          "PorTCjYCypszf7/fs4eXgrgV4yi/3qIEmffYmaJ9hXI=",
          "Gt95JWkzrdcSw41DCdoTs8umsC7CzH1y0rJ7m/2vGX4=",
          "IjtwdzooaSvEr/V+e5vESRhUoJgvLnaqdbTL+dnKyR4="
        ],
        "publicKey": "OqhZpUC8B15u+Gc11rQYRl8O3zOSAUIEC2JuDHI32TM="
      })";
}

}  // namespace brave_ads::test
