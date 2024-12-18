/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_CHALLENGE_BYPASS_RISTRETTO_CHALLENGE_BYPASS_RISTRETTO_TEST_CONSTANTS_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_CHALLENGE_BYPASS_RISTRETTO_CHALLENGE_BYPASS_RISTRETTO_TEST_CONSTANTS_H_

namespace brave_ads::cbr::test {

inline constexpr char kInvalidBase64[] = "INVALID";

inline constexpr char kSigningKeyBase64[] =
    R"(qsA+Hif/fQ3fNLMa37qbCLJjlzauLaqbjFNff/PkFAY=)";

inline constexpr char kPublicKeyBase64[] =
    R"(QnShwT9vRebch3WDu28nqlTaNCU5MaOF1n4VV4Q3K1g=)";

inline constexpr char kTokenBase64[] =
    R"(IXDCnZnVEJ0orkbZfr2ut2NQPQ0ofdervKBmQ2hyjcClGCjA3/ExbBumO0ua5cxwo//nN0uKQ60kknru8hRXx0FMYHu7ahizDiX18HVBawWBzA46pyrBtJDfiomK6HQI)";

inline constexpr char kBlindedTokenBase64[] =
    R"(mNRViqFD8ZPpjRZi4Xwj1UEsU1j9qPNc4R/BoiWsVi0=)";

inline constexpr char kSignedTokenBase64[] =
    R"(jGZR7JREp+zoqxgsMOa32F+zhhBw/0d/HeVhl9iPVWU=)";

inline constexpr char kBatchDLEQProofBase64[] =
    R"(y0a409PTX6g97xC0Xq8cCuY7ElLPaP+QJ6DaHNfqlQWizBYCSWdaleakKatkyNswfPmkQuhL7awmzQ0ygEUGDw==)";

inline constexpr char kUnblindedTokenBase64[] =
    R"(IXDCnZnVEJ0orkbZfr2ut2NQPQ0ofdervKBmQ2hyjcClGCjA3/ExbBumO0ua5cxwo//nN0uKQ60kknru8hRXx0DWhwHwuFlxmot8WgVbnQ0XtPx7q9BG0jbI00AJStwN)";

inline constexpr char kVerificationSignatureBase64[] =
    R"(V7Gilxl5TNv7pTqq8Sftmu+O+HgJ44Byn8PhDkDIwnsgncGiCduuoRMNagUnN7AXalaQdy1GedKj5thKFeyUcQ==)";

inline constexpr char kTokenPreimageBase64[] =
    R"(IXDCnZnVEJ0orkbZfr2ut2NQPQ0ofdervKBmQ2hyjcClGCjA3/ExbBumO0ua5cxwo//nN0uKQ60kknru8hRXxw==)";

// Deterministic values for testing.
//
// Confirmation Tokens:
//
//   Signing Key:
//     vzbdla00HbNHEjyHtH9zhNiHW9xaIh9lGoz+nrbPlwY=
//
//   Public Key:
//     OqhZpUC8B15u+Gc11rQYRl8O3zOSAUIEC2JuDHI32TM=
//
//   Blinded Tokens:
//     +qJiMi6k0hRzRAEN239nLthLqrNm53O78x/PV8I/JS0=
//     qJRP/aub7Io1osPghMfENIAZ8ImOH7D/tLzuokEHgQQ=
//     Rpubvx6PW1Emd4MLEUEvcilslF9IXH4rOf6M/DIw0RQ=
//     emAayme7sAYSMHZyQL7pz4spX0ENV8GguOaa0kVjy2Y=
//     BCsNsSajBb1oy0BBd8SYE7OqU0fFiAVae4uyGwh1yA8=
//     iI+VRiS6DIeCQrZ1VUDjrNqmrPfQsgL5LC2r34pPQWo=
//     uk4WrGWDL3kcQUFaAdKZcZwwWlFB+VvHxmXekbl7sx8=
//     tDTQL9Q2dstNUBis+dMeGYLXkoyZ0WCxS6p0tzm+FEQ=
//     GBMycmaAioDYqcGHaRuCHZqLj1kjm16hE9Q0IwqlegM=
//     ronQjHzjJ3/+k9bQcIysdTOLVhQTUr0SE5rRuPXr/S0=
//     1LcBYzvkZCveVBWqnD8q0ROib/DBxmClJXUPE/L0rFk=
//     eMwmKIn+x469uG2ELHVAy5gA6w6MhpvNBMxCZDaK6F0=
//     ksmor6Rf+mhygDpamII59NqgWTeGp7LBv6K9pFWuaC8=
//     8KPXVtGZSRhyQA7lijOVX5mw1s1tHBAyI1J02GVj/Hs=
//     KPa8J+FFbtRiOWFuartWFOU8sXmtWQCKAED/OCZeSDc=
//     hKJaPWMgJPk0tzdHhizBWGjK3OdPWek0ulHuw+ugaCg=
//     Gpl+hOjKQ+u8G/9k95ounlvqS1clessS8ffzphAOjSA=
//     TKFhPnXiyn7QKV3lZwxqtjuUUbpnZXCd6NraOK/Wl2c=
//     Ru0Jww8XcXowuE+I206YRzN0H/svPh965gSDUruqGBY=
//     XMtBT/TIWqhe7oEi0sc9PVV0+ORGmkZyXqYbY1sceGk=
//     Hls4b0JfV1wzmEkuzi+JAvV8GSeOMtCu1TUYV3qi1FQ=
//     PtV7vv6wwko9BJvKwKpss/ip8n9kPE4OI0pskdNujE0=
//     zm+xId+BnvG/0pVK0ensKZw6cpGkyfPoFU0Ciy90YAw=
//     vH4Ia7rARJ5BzOtgdDpo91nCb8yJi1hegK0zrD1kpzk=
//     VL2iooUBy0wSKWh+0xPsXL3XBnnHAIlEbYFT0FucdH4=
//     wDO5/O80DbUNpE7VPW8KjNyU6NgNpAXwDhUUL9fjFBk=
//     smD5xbol3KT21x8gWqLdrS6aEF/Z2NK3jcavPkbc11k=
//     UJQWTdjmqfG0Q7EOsnT8y7JDMJcvR1TdBu2ufeCkyxg=
//     OoVG5cljpSFh7slaHtUp1f0NxQMYnhElres1Mryw3AY=
//     /C2ZWFQRfb1z+jDR84qym63nF2taJegOnHfBvx2+iG8=
//     rC1beWdXC+0byVFU+A882xyhusXwQZfYm0HDq7refAE=
//     KjHtaRsaAhemIc62b++WSeATUGa3dt8ZI7R+gzi/2mc=
//     frh0M5bFn+aoaXuU7aFvi2arOP2PgQ2atGoVAJexumU=
//     TGubsu4p3Fy4/W882udNbE971BYLg7pNsykM12cuayQ=
//     7imGJvy4AjF+RaKfPAfz/Czt+kObY5qtSrqPnRjEIAQ=
//     glk6eUsg/9nUas1iQ7MRzvpww44PinupfOwxVfa9KmI=
//     +A3bf8OPgdZ0vtMi6eVC1RFFMqQG9qrWw/bQfeTD4W0=
//     KtRpWtyibVmDt0XTCXCJCg+aN7oD2KgCAWVjyfJO4gs=
//     NEAoVtzlNrMDmhQcCEiyYvivFwL5M1vdtaFx+j276QI=
//     nlTsApw3b5dKzu/iL6ax74/3h6MaDHhaGTAeGqBAmm0=
//     cHKwDZcMDJrTHJ38lv3rhdaoGVJ2yjH9G9KsQSFG5yk=
//     yF+sL24C27QK7RNGEcYXAHyb50/QCwgcvGgWjqqu0DE=
//     qgk0zv2bC6cUBMBEaUBo072QHyadMVqk8l1HvvB50l8=
//     krmsbSgqWiAZny1Z3NP6FgVkmEqPJzNWfb5oqTbxfys=
//     aKtrU/igXLTAfRzFSXyOzg7ZWNtzVClyQ1VTUp/K5Xo=
//     HE5V9vDc4uA1OvcS5NG1dpOjx1DjwqvyUsuBe1f6oAs=
//     /DWUsXLbZ0FnQ5P4Xv5/NZn4WY0O755v1bkj3Ye1KA8=
//     UMqOMU+hU2MqKdgmjgNW03YkL/ta0cQpcivRmRhKrH0=
//     3Pg0R+A1WdbQWMwN8Kr+4sCFaF7svBHdqoXzs79yDmQ=
//     gKq3Mu2S5r6zp+e23dpr4y5a3xY3KF6wDyhJqabhDh8=
//
//   Signed Tokens:
//     Ktsii4pOKOjfmqziUsdlV3lJETaGXG0rksRujn1i1Qs=
//     Mv5M6UhufCEmF7u9pBaGQJAckuwpkSpOJvJq1F+y9VE=
//     MpysVJMYUI4n0NcY2CdZyZxQxADOHJal1pZI2uPiqWs=
//     5stC+5v3eo3tADEkTtxESlCBIkoYI8aRDLJ8VVtcLyE=
//     MBSxmUfpp5p8FI+86A4+0zFYw5dPhre1YYw+sRFWkTs=
//     TPtzCHdDbaW3vAApNFc6nyC806fmdNqG9HaoZMQWGx8=
//     dt0Xmblg7vU0TEzE4yaBtNnQ8igZJcdlNj1CDVxXVHA=
//     RBJGJT9Pf2T2yzi+hfPXnXIjxZ6WbJ7UhZ1f1BGDpHc=
//     NN/FZ/sbjvhGAKzBQdtUrbjkDuCKbpwNCT3cKURvzB8=
//     BCoCHDIFRVyZqgGfB1SJpTsHa8tJ9VFOq0PUPubLBk8=
//     EAjQxthZt6mR2+wgMV35B4Bq61e16PCyX5JkhRwdxXM=
//     3LPp0kUrPjeKVkBFJSOL9b7/qjfUCc8ylqZyjcSFhVs=
//     zpjmculV506uDhkXJ4k6lCCFhVja5EkA2uXkefE3ZlY=
//     nEzbQkRVPl6H9+AF/OBcfLf61o1xO/jbB+7a1amJ3Eg=
//     NgSQDmucmraSK8QZve64DvMv1l01jQEpK9RjSR7J91Q=
//     vCB5ksvHTXCYtbCdBe+xynPo3T6ybVgLfHf0Od8PEEQ=
//     PlkLA8DFzpMQQaCsKICK7zxDRJ10YEdFGvFFfx9Fn08=
//     jm2usDGsU+/KSy2+6Q8X5cwhFr7pZ+tJ6GRCue2ywl0=
//     SD8rfdIV3Hp3PbiQ29KmNN4eYd0kMA6fVr4ZKb6McRg=
//     DKPADls0HZ0m0MtQb2qk5A/w/bUWAgf5gQI+F/9JMQQ=
//     5phPdMP/HWQCMGkXvgW8BDLgKZRTni2eQft8oDOlUhY=
//     LDkKHokr0ydfPuj3cFaXsI1Il1pFEZUrX3G2IM30Il8=
//     FFbRg7fgraPLTOBKkiepCl0ukp6fYHrh/E715Gsxn0Q=
//     aNBW1Qn9y0WejDJJzU/4FmfBnfOipeDOD0qo2u8u2xQ=
//     KGIhrGVhTah7Liwgphh6EidivlOVKbnWMQRrKItcnVY=
//     Ag+u6UoZ/AJnVmLH659K8zLlaFVgEV4TSWiCIo/1RGk=
//     Bjsr97vTdcPRFb8rVZ96yGKGEWqBRobalK09hSjnckk=
//     TLdnTFqBqDBI4LhGQrQLFIP9hYBXjvRIAj58YuCkrG8=
//     nLTmiuYHyKudXYxXSwT5fOLmIJOiuB5nsrvzRr+STlI=
//     sBYEmkom7ay5PR0JI4J9pohdAsT00BneDcBlk9XJAXc=
//     loZ/wr35u1XaMQXPi2KDCXbMoqCR4qSrihf8hGFGFnk=
//     Ethq6jJOlvciMq9CxnvCvPiS6dpQTSJAfGG3JAMTcn0=
//     uKGcjL8+ja5g+kg1GXmoKqB1UxsJjHFYyJsErIxRTDI=
//     zGNHYNpUX9fdDENSlT5a0MFzssn9nJwRs9Fh5NM1GDw=
//     xHnOdiSKGvOr7qzIXgP0epsFPy0STQy2+6nd6NZEeXo=
//     wuFVfI9geQ+U+R7LxM+SFiz15Fj7rpIRMOMNSt43PAM=
//     agMSIk+buHMb4qUc0veGzAUXdcVkHqc+Oy+Zt7IOKzA=
//     mj7RPYebPDz859w61Ce5HEQh3NGQ1xYyp5xthZtpym8=
//     jkmYEUw+dU8ypn03nXl6Fj+OzctOLlG8CSYEI15zNAg=
//     UAuc8dnFor4mva7EGD5bV/nqZz8wq2/RAte8r0iuNUY=
//     9OxHQt1/VR9JpMB4XrC0Yf7MScmHANkpSzcb1XujtkM=
//     Mv5V7+I0BBwNrVYf/012S+esEnqh3gAXZtlE8ClbOg0=
//     doMba7NOQ+4doCwCQdBGWbvI+qofpgNr1VHqrM8vMCw=
//     QIl+CO4G3g6Tbfre2g6TOkKOgZ4TgIoXeOoRqSV4jGU=
//     LBjAPlawmwRldmRDpKk0wJRQBpkNqhzDiDNBNYmzg1E=
//     QlKUaOO/znQjVtwXNGiwUDrV+GW3/a9wZXvvjvib+3I=
//     CAHB+5LsyE/ntr8d4Ll1eFnowotytI+ZksaSdiiIa00=
//     PorTCjYCypszf7/fs4eXgrgV4yi/3qIEmffYmaJ9hXI=
//     Gt95JWkzrdcSw41DCdoTs8umsC7CzH1y0rJ7m/2vGX4=
//     IjtwdzooaSvEr/V+e5vESRhUoJgvLnaqdbTL+dnKyR4=
//
//   Verification Signatures:
//     +Z9XQWp6ltCj5g/GyVU4GTVGNHIVWG5hmNZITgsAZvN088sfO8zmBTNI4IhGWBUCzlKs/zt7k25Kgz/v9lnClA==
//     BdktXBMekamjs0vNyYjAia1CzsjNy+7a+SiAKM9a68nqMDSzfhLMvOoLtCTA/WOJdLk1sSK9PvI0J8bghSKP+Q==
//     ziGht6ktE/UkZMGg2TaEzAB/ugKC/lQV6gUEHxzCApp7SQ5U+meKot5lAk5S0d/Sf1+7Ry8TVSvXkGW2G6ql2w==
//     oC3h53kphYAW8DvCChOa8zIu3UkVUFN5+pFQKep0dXYtGG/PZHV8RnzBVfCRW61FF/cKrfp+tfz3Rz8v52Yc/Q==
//     8U4rqAue0LnOAi1LbpW1+z/LXRAf8ovSA7fVoWV11E61fhiwCMYJYa2k3PhTNtzBf+yOEk6QNW25DrdsXV13PA==
//     3F+KPmTFNPHEquqicHfvYJPFiygpzPYlo/XphdY6g6Xlr6cD9vlm+YlBPYnKdwRlSlQiby0obz1n26nyD9XcIg==
//     soWJkIZVfHW9Wbow8/pk4qMsNAubDs/2JlOMMZmIaOJm6iJLrse5nws6RL56QhUS12KDpVi5hfr7POseXEH3lw==
//     IEpwZV6/I6+v9gEPjYSBZlqtvynUAEshq6rPNtAHaTgxcP5fvjfJQml+FHhU3x4BjGPkJ8WABB98/78gi7WLFA==
//     Z9qJvSP4rc9T1QVk8O/MW2P1iGSxZYtgpHnG+B4aCFOZEWXK2SSNo3BE0CuI7a53WMT2II4T/Yj6FhVA68UpqQ==
//     A68UwMC9GkG6NdeUpk1Uujfg7jXfgy5BPzQP8xO4CVDdK6MzPnaJTT1bfiSiYDxOOR3nUX6pR+keICY3IJHtkQ==
//     MLcO6JvMWnu6+7LD2SxU3wjAikYKqm8VsHyHEvxrwC8MqdYnAk3QwyqY4cz1y85D6IcfixLdV1tfgUVNIKS+Sw==
//     B3ka0buAN04NUTl5JEVkBFZnZnI9aYleqnxHpG4IIVIh1xkhpRHHguwr7+n+M7dJzKIa1o1nPfTR4Zaw8uPQQA==
//     85ABYWSteFPQHDZog8z9rDFWgjVqZEkQXP6vEnEclsv7BofsYwyRUDxiERcgRknzr30lPeqxg63mKMyGnZ25BA==
//     zmbuOuGBWOIMRDaLa1fH5lpqHdr4i6ZvOb7p/Fj44nByi+TMPzXv9GdF9CkDN20Iduy6LiOVZb6wvJtpJ3l2XQ==
//     WFBZXiarPUEz6COtHlodMT12v+Icf6gUQXW4GWhXoA6u5lsa3uGPEBvxZh90kcwyCKKI/Mt13W/PEZNxdZWBmQ==
//     PIedXOTy/5zkrxouvxv9yApmw1ye7zS8UZCcYhEYj5Z8Y20fzH7D5ZAJjt7uX0LvCZHmnSjV5xY7fcZP9PyZiQ==
//     LMhwPJ9L6wD2CwHtseesfyLvslKkQogymD7Yfu9946zVlB2VkQCQxRkKndQL0dU9qUbynuoqB6UKap/JR2DjdA==
//     G2cAs/OYgQeNhy4Yh6h+/+CGlDgQtof4SZEg0MkQ+xvKdmFBVmdIfKANaZYBNOcfxWxpKUH0NVaSe+tNn6QbCg==
//     Jq3aO3UsTkgOjaXrvL7cYMrfG5U3liGyqtVIxj4BEeT4BWiY4qMvXGlLFCLFPC3de2rouUaaqdDhIHZrRiSSsg==
//     9lt1kEYmSCvEZcvfs+KWrXD8XYykUNoLyYiSUq6VPPZIxg8dfBFp0Yabea3CZXHaetCKAznMVAlYyt2QYOcpig==
//     Ba+VI7EFvqQWRo8iuUYX1sS4EotS3zzYZFuwiyrdD8wNpGtKbo6DHgbzo+WkLbHUQFjZTlBeoVOw4Rl/NZJMMA==
//     QmN9vW4hC+4GxGDP8OIDsvwA/9vZzMDof0HRYkRD78+tZWSic/xC0rzL1ltTYVm2ZO64ZK1PxBE6j/6z7VbW8g==
//     HE23nVQysY3YlPYLA6CHtyyD2OmeixRsLzolWXxtWfjS4QExY0Xe2IK3dJkU/TsJGogfUQYl1zvuBCWcs7Z6yA==
//     B8tWTLiWzWa61s4zscWS23sf3PLAXU6vBSCSNOstz4PBXmmw1PcUEMwSUTJGQupwGLEEHvIJMPZBaUsJtmBAgA==
//     /dHptF8FxWu9IZybSnzsEu+PhuwS2UXusUsSdTa69zWuP4AsjPNOllaVY/TucChNDBhaGJMnBYm9bcmWSDjm0Q==
//     gD+f9NK0YIS48VIUhmaC9XOEYxjPtxRC4P4LDmDvPzJk22PPTPtP+p9MEyXPgP+zV5IoKuN6KtvJ2gCHE4khDQ==
//     Mhv2p6UefL/ahe3Ji8/U4ebMjJxSq5ImiqfeMZB+ngvqHRY0uH1WxsKeh94EJK997QZ61Ff0HylKjDtnBPw76w==
//     RZZP4Y6F94DC51vZxdEJ1S0f8VAG0wyBVDb7tgy1wjrZ8vi9OVxjtoHn681ejmpzysqSV8ATeTm/xNyUA2iG/w==
//     bhSaLLopcx06tl5YozNVLB7AyZPvjqey/ZFpBzwbCrQROVCfGXCLGnLVZGXkvLYnzJqA5NtG9Cw7X9+SvcZvbw==
//     n6CfLBw68UauJg9DtnpKtmnVMOjfRwPHZHjiHcGR1JXAD47RA+SBZ/0yI9yeJCpQSs4UFKwSCX48L8VXv9twWg==
//     iEUDL/79PS1LRfgqYovPb5bJk76a7rFFcbrXuMKPohoS2Rky8ajigLYjITinJAP+Ne1aD9AlXeAb6ifuuPhIzA==
//     AdCjNiHaoa/Suga+dkEFafrUGRQlosqhsCNY1+TRySCgeLA7McFSMDQ/oz8auB6VBZ0i8pZh/uUxmv+fumzIOw==
//     Ly4YClA0kcxfGlDTBMqFWxQgaW/9qh0peV/eqOdiQbq3xG0pFM8ouG1PbrQlakB7SFpOLaw8rXXsMIDNq4bGmA==
//     vUH/w8veqSwLz6V/ND4fAc7DOUqFhFwaxNJIuf0HYFDZ7rt4lqmraVzSGUP3hE/+0uEX4SRdByt33rnz/Akaeg==
//     QP/16mclGrIF6wHY6aC3Aczo2lO4q6HCt2TXk36b8QPvYQTH3wr6eZOxrvK+i2W/xT0iNKwmCdQvf+CMXMOb1w==
//     kgJH26HO81Whx88R8tuuoISkbNljhAMlsS+RjgSTUwxsWe2Y4S/MUkMfSBkvjhEEzKQ9fXXL1aHJV8pAL8w58w==
//     60yyP00POYx+d8zCfsSMxNJwXwimRvWQZ4tKxChr1IKofa4F0WHNE7qBM1fLF4ZHlavevyI6I3LNyU7lDOY/ng==
//     tmroZlGFmNp42vLQKE2b5CZ0RBoc9lP44hzqPNDP5GqXYx2Ti74ARHbvggfxayL+FFZR+s6y/VltyA926s8bgA==
//     A0pteyI7SiDfFIdHrgFaxscYi7+KswFJguIE4TDo2D+l/SXyCJ3noVKyo8hsCIgIcg7AgHF34Cw1IjZKq0GtVA==
//     S/ejSKQvDbBFXMFUZxcXWOB0g4gdVWtfjRqhltQhJyGJMl6yIhK6eJ8utsn0MbokafzN0X1nMDgUcZlrALf4MQ==
//     xEYTDWYEkjdTxqcfA+XKF4AvPnBPTCekmi8LAweUy0QS7FUFQ7ulzgLByRuhVaRV26u4yZUKjAY2zHT7GbEWSA==
//     ZcLPSI4WhgwxQufMjIaqgv5JnhQ38D2clKskgGjwtyN5sbWPW4OGlpYt0KaWXiUQ/DcGhrIHIRaDEEK5OgKJbw==
//     bBR0bdi/x+jyEOq/WrhNah+K2wCoqcFQ17y6twTO65FsINg+68faqPz27rh8S6OwkQaBoKoTqZZR24/qMofWdg==
//     pyhKI7QsDwx6meOf7QKXZYijnwkh2LznM6F7dkmqucRe2xpJwMIH9InbJzRzZA5+MoA0ZeNgkiLZDd+rYw4+IQ==
//     w8fnt7cqNYbVWkrF47VGmDAkNZVFmm7LCN3k/yqQIbFPZwUt3KzQwNEU/lH7vVLV2+IzVhZAr/SF5V0RXUx6uA==
//     sTfn98SyMF7A0BeDHX5U91d60XgXWTLtk9ofqqxYa83DA2uhiqBdGg73nKJLeAY2k7BnaS0UqlHaZCM61irMaA==
//     BtrGFdoljQcDzXG1qyI7t+YPKU8qZdJH9TPZvofHjubEDC9mvMxljgE1EIbcpvjV2ZFXJizsgUEAf+7pQYa2PA==
//     M4ch9HyN6Gf5Pa2WfICcY4cBJ2VVTEL8zoSWNfmdAroxu2pE9P42w1G3E5ZYf6H3D0SmLu05vNB3tBSk/vS1KA==
//     xxcALu7b51kqaWgN/wH1AM+sNdT6LtpZ5/CxyVUi7EXVgM9n/Lhud4oruxNvV+g4cbn176MOdmTqIdHKiYgD1g==
//     ZEJ0KdtJqANHu39QewDjsCmpiMWGPXgkUF07r4802xnX4JVkpb+tH4fVqMKr7xH43Hbu6s2scCKIHXIn/2Z6Sg==

}  // namespace brave_ads::cbr::test

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_CHALLENGE_BYPASS_RISTRETTO_CHALLENGE_BYPASS_RISTRETTO_TEST_CONSTANTS_H_
