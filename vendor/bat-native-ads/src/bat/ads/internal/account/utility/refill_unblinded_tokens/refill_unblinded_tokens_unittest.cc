/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/utility/refill_unblinded_tokens/refill_unblinded_tokens.h"

#include <memory>

#include "base/check.h"
#include "bat/ads/internal/account/issuers/issuers_info.h"
#include "bat/ads/internal/account/issuers/issuers_unittest_util.h"
#include "bat/ads/internal/account/issuers/issuers_util.h"
#include "bat/ads/internal/account/utility/refill_unblinded_tokens/refill_unblinded_tokens_delegate_mock.h"
#include "bat/ads/internal/account/wallet/wallet_info.h"
#include "bat/ads/internal/account/wallet/wallet_unittest_util.h"
#include "bat/ads/internal/common/unittest/unittest_base.h"
#include "bat/ads/internal/common/unittest/unittest_mock_util.h"
#include "bat/ads/internal/privacy/tokens/token_generator_mock.h"
#include "bat/ads/internal/privacy/tokens/token_generator_unittest_util.h"
#include "bat/ads/internal/privacy/tokens/unblinded_tokens/unblinded_token_util.h"
#include "bat/ads/internal/privacy/tokens/unblinded_tokens/unblinded_tokens_unittest_util.h"
#include "net/http/http_status_code.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

using ::testing::_;
using ::testing::InSequence;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

std::vector<privacy::cbr::Token> GetTokens() {
  const std::
      vector<std::string>
          tokens_base64 = {R"~(DH7weiDOCuGT2nXcnlmX7G5cybawo3YevuaEM0WXqNHxzpcwbcrftQ5QA9OTjLJwvTwBQyatHa8MSIyfffH+WVfblV4CgHL4/dYqnU8qt8PoB81inyrXRhI9ms5t8CYO)~", R"~(Ifrq+MkrmRRCD5RjaG+kpx8+oDwcwDmcxn5xqXRcIP0m5SDv9C2xo/E7Xt5WGNUDFVXYDZ2XkoJjXxI1Q0jEKsQuXLITmrAJbajVQPKJynbMoiZVgb1TZ5qzIUDzYaoJ)~", R"~(jSWJU4IvIrFnsnITyLVvQ48rw5mUe/Q7gZsIqdM+Fo9PCdLJDjL9t3B02Ttxksg2yat2dpN6hGOafnG7t35QW0nqL+OV7w5/RUm++6PZvMHyoyU/qwrD87TIuTZAO3MD)~", R"~(zo5rovlE6KzkgdXCGzl94Dp8w+MWsGj0EXBYshbOmEioyo1ql+tlZmy/i3zZBHrlNc62xlDegK8RrMwFlggAPcrk6aXJcQtj5G8ivXFoQuDJ2LOeCgckYqMZYHivwqgC)~", R"~(0+36W3XcOqC2dLgfkfN/5XFu9/kbI4PIuHuXtqMbG1OG9EYvXaTDBainQpXnqyDZpAqaCf8dVNXsRcDx4V9TrB17KqqXSmNHIIhWozNAuVBdOnEAZHTBfe7wIVc5GRAF)~", R"~(HdN2p+JKkPhnGRw06Sf3WBoMjbaj3TjQNa9eyG+Ei/Bs3W6GXFZGdB8v3/Z1wrQzHksXQ/23VFj3WaTsAMhd/lWogDb7C+g1GghxLSj3BjOP5sU8JzowtPrPfmYr/HYE)~", R"~(0wMbnHVmi67JvR53IxCG0rBBWsXV/rpxy44iEpq1nqA4o5A8BO5AC6T8JjMFgEEvWBwNeXfSgUMSgnU44XiE/wKp6HfX2FGcqF0QCNwA1RQxHKxiWMCVvFkxHUx5vYQL)~", R"~(5iBzJ0cCF3eqgjcBwDblnDyAwk4HC+f4k068Edy55f7eQTZtx9c2AX21j983+kBIKQeqI80/q9iNrbgFbpWrzRvLurX+xLmFnrWieg11irgQsXVFFV5DNyxrAFRGU4sL)~", R"~(ZfK/2GB1Zi9zSBLwva5KVxMa2DSd0jyqgGN4r1MW8+mJ1ugYh9kjvimWhyeQTjXfmeIyV/qeDvpiUsHrlRqab4PzHmnmzg3pbE/z+TQ1NXGG8my66vB4+1/FqXSN22cL)~", R"~(CyjZY1hJsB5UlVzF5s/jpmGN/q5r1rwEfAFooL7z2kRtezIwetRISfJ1KIsDVsVwGYQKhAFF62GztJ7FXpZ1FfmPiGL5rE7wbPfE2T4CAmH9DEe2UPUIOeMhzjIgpGIH)~", R"~(faN7sGwuyZ7ag61a35wt7S5XvIQDi5LdH2NK8mp+NCqQPGQxxOZvR1UFI99oQgo5tDyoVMHRcDSKHjHI5GWdo0bLcwqKaUHQtsGvq4kUtKJATYlwGA1D3EZ0TtqWH8EG)~", R"~(k9swupRZLdMMEiWJQFAu0LoiH6TeWX4J14bX/YE1rMZ4BagnpeSjwUmZLMUHsKJiFdnSqpDtFuNKGH/K3Mh+ZBcyisLFHZkRTu9tb96Psld5aa5JMXThtJf0jxHgXTMG)~", R"~(Gw1iI5Cl1gCmXYGjrx2cdA/FbY5MCgk1Mpkr1hoCwtnX5Au5Ahaivb08GNJVLu8su09gFPm7Nx6ZozidjG/DsgHCrxBbnTjWfNoSf5qzXf4ffx+kfBiCusEPJkrbS7QD)~", R"~(kMmzIwG6K0/SG85lWcLY8LRMWjIeR4vvNix6ZaTVZWMXjQeejaaVUEld1gqvoNmaubmQtgy5UMBKs+39RTVgUPDOyjld9dvcsqy1FP/jZe9cxncRPKWKEZ6RTKT8JA8L)~", R"~(SbQ+dBXHxMuCaA0Yh+Jzsf7+egTCPNYsWDHkNs3ZNO66w48Oeal571xny6R0Ox9KC8QW/3srB1jT6/pMcWSt/ZHRCDel6pxyfqZ+x6OVVZsMNH6IrXWyzT/YGUjnALAK)~", R"~(oOTFr0aRkJdht7CuLbOEKu59R6lGmb6rCaOX5MUrJ5dd1lVU7Z+mfte1IKufyeab+P2ksEugtR3LCUJL8TFhrXXfDQoOf7dUdqJpyvOWqYLu1uckjpPOmLpPk0aSVqgF)~", R"~(EupatYmtXTYLbrQjP0pQGhSSNOXflaw95wnhITVOii5dNnQHIdq12iVwQNQSTy5jschTcRFBcPH6/+A+CKy4gsadXsYn2mWjDIq+goK1C1DkUSCAomtQINQsnX8y0l0L)~", R"~(w1owKX96yhN8UfiW4BFr9C1d4q8IKVBLpl8VJUt2pqH58am/lr+VhYMvQnuzMM69dcnV2XRjNttQHhTV+xrSPENkzrCN1HBmCtcu9tKQsIsA1BTR5m8IYp5PqgR6Ij8M)~", R"~(+MWbSoJkz+KCnz9b3CQeeiaXjedx5p5gjbEYHh8/38OUETq6PiuONnpz6D9n5UfE8hCONwXu18t12LQwehZ2JZF+JA3MCH8FpetZN4WoGz/xU2LQkTdgeSIPu/lSrYcM)~", R"~(Lg88mfaH3gnlJhYMjD4x8XfSmHtcVlpVivjdzx2z1kml3vHHWCyBH5OpZ9qYO3bYxzJ7gDJa4296vuj/diivDIzEpiJxJ0lzutW24Fh2zLqR3LxrIYGyZlSx1nE/luAB)~", R"~(y1YL2GOWObDr3LorCpFJAtmyUPMdfuoECFQ9a+kxlBEraPZCaaATkYXkuSUd/jodAM5OdQeAgFIF2srj3oyoIu76Y0B3uFYksu+pRB2tWYbbpONjBgNHTc3GzBZsYT8E)~", R"~(jBhIEjo/MVOOE34Ab3xhoPlXvb3rIzlLOGJ/DEcVDAxfvSrNPrp1ILWpz93xfnMAFwG7b+LI5OmnmEHCQ0abUxDqEg5pG/e5xBbyWrXoOVkQ3xfYdnlafc4FxzSW4L8C)~", R"~(plRDwdeaW7SbRefcX+geKMoCcym7H5UHRY5o7YZB9yjmlZbi2/H8Y1YqPoLfpK3fxAgikzhtg8IovdVpNu+SYuk9powxeYiNvSm+/ZB9ywU0r0IZKB4kYmuvusECrH8A)~", R"~(noax7RfK3s4egB5wSZQ831hRbezNVzxCRpNOgi1eNEQAk1fTdMi6nnjdcX1mEtz330MJepraMfpAed+kPsreqr2SoLRClXWCyXdOA0TrcEwa6Oma1xTeF0o47WhMvIcM)~", R"~(aRz6158ReeSiTbBj9nW1t3e50drDQeQP6xSuSHo8kodIt1tOIPRNVGdjns4eU61qlIaNwJZp5VG0n+2DwhCQsgpxyZbx3+nXL/Uczyp8YwJCWtnBqhhKvvazGdJW1OgE)~", R"~(oBD52LQLAE1bo5s6NGPt0rGeQMGROHrN679EWza161Hyhz+UN1BRZV9stHkxSPbvHyF2oqwB1vkDRSG9p/3ZFfNanYubT0f50VTrBG4Q8tHzglTMlNHKujeSaalvfF4A)~", R"~(VaQF+uAP7LTke6BDhVsgjVEX0PmG7fs0v7RbRI4KMXwqRsMNUtJXFNnyoGxiN4jQGzXyYVqUKVJsa+tabC8tQpPwGtpUMMhmBpb/+j1LH3Lwap+zVW93qrq/A72Ep60C)~", R"~(N4Z0LLuPUMVqpTzKXZfPuzbSreG38+zKIgSTMxJMsEyXcu6t40IL6T2N6mQKILW6qptih/oZd35zTDa0OYFJpNbogUazlSdYoi2PPxcASPyszv3K6v+ngeydVmXFSS8L)~", R"~(FlYLp/mxj9Z2mUJJpjVFS6mRrU65guREXj5gKDDWZe6cMHOExqr6Empym+mGke3Z0MuqN1n9ks17Zo/tgqwZgDYZhM8iHvcUTnhvQORGpigfeU6TETwyvptgWVqYQLwK)~", R"~(bi8JH/URcjgb6J6+KVYK8u15AgiqZm7MJ7bxMjDPF77ddCOT34c8j8b53Dd43CKHabHKOF01ch0zBUNFNqNWTT9CeuRZhLrJl/TpsSHm8bq4grBCCgkXUOqFrQsqaqYI)~", R"~(9byplT3SZycXf8bjX3Wx6wrByx2Kg4zGMMjOHZlii8iFJohxAh4Msb/gq/9jkbbBvV/9SVDP+cZeJ4g3FIR7atHEZIE064jTa411pQPG0QXgAgB+OrTDEFmm+Zt8Wj8I)~", R"~(ZtrPsqGR4y/yGooOJQbfFe5kQOIBXS9fGi94vojCEsHaDAkJ2vZJLaSJ5nU+ZkDUQ7doFrtd6/5y6Tsjq1MNqqNBtc0zab4QB+n96ow7a/mco7pf7TnD4G/hVakYOO0L)~", R"~(tUeXr0aILiTglTjI9HLgHPzrKPwkqwulcob0/OI7T/96mXUHa30lbXsmIfjppJm4UnmwtF8hBhGkIFpgboskisM2twOS8S/Bgle8IoF1DZ5ALQ/q2mRje32Gx+xL8EwO)~", R"~(0x67wnJxXBO+95sOd044KChhi4GY2c6bVFJNVZ2H69YrhMgPK+V3J6N0hO6GGwmsDlXOVIGAOYPiDfev18mQk4IgDfcNrBbOtZXRUewJ/E7YS2t2m0rlOzsvUznzfsQB)~", R"~(M5BGHqrzwywJpKjx5t7X/2hoFTio+eVaZVKGow4kwy0s8W/BAcH1pAPfPOTFl283qh6iNNRQbncb4xSQHOBGtsUWMdePuShtA7bVnD9bmkFVvkGlD+8YxkUhFcrKUGIC)~", R"~(AVjSnnK//7C1YmPHn5nr5oacX2BkhFm5W9Riv/ntQa6QqMTOe3EizakaZS9bIScP8TxJsFhUj/yAeXlMV2kG+mA7VrewXpfJ0pbS7vg3FnoL+mpAOsLSff41iGD9jwIB)~", R"~(UglijJk6Fj9dTxC6TR2Vw8jrTKkBwenXWAHZXkOI5OVMrDQID6qbM6qe50hLNl3sRhmhAQrUGkcMuakI5xh3cCctvCoqKC04JtEAUknlfwRbv3yHIfNgsiVxtyRoDNUA)~", R"~(HzXsrxQqS1SZ9GNujumumi98W43n+zGcO+girwO1v37vDFdLA9GTmdqXFVMqO1xtPg03dm/chBtlAX3ylrtMSH6Sss0vTfsM9Yi5pkAL32tQmuj7FTPuhDYH8ERAcy8F)~", R"~(4KMeGsXiusi5VpVJJ81Uxc9goJ9jFlArRiFwYxr9DT4YXuNwYvwLqrHjrQM+2jv6cm78CsFYWxJKpepGK2oF45EOyvsX9SvXLRQa6gZYRNeP9PwCH7mHztgFvPbHnpgL)~", R"~(LiRyEYTu6ppsahSjFfz/2owWySGVSqojs9nxEDS7BUtBcjEMzGJB7p+pzC6ljfZm0BRTJz1sL9B8h0GM5LPE/Qs5OP6uyiks03QK2wfyf7Gw5njhpVz0/HdE2IYbFlsE)~", R"~(M0o812DGEcbp6B7rmqFQV/xWMkLb8CGDP3iEv8CNtp8VPTS0GoMgybQlgEEwFbCY6IvT8GqZJtiYEELU3NxEGfkGsD3SVeeGPRJhRr6RpV/cEkpMrjsl5tYj3cSqHJQK)~", R"~(iwQDnYJHOA1BbtHEiwVUxV1LVchhngtjEf2vT0aYIETqPv2wXr7M70+ZKSzifXJrBbmPok4uXXZCU1WWxNH0JSg6XXRRv/J5LrM+zhjMCkeM4ndo7QbXweZXM1ynpLMH)~", R"~(eSv9KL7NaBFVvvFTm2WrmBZTmE5+znYRPFwqtWEdd6q9OLHLgPsbtf7ROQ0D/L0KmfH/rwfuPKki+IQQs5/+CVstxa8bIf+s9ZXrguyfjK/6YqPZ1B8PU/UPAwfRk68N)~", R"~(0SnR2MlzedPjCtTunhDiXM1wBp+dJPxsZU7JKwlsHwLIFSMJmBKeSQPBOajBIisqK9aS3gS0xt9+tPYacGdwcfECLVUPdhRby9SAuWZBRUIF89XMxSa73IsQNaWUeqQA)~", R"~(ClKVqKc+Iv/AGczdmZ8E520mPUfNc2cE7P1+Jm9NxwJZZDuCzn3/wvhBHpJvSj1iNsCyYjjIhSqxGaT24Ita2c330zl2PBIaA4eoBmmW4HUYtNcpXdo6qE6RadspKAYB)~", R"~(itu0lbk4kgn0cQP0iiksduQZjKRcDPfnTh2B/Lnqir+4eZKsJmD/oYoLwwT6riiEuFY6FappNzcugHd/M1mw7+fkACivxSBuNPRfLYzWn7L0Z98CKMZHMa35lt1IdaUM)~", R"~(iJ4njAR7URIjWK+/TU9hjA5WxySN/vFDR1sC2FAYM7vt1aue58eF+KZS1qSlfcy2anEATeKCCfn0J0e+buSMrO9jxVUYqCuoF9vxwZQRwVsk3dInXS48bHKgQBdUXJwA)~", R"~(jNUU4EjWeWhY6NW63ayLmPqguWcyxKZGFX9Ynj2JaMths/m+slg8js3N+dOu124QnU6Re4+fitqyL+4cFVKhrO0F6+7iEk6Ip2DbCdXDTzaX+m655Xume8CZ3jBJlYkE)~", R"~(SgYf6rrnry9EIWiJWNAcgp+6TsucKXXO4vPEkC0NioJYLp6QOCThiwSRnkkFHqMzVt4YIooowXaaXwrfGjXvZiL98rt1eddhYRgJJx+ZP7T0L0Kp2o7ezIOJsCCz1HIF)~", R"~(B77P8PpGowklUcjCGw6l/31IRdXgp2gjzuQye11kW0jG3/jktIRME4ipIsgrZ2IMyEaGvGk+0xL4zeir0Fs143+lSmqEgIPnTQoaygfobmSB235W/0fI/uhOsW8NfN0D)~"};

  std::vector<privacy::cbr::Token> tokens;

  for (const auto& token_base64 : tokens_base64) {
    const privacy::cbr::Token token = privacy::cbr::Token(token_base64);
    CHECK(token.has_value());
    tokens.push_back(token);
  }

  return tokens;
}

URLResponseMap GetValidUrlResonses() {
  return {{// Request signed tokens
           "/v3/confirmation/token/27a39b2f-9b2e-4eb0-bbb2-2f84447496e7",
           {{net::HTTP_CREATED, R"(
              {
                "nonce": "2f0e2891-e7a5-4262-835b-550b13e58e5c"
              }
            )"}}},
          {// Get signed tokens
           "/v3/confirmation/token/"
           "27a39b2f-9b2e-4eb0-bbb2-2f84447496e7?nonce=2f0e2891-e7a5-4262-835b-"
           "550b13e58e5c",
           {{net::HTTP_OK, R"(
              {
                "batchProof": "BnqmsPk3PsQXVhcCE8YALSE8O+LVqOWabzCuyCTSgQjwAb3iAKrqDV3/zWKdU5TRoqzr32pyPyaS3xFI2iVmAw==",
                "signedTokens": [
                  "fD5YfqudgGrfn+oHpwPsF7COcPrCTLsYX70wa+EE+gg=",
                  "OOPCQu4K+hfE7YaYnI4SyNI1KTIfNR71rIuZKs/9rE8=",
                  "4kCHwIqcMuptlWqHNqGVpSBB5og8h5ooIQkno+qV0j4=",
                  "/lNHOB5ISVVNvoTkS0n4PhDynjYJxKYwXnaDVfzmGSI=",
                  "+ADYC6BAjtbrULLhXoBJM6mK7RPAyYUBA37Dfz223A8=",
                  "ipBrQYPynDtfMVH4COUqZTUm/7Cs5j+4f2v+w1s0H20=",
                  "Jrmctnj+ixdK3xUq+0eLklQsyofptcf9paHQrVD20QE=",
                  "MMxS2Hdx3y6l2jWcBf1fMKxwAWN215S4CD/BPJ57oTA=",
                  "oPI2nQ8Xu5cS8dmLfDynFjWaxxGgLzYX++qUdgLWxxU=",
                  "mk+RCIjgRyqsFDG6Sukg7Sqq9ke7DheF8ID3QJqdCi8=",
                  "OlKDho69Ulh+s/6KF8eS9LG3I58Aq3mgfPErr8AEo1s=",
                  "pnZk5XlLuED7I/sYNYOedBqLvg9KAC1Tw4poxfojFBg=",
                  "2mL4YIz3VFtdrHBpBUQLIPlsXkvfpqneMCneVDqDgBI=",
                  "QPG8e94mNMUgeueC2h+ANRfnkjkG5yli/hpPw8mFwRk=",
                  "2OiY14D3B9nKW1ai/ACOx/VO+y/xWFcrXwGPvlGQGwY=",
                  "hNe+AZ+QIkbkwfnkYKmuq4LFjJez9c8QXCONIHMa2yI=",
                  "lhXQa087T1T8yt32rwlO0Y9K9i6A6ysJxaeoCpQsUXk=",
                  "2BVub545mBdHJIZnotoHP2QIrSstOdAGeHkTk8PbsA4=",
                  "cvsy/fUIwOYgbTvxWoAH+RjRjdBKvjpC0yS8V7TTAzo=",
                  "UsWm27QlfxDFAXUKOyQd+QbzFniAo8KMAcb8ogQn3zk=",
                  "LO9hDP7KfQFIFuw4y6qKolzZCQAjVUtGa6SEJ0WtH28=",
                  "oLrrrpgKoz/L8cEG4J2VV9VSJF8QG4Gactshr1WwZXQ=",
                  "DrtwKP5kQEey3uOZvQzjqCTT30elIrLRvw3PIBqSdg4=",
                  "mBxJCg3ClDS2IiJePXsv6KK6eQCY1yXvOi8m0/54uRg=",
                  "9p4vrVEEIEnmreI1gy2JHvVtunHJjqT+oxUmwidJDlQ=",
                  "VBMfinFy5m7jXqv1LPVqSvAn4mhntpFZ/PyS4eoJmiQ=",
                  "om0eBmPqhiswq66mRdfgyzyPG/n/1jJXS5vLRMB1zTA=",
                  "vs1t2qaE0RptGUHoc6CC1yNJAHJhs7g5Plwpk2hhwgQ=",
                  "GLtViGiHvY6DnWT3OQ65JTBoCu4uv+S0MCvm97VJWkA=",
                  "0tKtV02T7yomO6tb3D5rYr/UHQy6rITYVygqUMF+1Hk=",
                  "SG4OS7WthG8Toff8NHIfBafHTB/8stW+bGrnt9ZUCWQ=",
                  "/JaxZ/fXY8/bZdhL33sorUof6qDfhRHqJn7FGXNg5Wg=",
                  "8vZlB2XPZF4vMn4K6FSNjvk5aZ4G6iCVSoU+Rh6Kgx0=",
                  "xIbWr9fuB2qr1Xr6r5vMIzeOraIiLB338MSWl8RjATE=",
                  "xDYuZfPQiVA5sW75Z4M+1fmtYvifXTEYX/BWsA701ks=",
                  "2l6UgMUlJBEY2R+CTJBX5M2l552bkEPECu7YMP2OAy0=",
                  "uLrkxPY2eBn3FJ4fkuklZimz455rCzCzvcFYBmVWFUQ=",
                  "4EbkdgBc1IvhlGfaXuQxthQl3+wtM/qMdmnyfJE/MVc=",
                  "RAlXUOypctgZ+EIBiqOVmnSW5VroQfT1aGqk0o/wR0s=",
                  "tEehxSWHMtdBzl5mZWNSx9CmGzu1vrWm+YwdjvnNcUw=",
                  "NF8qNh56/nXBPITAakis/FBUbNYlJQZ9ngR34VjJkiE=",
                  "qrPGZKEmgnLMON6akKR2GR3omiPNBLnvB0f5Mh8EMVY=",
                  "2A0rAiadKERas5Nb4d7UpBEMd15H8CF6R4a+E7QnPCk=",
                  "MnS9QD/JJfsMWqZgXceAFDo/E60YQyd52Km+3jPCzhg=",
                  "0rTQsecKlhLU9v6SBZuJbrUU+Yd5hx97EanqrZw6UV8=",
                  "qIwAZMezVrm7ufJoTqSF+DEwOBXVdwf4zm0GMQZiZzI=",
                  "6pYOa+9Kht35CGvrGEsbFqu3mxgzVTZzFJWytq0MpjU=",
                  "xGd6OV9+IPhKkXgmn7AP6TcTZSANmweCS+PlgZLjQRA=",
                  "tlX/IqPpfSvJfwCZzIZonVx3hln15RZpsifkiMxr53s=",
                  "mML4eqBLA9XjZTqhoxVA6lVbMcjL54GqluGGPmMhWQA="
                ],
                "publicKey": "crDVI1R6xHQZ4D9cQu4muVM5MaaM1QcOT4It8Y/CYlw="
              }
            )"}}}};
}

}  // namespace

class BatAdsRefillUnblindedTokensTest : public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    token_generator_mock_ =
        std::make_unique<NiceMock<privacy::TokenGeneratorMock>>();
    refill_unblinded_tokens_ =
        std::make_unique<RefillUnblindedTokens>(token_generator_mock_.get());
    refill_unblinded_tokens_delegate_mock_ =
        std::make_unique<NiceMock<RefillUnblindedTokensDelegateMock>>();
    refill_unblinded_tokens_->SetDelegate(
        refill_unblinded_tokens_delegate_mock_.get());
  }

  std::unique_ptr<privacy::TokenGeneratorMock> token_generator_mock_;
  std::unique_ptr<RefillUnblindedTokens> refill_unblinded_tokens_;
  std::unique_ptr<RefillUnblindedTokensDelegateMock>
      refill_unblinded_tokens_delegate_mock_;
};

TEST_F(BatAdsRefillUnblindedTokensTest, RefillUnblindedTokens) {
  // Arrange
  const URLResponseMap url_responses = GetValidUrlResonses();
  MockUrlResponses(ads_client_mock_, url_responses);

  const std::vector<privacy::cbr::Token> tokens = GetTokens();
  ON_CALL(*token_generator_mock_, Generate(_)).WillByDefault(Return(tokens));

  BuildAndSetIssuers();

  // Act
  EXPECT_CALL(*refill_unblinded_tokens_delegate_mock_,
              OnDidRefillUnblindedTokens());

  EXPECT_CALL(*refill_unblinded_tokens_delegate_mock_,
              OnFailedToRefillUnblindedTokens())
      .Times(0);

  EXPECT_CALL(*refill_unblinded_tokens_delegate_mock_,
              OnWillRetryRefillingUnblindedTokens(_))
      .Times(0);

  EXPECT_CALL(*refill_unblinded_tokens_delegate_mock_,
              OnDidRetryRefillingUnblindedTokens())
      .Times(0);

  const WalletInfo wallet = GetWalletForTesting();
  refill_unblinded_tokens_->MaybeRefill(wallet);

  // Assert
  EXPECT_EQ(50, privacy::UnblindedTokenCount());
}

TEST_F(BatAdsRefillUnblindedTokensTest, RefillUnblindedTokensCaptchaRequired) {
  // Arrange
  const URLResponseMap url_responses = {
      {// Request signed tokens
       "/v3/confirmation/token/27a39b2f-9b2e-4eb0-bbb2-2f84447496e7",
       {{net::HTTP_CREATED, R"(
              {
                "nonce": "2f0e2891-e7a5-4262-835b-550b13e58e5c"
              }
            )"}}},
      {// Get signed tokens
       "/v3/confirmation/token/"
       "27a39b2f-9b2e-4eb0-bbb2-2f84447496e7?nonce=2f0e2891-e7a5-4262-835b-"
       "550b13e58e5c",
       {{net::HTTP_UNAUTHORIZED, R"(
              {
                "captcha_id": "captcha-id"
              }
            )"}}}};
  MockUrlResponses(ads_client_mock_, url_responses);

  const std::vector<privacy::cbr::Token> tokens = GetTokens();
  ON_CALL(*token_generator_mock_, Generate(_)).WillByDefault(Return(tokens));

  BuildAndSetIssuers();

  // Act
  EXPECT_CALL(*refill_unblinded_tokens_delegate_mock_,
              OnDidRefillUnblindedTokens())
      .Times(0);

  EXPECT_CALL(*refill_unblinded_tokens_delegate_mock_,
              OnFailedToRefillUnblindedTokens())
      .Times(0);

  EXPECT_CALL(*refill_unblinded_tokens_delegate_mock_,
              OnWillRetryRefillingUnblindedTokens(_))
      .Times(0);

  EXPECT_CALL(*refill_unblinded_tokens_delegate_mock_,
              OnDidRetryRefillingUnblindedTokens())
      .Times(0);

  EXPECT_CALL(*refill_unblinded_tokens_delegate_mock_,
              OnCaptchaRequiredToRefillUnblindedTokens("captcha-id"));

  const WalletInfo wallet = GetWalletForTesting();
  refill_unblinded_tokens_->MaybeRefill(wallet);

  // Assert
  EXPECT_EQ(0, privacy::UnblindedTokenCount());
}

TEST_F(BatAdsRefillUnblindedTokensTest, IssuersPublicKeyMismatch) {
  // Arrange
  const URLResponseMap url_responses = GetValidUrlResonses();
  MockUrlResponses(ads_client_mock_, url_responses);

  const std::vector<privacy::cbr::Token> tokens = GetTokens();
  ON_CALL(*token_generator_mock_, Generate(_)).WillByDefault(Return(tokens));

  const IssuersInfo issuers =
      BuildIssuers(7'200'000,
                   {{"JsvJluEN35bJBgJWTdW/8dAgPrrTM1I1pXga+o7cllo=", 0.0},
                    {"hKjGQd7WAXs0lcdf+SCHCTKsBLWtKaEubwlK4YA1NkA=", 0.0}},
                   {{"JiwFR2EU/Adf1lgox+xqOVPuc6a/rxdy/LguFG5eaXg=", 0.0},
                    {"bPE1QE65mkIgytffeu7STOfly+x10BXCGuk5pVlOHQU=", 0.1}});

  SetIssuers(issuers);

  // Act
  EXPECT_CALL(*refill_unblinded_tokens_delegate_mock_,
              OnDidRefillUnblindedTokens())
      .Times(0);

  EXPECT_CALL(*refill_unblinded_tokens_delegate_mock_,
              OnFailedToRefillUnblindedTokens());

  EXPECT_CALL(*refill_unblinded_tokens_delegate_mock_,
              OnWillRetryRefillingUnblindedTokens(_))
      .Times(0);

  EXPECT_CALL(*refill_unblinded_tokens_delegate_mock_,
              OnDidRetryRefillingUnblindedTokens())
      .Times(0);

  const WalletInfo wallet = GetWalletForTesting();
  refill_unblinded_tokens_->MaybeRefill(wallet);

  // Assert
  EXPECT_EQ(0, privacy::UnblindedTokenCount());
}

TEST_F(BatAdsRefillUnblindedTokensTest, InvalidIssuersFormat) {
  // Arrange

  // Act
  EXPECT_CALL(*refill_unblinded_tokens_delegate_mock_,
              OnDidRefillUnblindedTokens())
      .Times(0);

  EXPECT_CALL(*refill_unblinded_tokens_delegate_mock_,
              OnFailedToRefillUnblindedTokens());

  EXPECT_CALL(*refill_unblinded_tokens_delegate_mock_,
              OnWillRetryRefillingUnblindedTokens(_))
      .Times(0);

  EXPECT_CALL(*refill_unblinded_tokens_delegate_mock_,
              OnDidRetryRefillingUnblindedTokens())
      .Times(0);

  const WalletInfo wallet = GetWalletForTesting();
  refill_unblinded_tokens_->MaybeRefill(wallet);

  // Assert
  EXPECT_EQ(0, privacy::UnblindedTokenCount());
}

TEST_F(BatAdsRefillUnblindedTokensTest, InvalidWallet) {
  // Arrange

  // Act
  EXPECT_CALL(*refill_unblinded_tokens_delegate_mock_,
              OnDidRefillUnblindedTokens())
      .Times(0);

  EXPECT_CALL(*refill_unblinded_tokens_delegate_mock_,
              OnFailedToRefillUnblindedTokens());

  EXPECT_CALL(*refill_unblinded_tokens_delegate_mock_,
              OnWillRetryRefillingUnblindedTokens(_))
      .Times(0);

  EXPECT_CALL(*refill_unblinded_tokens_delegate_mock_,
              OnDidRetryRefillingUnblindedTokens())
      .Times(0);

  const WalletInfo invalid_wallet;
  refill_unblinded_tokens_->MaybeRefill(invalid_wallet);

  // Assert
  EXPECT_EQ(0, privacy::UnblindedTokenCount());
}

TEST_F(BatAdsRefillUnblindedTokensTest,
       RetryRequestSignedTokensAfterInternalServerError) {
  // Arrange
  const URLResponseMap url_responses = {
      {// Request signed tokens
       "/v3/confirmation/token/27a39b2f-9b2e-4eb0-bbb2-2f84447496e7",
       {{net::HTTP_INTERNAL_SERVER_ERROR, {}}, {net::HTTP_CREATED, R"(
            {
              "nonce": "2f0e2891-e7a5-4262-835b-550b13e58e5c"
            }
          )"}}},
      {// Get signed tokens
       "/v3/confirmation/token/"
       "27a39b2f-9b2e-4eb0-bbb2-2f84447496e7?nonce=2f0e2891-e7a5-4262-835b-"
       "550b13e58e5c",
       {{net::HTTP_OK, R"(
            {
              "batchProof": "BnqmsPk3PsQXVhcCE8YALSE8O+LVqOWabzCuyCTSgQjwAb3iAKrqDV3/zWKdU5TRoqzr32pyPyaS3xFI2iVmAw==",
              "signedTokens": [
                "fD5YfqudgGrfn+oHpwPsF7COcPrCTLsYX70wa+EE+gg=",
                "OOPCQu4K+hfE7YaYnI4SyNI1KTIfNR71rIuZKs/9rE8=",
                "4kCHwIqcMuptlWqHNqGVpSBB5og8h5ooIQkno+qV0j4=",
                "/lNHOB5ISVVNvoTkS0n4PhDynjYJxKYwXnaDVfzmGSI=",
                "+ADYC6BAjtbrULLhXoBJM6mK7RPAyYUBA37Dfz223A8=",
                "ipBrQYPynDtfMVH4COUqZTUm/7Cs5j+4f2v+w1s0H20=",
                "Jrmctnj+ixdK3xUq+0eLklQsyofptcf9paHQrVD20QE=",
                "MMxS2Hdx3y6l2jWcBf1fMKxwAWN215S4CD/BPJ57oTA=",
                "oPI2nQ8Xu5cS8dmLfDynFjWaxxGgLzYX++qUdgLWxxU=",
                "mk+RCIjgRyqsFDG6Sukg7Sqq9ke7DheF8ID3QJqdCi8=",
                "OlKDho69Ulh+s/6KF8eS9LG3I58Aq3mgfPErr8AEo1s=",
                "pnZk5XlLuED7I/sYNYOedBqLvg9KAC1Tw4poxfojFBg=",
                "2mL4YIz3VFtdrHBpBUQLIPlsXkvfpqneMCneVDqDgBI=",
                "QPG8e94mNMUgeueC2h+ANRfnkjkG5yli/hpPw8mFwRk=",
                "2OiY14D3B9nKW1ai/ACOx/VO+y/xWFcrXwGPvlGQGwY=",
                "hNe+AZ+QIkbkwfnkYKmuq4LFjJez9c8QXCONIHMa2yI=",
                "lhXQa087T1T8yt32rwlO0Y9K9i6A6ysJxaeoCpQsUXk=",
                "2BVub545mBdHJIZnotoHP2QIrSstOdAGeHkTk8PbsA4=",
                "cvsy/fUIwOYgbTvxWoAH+RjRjdBKvjpC0yS8V7TTAzo=",
                "UsWm27QlfxDFAXUKOyQd+QbzFniAo8KMAcb8ogQn3zk=",
                "LO9hDP7KfQFIFuw4y6qKolzZCQAjVUtGa6SEJ0WtH28=",
                "oLrrrpgKoz/L8cEG4J2VV9VSJF8QG4Gactshr1WwZXQ=",
                "DrtwKP5kQEey3uOZvQzjqCTT30elIrLRvw3PIBqSdg4=",
                "mBxJCg3ClDS2IiJePXsv6KK6eQCY1yXvOi8m0/54uRg=",
                "9p4vrVEEIEnmreI1gy2JHvVtunHJjqT+oxUmwidJDlQ=",
                "VBMfinFy5m7jXqv1LPVqSvAn4mhntpFZ/PyS4eoJmiQ=",
                "om0eBmPqhiswq66mRdfgyzyPG/n/1jJXS5vLRMB1zTA=",
                "vs1t2qaE0RptGUHoc6CC1yNJAHJhs7g5Plwpk2hhwgQ=",
                "GLtViGiHvY6DnWT3OQ65JTBoCu4uv+S0MCvm97VJWkA=",
                "0tKtV02T7yomO6tb3D5rYr/UHQy6rITYVygqUMF+1Hk=",
                "SG4OS7WthG8Toff8NHIfBafHTB/8stW+bGrnt9ZUCWQ=",
                "/JaxZ/fXY8/bZdhL33sorUof6qDfhRHqJn7FGXNg5Wg=",
                "8vZlB2XPZF4vMn4K6FSNjvk5aZ4G6iCVSoU+Rh6Kgx0=",
                "xIbWr9fuB2qr1Xr6r5vMIzeOraIiLB338MSWl8RjATE=",
                "xDYuZfPQiVA5sW75Z4M+1fmtYvifXTEYX/BWsA701ks=",
                "2l6UgMUlJBEY2R+CTJBX5M2l552bkEPECu7YMP2OAy0=",
                "uLrkxPY2eBn3FJ4fkuklZimz455rCzCzvcFYBmVWFUQ=",
                "4EbkdgBc1IvhlGfaXuQxthQl3+wtM/qMdmnyfJE/MVc=",
                "RAlXUOypctgZ+EIBiqOVmnSW5VroQfT1aGqk0o/wR0s=",
                "tEehxSWHMtdBzl5mZWNSx9CmGzu1vrWm+YwdjvnNcUw=",
                "NF8qNh56/nXBPITAakis/FBUbNYlJQZ9ngR34VjJkiE=",
                "qrPGZKEmgnLMON6akKR2GR3omiPNBLnvB0f5Mh8EMVY=",
                "2A0rAiadKERas5Nb4d7UpBEMd15H8CF6R4a+E7QnPCk=",
                "MnS9QD/JJfsMWqZgXceAFDo/E60YQyd52Km+3jPCzhg=",
                "0rTQsecKlhLU9v6SBZuJbrUU+Yd5hx97EanqrZw6UV8=",
                "qIwAZMezVrm7ufJoTqSF+DEwOBXVdwf4zm0GMQZiZzI=",
                "6pYOa+9Kht35CGvrGEsbFqu3mxgzVTZzFJWytq0MpjU=",
                "xGd6OV9+IPhKkXgmn7AP6TcTZSANmweCS+PlgZLjQRA=",
                "tlX/IqPpfSvJfwCZzIZonVx3hln15RZpsifkiMxr53s=",
                "mML4eqBLA9XjZTqhoxVA6lVbMcjL54GqluGGPmMhWQA="
              ],
              "publicKey": "crDVI1R6xHQZ4D9cQu4muVM5MaaM1QcOT4It8Y/CYlw="
            }
          )"}}}};
  MockUrlResponses(ads_client_mock_, url_responses);

  const std::vector<privacy::cbr::Token> tokens = GetTokens();
  ON_CALL(*token_generator_mock_, Generate(_)).WillByDefault(Return(tokens));

  BuildAndSetIssuers();

  // Act
  const InSequence seq;

  EXPECT_CALL(*refill_unblinded_tokens_delegate_mock_,
              OnFailedToRefillUnblindedTokens());

  EXPECT_CALL(*refill_unblinded_tokens_delegate_mock_,
              OnWillRetryRefillingUnblindedTokens(_));

  EXPECT_CALL(*refill_unblinded_tokens_delegate_mock_,
              OnDidRetryRefillingUnblindedTokens());

  EXPECT_CALL(*refill_unblinded_tokens_delegate_mock_,
              OnDidRefillUnblindedTokens());

  const WalletInfo wallet = GetWalletForTesting();
  refill_unblinded_tokens_->MaybeRefill(wallet);

  FastForwardClockToNextPendingTask();

  // Assert
  EXPECT_EQ(50, privacy::UnblindedTokenCount());
}

TEST_F(BatAdsRefillUnblindedTokensTest, RequestSignedTokensMissingNonce) {
  // Arrange
  const URLResponseMap url_responses = {
      {"/v3/confirmation/token/27a39b2f-9b2e-4eb0-bbb2-2f84447496e7",
       {{net::HTTP_CREATED, {}}}}};
  MockUrlResponses(ads_client_mock_, url_responses);

  const std::vector<privacy::cbr::Token> tokens = GetTokens();
  ON_CALL(*token_generator_mock_, Generate(_)).WillByDefault(Return(tokens));

  BuildAndSetIssuers();

  // Act
  EXPECT_CALL(*refill_unblinded_tokens_delegate_mock_,
              OnDidRefillUnblindedTokens())
      .Times(0);

  EXPECT_CALL(*refill_unblinded_tokens_delegate_mock_,
              OnFailedToRefillUnblindedTokens());

  EXPECT_CALL(*refill_unblinded_tokens_delegate_mock_,
              OnWillRetryRefillingUnblindedTokens(_))
      .Times(0);

  EXPECT_CALL(*refill_unblinded_tokens_delegate_mock_,
              OnDidRetryRefillingUnblindedTokens())
      .Times(0);

  const WalletInfo wallet = GetWalletForTesting();
  refill_unblinded_tokens_->MaybeRefill(wallet);

  // Assert
  EXPECT_EQ(0, privacy::UnblindedTokenCount());
}

TEST_F(BatAdsRefillUnblindedTokensTest,
       RetryGetSignedTokensAfterInternalServerError) {
  // Arrange
  const URLResponseMap url_responses = {
      {// Request signed tokens
       "/v3/confirmation/token/27a39b2f-9b2e-4eb0-bbb2-2f84447496e7",
       {{net::HTTP_CREATED, R"(
            {
              "nonce": "2f0e2891-e7a5-4262-835b-550b13e58e5c"
            }
          )"},
        {net::HTTP_CREATED, R"(
            {
              "nonce": "2f0e2891-e7a5-4262-835b-550b13e58e5c"
            }
          )"}}},
      {// Get signed tokens
       "/v3/confirmation/token/"
       "27a39b2f-9b2e-4eb0-bbb2-2f84447496e7?nonce=2f0e2891-e7a5-4262-835b-"
       "550b13e58e5c",
       {{net::HTTP_INTERNAL_SERVER_ERROR, {}}, {net::HTTP_OK, R"(
            {
              "batchProof": "BnqmsPk3PsQXVhcCE8YALSE8O+LVqOWabzCuyCTSgQjwAb3iAKrqDV3/zWKdU5TRoqzr32pyPyaS3xFI2iVmAw==",
              "signedTokens": [
                "fD5YfqudgGrfn+oHpwPsF7COcPrCTLsYX70wa+EE+gg=",
                "OOPCQu4K+hfE7YaYnI4SyNI1KTIfNR71rIuZKs/9rE8=",
                "4kCHwIqcMuptlWqHNqGVpSBB5og8h5ooIQkno+qV0j4=",
                "/lNHOB5ISVVNvoTkS0n4PhDynjYJxKYwXnaDVfzmGSI=",
                "+ADYC6BAjtbrULLhXoBJM6mK7RPAyYUBA37Dfz223A8=",
                "ipBrQYPynDtfMVH4COUqZTUm/7Cs5j+4f2v+w1s0H20=",
                "Jrmctnj+ixdK3xUq+0eLklQsyofptcf9paHQrVD20QE=",
                "MMxS2Hdx3y6l2jWcBf1fMKxwAWN215S4CD/BPJ57oTA=",
                "oPI2nQ8Xu5cS8dmLfDynFjWaxxGgLzYX++qUdgLWxxU=",
                "mk+RCIjgRyqsFDG6Sukg7Sqq9ke7DheF8ID3QJqdCi8=",
                "OlKDho69Ulh+s/6KF8eS9LG3I58Aq3mgfPErr8AEo1s=",
                "pnZk5XlLuED7I/sYNYOedBqLvg9KAC1Tw4poxfojFBg=",
                "2mL4YIz3VFtdrHBpBUQLIPlsXkvfpqneMCneVDqDgBI=",
                "QPG8e94mNMUgeueC2h+ANRfnkjkG5yli/hpPw8mFwRk=",
                "2OiY14D3B9nKW1ai/ACOx/VO+y/xWFcrXwGPvlGQGwY=",
                "hNe+AZ+QIkbkwfnkYKmuq4LFjJez9c8QXCONIHMa2yI=",
                "lhXQa087T1T8yt32rwlO0Y9K9i6A6ysJxaeoCpQsUXk=",
                "2BVub545mBdHJIZnotoHP2QIrSstOdAGeHkTk8PbsA4=",
                "cvsy/fUIwOYgbTvxWoAH+RjRjdBKvjpC0yS8V7TTAzo=",
                "UsWm27QlfxDFAXUKOyQd+QbzFniAo8KMAcb8ogQn3zk=",
                "LO9hDP7KfQFIFuw4y6qKolzZCQAjVUtGa6SEJ0WtH28=",
                "oLrrrpgKoz/L8cEG4J2VV9VSJF8QG4Gactshr1WwZXQ=",
                "DrtwKP5kQEey3uOZvQzjqCTT30elIrLRvw3PIBqSdg4=",
                "mBxJCg3ClDS2IiJePXsv6KK6eQCY1yXvOi8m0/54uRg=",
                "9p4vrVEEIEnmreI1gy2JHvVtunHJjqT+oxUmwidJDlQ=",
                "VBMfinFy5m7jXqv1LPVqSvAn4mhntpFZ/PyS4eoJmiQ=",
                "om0eBmPqhiswq66mRdfgyzyPG/n/1jJXS5vLRMB1zTA=",
                "vs1t2qaE0RptGUHoc6CC1yNJAHJhs7g5Plwpk2hhwgQ=",
                "GLtViGiHvY6DnWT3OQ65JTBoCu4uv+S0MCvm97VJWkA=",
                "0tKtV02T7yomO6tb3D5rYr/UHQy6rITYVygqUMF+1Hk=",
                "SG4OS7WthG8Toff8NHIfBafHTB/8stW+bGrnt9ZUCWQ=",
                "/JaxZ/fXY8/bZdhL33sorUof6qDfhRHqJn7FGXNg5Wg=",
                "8vZlB2XPZF4vMn4K6FSNjvk5aZ4G6iCVSoU+Rh6Kgx0=",
                "xIbWr9fuB2qr1Xr6r5vMIzeOraIiLB338MSWl8RjATE=",
                "xDYuZfPQiVA5sW75Z4M+1fmtYvifXTEYX/BWsA701ks=",
                "2l6UgMUlJBEY2R+CTJBX5M2l552bkEPECu7YMP2OAy0=",
                "uLrkxPY2eBn3FJ4fkuklZimz455rCzCzvcFYBmVWFUQ=",
                "4EbkdgBc1IvhlGfaXuQxthQl3+wtM/qMdmnyfJE/MVc=",
                "RAlXUOypctgZ+EIBiqOVmnSW5VroQfT1aGqk0o/wR0s=",
                "tEehxSWHMtdBzl5mZWNSx9CmGzu1vrWm+YwdjvnNcUw=",
                "NF8qNh56/nXBPITAakis/FBUbNYlJQZ9ngR34VjJkiE=",
                "qrPGZKEmgnLMON6akKR2GR3omiPNBLnvB0f5Mh8EMVY=",
                "2A0rAiadKERas5Nb4d7UpBEMd15H8CF6R4a+E7QnPCk=",
                "MnS9QD/JJfsMWqZgXceAFDo/E60YQyd52Km+3jPCzhg=",
                "0rTQsecKlhLU9v6SBZuJbrUU+Yd5hx97EanqrZw6UV8=",
                "qIwAZMezVrm7ufJoTqSF+DEwOBXVdwf4zm0GMQZiZzI=",
                "6pYOa+9Kht35CGvrGEsbFqu3mxgzVTZzFJWytq0MpjU=",
                "xGd6OV9+IPhKkXgmn7AP6TcTZSANmweCS+PlgZLjQRA=",
                "tlX/IqPpfSvJfwCZzIZonVx3hln15RZpsifkiMxr53s=",
                "mML4eqBLA9XjZTqhoxVA6lVbMcjL54GqluGGPmMhWQA="
              ],
              "publicKey": "crDVI1R6xHQZ4D9cQu4muVM5MaaM1QcOT4It8Y/CYlw="
            }
          )"}}}};
  MockUrlResponses(ads_client_mock_, url_responses);

  const std::vector<privacy::cbr::Token> tokens = GetTokens();
  ON_CALL(*token_generator_mock_, Generate(_)).WillByDefault(Return(tokens));

  BuildAndSetIssuers();

  // Act
  const InSequence seq;

  EXPECT_CALL(*refill_unblinded_tokens_delegate_mock_,
              OnFailedToRefillUnblindedTokens());

  EXPECT_CALL(*refill_unblinded_tokens_delegate_mock_,
              OnWillRetryRefillingUnblindedTokens(_));

  EXPECT_CALL(*refill_unblinded_tokens_delegate_mock_,
              OnDidRetryRefillingUnblindedTokens());

  EXPECT_CALL(*refill_unblinded_tokens_delegate_mock_,
              OnDidRefillUnblindedTokens());

  const WalletInfo wallet = GetWalletForTesting();
  refill_unblinded_tokens_->MaybeRefill(wallet);

  FastForwardClockToNextPendingTask();

  // Assert
  EXPECT_EQ(50, privacy::UnblindedTokenCount());
}

TEST_F(BatAdsRefillUnblindedTokensTest, GetSignedTokensInvalidResponse) {
  // Arrange
  const URLResponseMap url_responses = {
      {// Request signed tokens
       "/v3/confirmation/token/27a39b2f-9b2e-4eb0-bbb2-2f84447496e7",
       {{net::HTTP_CREATED, R"(
            {
              "nonce": "2f0e2891-e7a5-4262-835b-550b13e58e5c"
            }
          )"}}},
      {// Get signed tokens
       "/v3/confirmation/token/"
       "27a39b2f-9b2e-4eb0-bbb2-2f84447496e7?nonce=2f0e2891-e7a5-4262-835b-"
       "550b13e58e5c",
       {{net::HTTP_OK, "invalid_json"}}}};
  MockUrlResponses(ads_client_mock_, url_responses);

  const std::vector<privacy::cbr::Token> tokens = GetTokens();
  ON_CALL(*token_generator_mock_, Generate(_)).WillByDefault(Return(tokens));

  BuildAndSetIssuers();

  // Act
  EXPECT_CALL(*refill_unblinded_tokens_delegate_mock_,
              OnDidRefillUnblindedTokens())
      .Times(0);

  EXPECT_CALL(*refill_unblinded_tokens_delegate_mock_,
              OnFailedToRefillUnblindedTokens());

  EXPECT_CALL(*refill_unblinded_tokens_delegate_mock_,
              OnWillRetryRefillingUnblindedTokens(_))
      .Times(0);

  EXPECT_CALL(*refill_unblinded_tokens_delegate_mock_,
              OnDidRetryRefillingUnblindedTokens())
      .Times(0);

  const WalletInfo wallet = GetWalletForTesting();
  refill_unblinded_tokens_->MaybeRefill(wallet);

  // Assert
  EXPECT_EQ(0, privacy::UnblindedTokenCount());
}

TEST_F(BatAdsRefillUnblindedTokensTest, GetSignedTokensMissingPublicKey) {
  // Arrange
  const URLResponseMap url_responses = {
      {// Request signed tokens
       "/v3/confirmation/token/27a39b2f-9b2e-4eb0-bbb2-2f84447496e7",
       {{net::HTTP_CREATED, R"(
            {
              "nonce": "2f0e2891-e7a5-4262-835b-550b13e58e5c"
            }
          )"}}},
      {// Get signed tokens
       "/v3/confirmation/token/"
       "27a39b2f-9b2e-4eb0-bbb2-2f84447496e7?nonce=2f0e2891-e7a5-4262-835b-"
       "550b13e58e5c",
       {{net::HTTP_OK, R"(
            {
              "batchProof": "BnqmsPk3PsQXVhcCE8YALSE8O+LVqOWabzCuyCTSgQjwAb3iAKrqDV3/zWKdU5TRoqzr32pyPyaS3xFI2iVmAw==",
              "signedTokens": [
                "fD5YfqudgGrfn+oHpwPsF7COcPrCTLsYX70wa+EE+gg=",
                "OOPCQu4K+hfE7YaYnI4SyNI1KTIfNR71rIuZKs/9rE8=",
                "4kCHwIqcMuptlWqHNqGVpSBB5og8h5ooIQkno+qV0j4=",
                "/lNHOB5ISVVNvoTkS0n4PhDynjYJxKYwXnaDVfzmGSI=",
                "+ADYC6BAjtbrULLhXoBJM6mK7RPAyYUBA37Dfz223A8=",
                "ipBrQYPynDtfMVH4COUqZTUm/7Cs5j+4f2v+w1s0H20=",
                "Jrmctnj+ixdK3xUq+0eLklQsyofptcf9paHQrVD20QE=",
                "MMxS2Hdx3y6l2jWcBf1fMKxwAWN215S4CD/BPJ57oTA=",
                "oPI2nQ8Xu5cS8dmLfDynFjWaxxGgLzYX++qUdgLWxxU=",
                "mk+RCIjgRyqsFDG6Sukg7Sqq9ke7DheF8ID3QJqdCi8=",
                "OlKDho69Ulh+s/6KF8eS9LG3I58Aq3mgfPErr8AEo1s=",
                "pnZk5XlLuED7I/sYNYOedBqLvg9KAC1Tw4poxfojFBg=",
                "2mL4YIz3VFtdrHBpBUQLIPlsXkvfpqneMCneVDqDgBI=",
                "QPG8e94mNMUgeueC2h+ANRfnkjkG5yli/hpPw8mFwRk=",
                "2OiY14D3B9nKW1ai/ACOx/VO+y/xWFcrXwGPvlGQGwY=",
                "hNe+AZ+QIkbkwfnkYKmuq4LFjJez9c8QXCONIHMa2yI=",
                "lhXQa087T1T8yt32rwlO0Y9K9i6A6ysJxaeoCpQsUXk=",
                "2BVub545mBdHJIZnotoHP2QIrSstOdAGeHkTk8PbsA4=",
                "cvsy/fUIwOYgbTvxWoAH+RjRjdBKvjpC0yS8V7TTAzo=",
                "UsWm27QlfxDFAXUKOyQd+QbzFniAo8KMAcb8ogQn3zk=",
                "LO9hDP7KfQFIFuw4y6qKolzZCQAjVUtGa6SEJ0WtH28=",
                "oLrrrpgKoz/L8cEG4J2VV9VSJF8QG4Gactshr1WwZXQ=",
                "DrtwKP5kQEey3uOZvQzjqCTT30elIrLRvw3PIBqSdg4=",
                "mBxJCg3ClDS2IiJePXsv6KK6eQCY1yXvOi8m0/54uRg=",
                "9p4vrVEEIEnmreI1gy2JHvVtunHJjqT+oxUmwidJDlQ=",
                "VBMfinFy5m7jXqv1LPVqSvAn4mhntpFZ/PyS4eoJmiQ=",
                "om0eBmPqhiswq66mRdfgyzyPG/n/1jJXS5vLRMB1zTA=",
                "vs1t2qaE0RptGUHoc6CC1yNJAHJhs7g5Plwpk2hhwgQ=",
                "GLtViGiHvY6DnWT3OQ65JTBoCu4uv+S0MCvm97VJWkA=",
                "0tKtV02T7yomO6tb3D5rYr/UHQy6rITYVygqUMF+1Hk=",
                "SG4OS7WthG8Toff8NHIfBafHTB/8stW+bGrnt9ZUCWQ=",
                "/JaxZ/fXY8/bZdhL33sorUof6qDfhRHqJn7FGXNg5Wg=",
                "8vZlB2XPZF4vMn4K6FSNjvk5aZ4G6iCVSoU+Rh6Kgx0=",
                "xIbWr9fuB2qr1Xr6r5vMIzeOraIiLB338MSWl8RjATE=",
                "xDYuZfPQiVA5sW75Z4M+1fmtYvifXTEYX/BWsA701ks=",
                "2l6UgMUlJBEY2R+CTJBX5M2l552bkEPECu7YMP2OAy0=",
                "uLrkxPY2eBn3FJ4fkuklZimz455rCzCzvcFYBmVWFUQ=",
                "4EbkdgBc1IvhlGfaXuQxthQl3+wtM/qMdmnyfJE/MVc=",
                "RAlXUOypctgZ+EIBiqOVmnSW5VroQfT1aGqk0o/wR0s=",
                "tEehxSWHMtdBzl5mZWNSx9CmGzu1vrWm+YwdjvnNcUw=",
                "NF8qNh56/nXBPITAakis/FBUbNYlJQZ9ngR34VjJkiE=",
                "qrPGZKEmgnLMON6akKR2GR3omiPNBLnvB0f5Mh8EMVY=",
                "2A0rAiadKERas5Nb4d7UpBEMd15H8CF6R4a+E7QnPCk=",
                "MnS9QD/JJfsMWqZgXceAFDo/E60YQyd52Km+3jPCzhg=",
                "0rTQsecKlhLU9v6SBZuJbrUU+Yd5hx97EanqrZw6UV8=",
                "qIwAZMezVrm7ufJoTqSF+DEwOBXVdwf4zm0GMQZiZzI=",
                "6pYOa+9Kht35CGvrGEsbFqu3mxgzVTZzFJWytq0MpjU=",
                "xGd6OV9+IPhKkXgmn7AP6TcTZSANmweCS+PlgZLjQRA=",
                "tlX/IqPpfSvJfwCZzIZonVx3hln15RZpsifkiMxr53s=",
                "mML4eqBLA9XjZTqhoxVA6lVbMcjL54GqluGGPmMhWQA="
              ]
            }
          )"}}}};
  MockUrlResponses(ads_client_mock_, url_responses);

  const std::vector<privacy::cbr::Token> tokens = GetTokens();
  ON_CALL(*token_generator_mock_, Generate(_)).WillByDefault(Return(tokens));

  BuildAndSetIssuers();

  // Act
  EXPECT_CALL(*refill_unblinded_tokens_delegate_mock_,
              OnDidRefillUnblindedTokens())
      .Times(0);

  EXPECT_CALL(*refill_unblinded_tokens_delegate_mock_,
              OnFailedToRefillUnblindedTokens());

  EXPECT_CALL(*refill_unblinded_tokens_delegate_mock_,
              OnWillRetryRefillingUnblindedTokens(_))
      .Times(0);

  EXPECT_CALL(*refill_unblinded_tokens_delegate_mock_,
              OnDidRetryRefillingUnblindedTokens())
      .Times(0);

  const WalletInfo wallet = GetWalletForTesting();
  refill_unblinded_tokens_->MaybeRefill(wallet);

  // Assert
  EXPECT_EQ(0, privacy::UnblindedTokenCount());
}

TEST_F(BatAdsRefillUnblindedTokensTest, GetSignedTokensMissingBatchProofDleq) {
  // Arrange
  const URLResponseMap url_responses = {
      {// Request signed tokens
       "/v3/confirmation/token/27a39b2f-9b2e-4eb0-bbb2-2f84447496e7",
       {{net::HTTP_CREATED, R"(
            {
              "nonce": "2f0e2891-e7a5-4262-835b-550b13e58e5c"
            }
          )"}}},
      {// Get signed tokens
       "/v3/confirmation/token/"
       "27a39b2f-9b2e-4eb0-bbb2-2f84447496e7?nonce=2f0e2891-e7a5-4262-835b-"
       "550b13e58e5c",
       {{net::HTTP_OK, R"(
            {
              "signedTokens": [
                "fD5YfqudgGrfn+oHpwPsF7COcPrCTLsYX70wa+EE+gg=",
                "OOPCQu4K+hfE7YaYnI4SyNI1KTIfNR71rIuZKs/9rE8=",
                "4kCHwIqcMuptlWqHNqGVpSBB5og8h5ooIQkno+qV0j4=",
                "/lNHOB5ISVVNvoTkS0n4PhDynjYJxKYwXnaDVfzmGSI=",
                "+ADYC6BAjtbrULLhXoBJM6mK7RPAyYUBA37Dfz223A8=",
                "ipBrQYPynDtfMVH4COUqZTUm/7Cs5j+4f2v+w1s0H20=",
                "Jrmctnj+ixdK3xUq+0eLklQsyofptcf9paHQrVD20QE=",
                "MMxS2Hdx3y6l2jWcBf1fMKxwAWN215S4CD/BPJ57oTA=",
                "oPI2nQ8Xu5cS8dmLfDynFjWaxxGgLzYX++qUdgLWxxU=",
                "mk+RCIjgRyqsFDG6Sukg7Sqq9ke7DheF8ID3QJqdCi8=",
                "OlKDho69Ulh+s/6KF8eS9LG3I58Aq3mgfPErr8AEo1s=",
                "pnZk5XlLuED7I/sYNYOedBqLvg9KAC1Tw4poxfojFBg=",
                "2mL4YIz3VFtdrHBpBUQLIPlsXkvfpqneMCneVDqDgBI=",
                "QPG8e94mNMUgeueC2h+ANRfnkjkG5yli/hpPw8mFwRk=",
                "2OiY14D3B9nKW1ai/ACOx/VO+y/xWFcrXwGPvlGQGwY=",
                "hNe+AZ+QIkbkwfnkYKmuq4LFjJez9c8QXCONIHMa2yI=",
                "lhXQa087T1T8yt32rwlO0Y9K9i6A6ysJxaeoCpQsUXk=",
                "2BVub545mBdHJIZnotoHP2QIrSstOdAGeHkTk8PbsA4=",
                "cvsy/fUIwOYgbTvxWoAH+RjRjdBKvjpC0yS8V7TTAzo=",
                "UsWm27QlfxDFAXUKOyQd+QbzFniAo8KMAcb8ogQn3zk=",
                "LO9hDP7KfQFIFuw4y6qKolzZCQAjVUtGa6SEJ0WtH28=",
                "oLrrrpgKoz/L8cEG4J2VV9VSJF8QG4Gactshr1WwZXQ=",
                "DrtwKP5kQEey3uOZvQzjqCTT30elIrLRvw3PIBqSdg4=",
                "mBxJCg3ClDS2IiJePXsv6KK6eQCY1yXvOi8m0/54uRg=",
                "9p4vrVEEIEnmreI1gy2JHvVtunHJjqT+oxUmwidJDlQ=",
                "VBMfinFy5m7jXqv1LPVqSvAn4mhntpFZ/PyS4eoJmiQ=",
                "om0eBmPqhiswq66mRdfgyzyPG/n/1jJXS5vLRMB1zTA=",
                "vs1t2qaE0RptGUHoc6CC1yNJAHJhs7g5Plwpk2hhwgQ=",
                "GLtViGiHvY6DnWT3OQ65JTBoCu4uv+S0MCvm97VJWkA=",
                "0tKtV02T7yomO6tb3D5rYr/UHQy6rITYVygqUMF+1Hk=",
                "SG4OS7WthG8Toff8NHIfBafHTB/8stW+bGrnt9ZUCWQ=",
                "/JaxZ/fXY8/bZdhL33sorUof6qDfhRHqJn7FGXNg5Wg=",
                "8vZlB2XPZF4vMn4K6FSNjvk5aZ4G6iCVSoU+Rh6Kgx0=",
                "xIbWr9fuB2qr1Xr6r5vMIzeOraIiLB338MSWl8RjATE=",
                "xDYuZfPQiVA5sW75Z4M+1fmtYvifXTEYX/BWsA701ks=",
                "2l6UgMUlJBEY2R+CTJBX5M2l552bkEPECu7YMP2OAy0=",
                "uLrkxPY2eBn3FJ4fkuklZimz455rCzCzvcFYBmVWFUQ=",
                "4EbkdgBc1IvhlGfaXuQxthQl3+wtM/qMdmnyfJE/MVc=",
                "RAlXUOypctgZ+EIBiqOVmnSW5VroQfT1aGqk0o/wR0s=",
                "tEehxSWHMtdBzl5mZWNSx9CmGzu1vrWm+YwdjvnNcUw=",
                "NF8qNh56/nXBPITAakis/FBUbNYlJQZ9ngR34VjJkiE=",
                "qrPGZKEmgnLMON6akKR2GR3omiPNBLnvB0f5Mh8EMVY=",
                "2A0rAiadKERas5Nb4d7UpBEMd15H8CF6R4a+E7QnPCk=",
                "MnS9QD/JJfsMWqZgXceAFDo/E60YQyd52Km+3jPCzhg=",
                "0rTQsecKlhLU9v6SBZuJbrUU+Yd5hx97EanqrZw6UV8=",
                "qIwAZMezVrm7ufJoTqSF+DEwOBXVdwf4zm0GMQZiZzI=",
                "6pYOa+9Kht35CGvrGEsbFqu3mxgzVTZzFJWytq0MpjU=",
                "xGd6OV9+IPhKkXgmn7AP6TcTZSANmweCS+PlgZLjQRA=",
                "tlX/IqPpfSvJfwCZzIZonVx3hln15RZpsifkiMxr53s=",
                "mML4eqBLA9XjZTqhoxVA6lVbMcjL54GqluGGPmMhWQA="
              ],
              "publicKey": "crDVI1R6xHQZ4D9cQu4muVM5MaaM1QcOT4It8Y/CYlw="
            }
          )"}}}};
  MockUrlResponses(ads_client_mock_, url_responses);

  const std::vector<privacy::cbr::Token> tokens = GetTokens();
  ON_CALL(*token_generator_mock_, Generate(_)).WillByDefault(Return(tokens));

  BuildAndSetIssuers();

  // Act
  EXPECT_CALL(*refill_unblinded_tokens_delegate_mock_,
              OnDidRefillUnblindedTokens())
      .Times(0);

  EXPECT_CALL(*refill_unblinded_tokens_delegate_mock_,
              OnFailedToRefillUnblindedTokens());

  EXPECT_CALL(*refill_unblinded_tokens_delegate_mock_,
              OnWillRetryRefillingUnblindedTokens(_))
      .Times(0);

  EXPECT_CALL(*refill_unblinded_tokens_delegate_mock_,
              OnDidRetryRefillingUnblindedTokens())
      .Times(0);

  const WalletInfo wallet = GetWalletForTesting();
  refill_unblinded_tokens_->MaybeRefill(wallet);

  // Assert
  EXPECT_EQ(0, privacy::UnblindedTokenCount());
}

TEST_F(BatAdsRefillUnblindedTokensTest, GetSignedTokensMissingSignedTokens) {
  // Arrange
  const URLResponseMap url_responses = {
      {// Request signed tokens
       "/v3/confirmation/token/27a39b2f-9b2e-4eb0-bbb2-2f84447496e7",
       {{net::HTTP_CREATED, R"(
            {
              "nonce": "2f0e2891-e7a5-4262-835b-550b13e58e5c"
            }
          )"}}},
      {// Get signed tokens
       "/v3/confirmation/token/"
       "27a39b2f-9b2e-4eb0-bbb2-2f84447496e7?nonce=2f0e2891-e7a5-4262-835b-"
       "550b13e58e5c",
       {{net::HTTP_OK, R"(
            {
              "batchProof": "BnqmsPk3PsQXVhcCE8YALSE8O+LVqOWabzCuyCTSgQjwAb3iAKrqDV3/zWKdU5TRoqzr32pyPyaS3xFI2iVmAw==",
              "publicKey": "crDVI1R6xHQZ4D9cQu4muVM5MaaM1QcOT4It8Y/CYlw="
            }
          )"}}}};
  MockUrlResponses(ads_client_mock_, url_responses);

  const std::vector<privacy::cbr::Token> tokens = GetTokens();
  ON_CALL(*token_generator_mock_, Generate(_)).WillByDefault(Return(tokens));

  BuildAndSetIssuers();

  // Act
  EXPECT_CALL(*refill_unblinded_tokens_delegate_mock_,
              OnDidRefillUnblindedTokens())
      .Times(0);

  EXPECT_CALL(*refill_unblinded_tokens_delegate_mock_,
              OnFailedToRefillUnblindedTokens());

  EXPECT_CALL(*refill_unblinded_tokens_delegate_mock_,
              OnWillRetryRefillingUnblindedTokens(_))
      .Times(0);

  EXPECT_CALL(*refill_unblinded_tokens_delegate_mock_,
              OnDidRetryRefillingUnblindedTokens())
      .Times(0);

  const WalletInfo wallet = GetWalletForTesting();
  refill_unblinded_tokens_->MaybeRefill(wallet);

  // Assert
  EXPECT_EQ(0, privacy::UnblindedTokenCount());
}

TEST_F(BatAdsRefillUnblindedTokensTest, GetInvalidSignedTokens) {
  // Arrange
  const URLResponseMap url_responses = {
      {// Request signed tokens
       "/v3/confirmation/token/27a39b2f-9b2e-4eb0-bbb2-2f84447496e7",
       {{net::HTTP_CREATED, R"(
            {
              "nonce": "2f0e2891-e7a5-4262-835b-550b13e58e5c"
            }
          )"}}},
      {// Get signed tokens
       "/v3/confirmation/token/"
       "27a39b2f-9b2e-4eb0-bbb2-2f84447496e7?nonce=2f0e2891-e7a5-4262-835b-"
       "550b13e58e5c",
       {{net::HTTP_OK, R"(
            {
              "batchProof": "BnqmsPk3PsQXVhcCE8YALSE8O+LVqOWabzCuyCTSgQjwAb3iAKrqDV3/zWKdU5TRoqzr32pyPyaS3xFI2iVmAw==",
              "signedTokens": [
                "gD5YfqudgGrfn+oHpwPsF7COcPrCTLsYX70wa+EE+gg=",
                "OOPCQu4K+hfE7YaYnI4SyNI1KTIfNR71rIuZKs/9rE8=",
                "4kCHwIqcMuptlWqHNqGVpSBB5og8h5ooIQkno+qV0j4=",
                "/lNHOB5ISVVNvoTkS0n4PhDynjYJxKYwXnaDVfzmGSI=",
                "+ADYC6BAjtbrULLhXoBJM6mK7RPAyYUBA37Dfz223A8=",
                "ipBrQYPynDtfMVH4COUqZTUm/7Cs5j+4f2v+w1s0H20=",
                "Jrmctnj+ixdK3xUq+0eLklQsyofptcf9paHQrVD20QE=",
                "MMxS2Hdx3y6l2jWcBf1fMKxwAWN215S4CD/BPJ57oTA=",
                "oPI2nQ8Xu5cS8dmLfDynFjWaxxGgLzYX++qUdgLWxxU=",
                "mk+RCIjgRyqsFDG6Sukg7Sqq9ke7DheF8ID3QJqdCi8=",
                "OlKDho69Ulh+s/6KF8eS9LG3I58Aq3mgfPErr8AEo1s=",
                "pnZk5XlLuED7I/sYNYOedBqLvg9KAC1Tw4poxfojFBg=",
                "2mL4YIz3VFtdrHBpBUQLIPlsXkvfpqneMCneVDqDgBI=",
                "QPG8e94mNMUgeueC2h+ANRfnkjkG5yli/hpPw8mFwRk=",
                "2OiY14D3B9nKW1ai/ACOx/VO+y/xWFcrXwGPvlGQGwY=",
                "hNe+AZ+QIkbkwfnkYKmuq4LFjJez9c8QXCONIHMa2yI=",
                "lhXQa087T1T8yt32rwlO0Y9K9i6A6ysJxaeoCpQsUXk=",
                "2BVub545mBdHJIZnotoHP2QIrSstOdAGeHkTk8PbsA4=",
                "cvsy/fUIwOYgbTvxWoAH+RjRjdBKvjpC0yS8V7TTAzo=",
                "UsWm27QlfxDFAXUKOyQd+QbzFniAo8KMAcb8ogQn3zk=",
                "LO9hDP7KfQFIFuw4y6qKolzZCQAjVUtGa6SEJ0WtH28=",
                "oLrrrpgKoz/L8cEG4J2VV9VSJF8QG4Gactshr1WwZXQ=",
                "DrtwKP5kQEey3uOZvQzjqCTT30elIrLRvw3PIBqSdg4=",
                "mBxJCg3ClDS2IiJePXsv6KK6eQCY1yXvOi8m0/54uRg=",
                "9p4vrVEEIEnmreI1gy2JHvVtunHJjqT+oxUmwidJDlQ=",
                "VBMfinFy5m7jXqv1LPVqSvAn4mhntpFZ/PyS4eoJmiQ=",
                "om0eBmPqhiswq66mRdfgyzyPG/n/1jJXS5vLRMB1zTA=",
                "vs1t2qaE0RptGUHoc6CC1yNJAHJhs7g5Plwpk2hhwgQ=",
                "GLtViGiHvY6DnWT3OQ65JTBoCu4uv+S0MCvm97VJWkA=",
                "0tKtV02T7yomO6tb3D5rYr/UHQy6rITYVygqUMF+1Hk=",
                "SG4OS7WthG8Toff8NHIfBafHTB/8stW+bGrnt9ZUCWQ=",
                "/JaxZ/fXY8/bZdhL33sorUof6qDfhRHqJn7FGXNg5Wg=",
                "8vZlB2XPZF4vMn4K6FSNjvk5aZ4G6iCVSoU+Rh6Kgx0=",
                "xIbWr9fuB2qr1Xr6r5vMIzeOraIiLB338MSWl8RjATE=",
                "xDYuZfPQiVA5sW75Z4M+1fmtYvifXTEYX/BWsA701ks=",
                "2l6UgMUlJBEY2R+CTJBX5M2l552bkEPECu7YMP2OAy0=",
                "uLrkxPY2eBn3FJ4fkuklZimz455rCzCzvcFYBmVWFUQ=",
                "4EbkdgBc1IvhlGfaXuQxthQl3+wtM/qMdmnyfJE/MVc=",
                "RAlXUOypctgZ+EIBiqOVmnSW5VroQfT1aGqk0o/wR0s=",
                "tEehxSWHMtdBzl5mZWNSx9CmGzu1vrWm+YwdjvnNcUw=",
                "NF8qNh56/nXBPITAakis/FBUbNYlJQZ9ngR34VjJkiE=",
                "qrPGZKEmgnLMON6akKR2GR3omiPNBLnvB0f5Mh8EMVY=",
                "2A0rAiadKERas5Nb4d7UpBEMd15H8CF6R4a+E7QnPCk=",
                "MnS9QD/JJfsMWqZgXceAFDo/E60YQyd52Km+3jPCzhg=",
                "0rTQsecKlhLU9v6SBZuJbrUU+Yd5hx97EanqrZw6UV8=",
                "qIwAZMezVrm7ufJoTqSF+DEwOBXVdwf4zm0GMQZiZzI=",
                "6pYOa+9Kht35CGvrGEsbFqu3mxgzVTZzFJWytq0MpjU=",
                "xGd6OV9+IPhKkXgmn7AP6TcTZSANmweCS+PlgZLjQRA=",
                "tlX/IqPpfSvJfwCZzIZonVx3hln15RZpsifkiMxr53s=",
                "mML4eqBLA9XjZTqhoxVA6lVbMcjL54GqluGGPmMhWQA="
              ],
              "publicKey": "crDVI1R6xHQZ4D9cQu4muVM5MaaM1QcOT4It8Y/CYlw="
            }
          )"}}}};
  MockUrlResponses(ads_client_mock_, url_responses);

  const std::vector<privacy::cbr::Token> tokens = GetTokens();
  ON_CALL(*token_generator_mock_, Generate(_)).WillByDefault(Return(tokens));

  BuildAndSetIssuers();

  // Act
  EXPECT_CALL(*refill_unblinded_tokens_delegate_mock_,
              OnDidRefillUnblindedTokens())
      .Times(0);

  EXPECT_CALL(*refill_unblinded_tokens_delegate_mock_,
              OnFailedToRefillUnblindedTokens());

  EXPECT_CALL(*refill_unblinded_tokens_delegate_mock_,
              OnWillRetryRefillingUnblindedTokens(_))
      .Times(0);

  EXPECT_CALL(*refill_unblinded_tokens_delegate_mock_,
              OnDidRetryRefillingUnblindedTokens())
      .Times(0);

  const WalletInfo wallet = GetWalletForTesting();
  refill_unblinded_tokens_->MaybeRefill(wallet);

  // Assert
  EXPECT_EQ(0, privacy::UnblindedTokenCount());
}

TEST_F(BatAdsRefillUnblindedTokensTest, DoNotRefillIfAboveTheMinimumThreshold) {
  // Arrange
  privacy::SetUnblindedTokens(50);

  BuildAndSetIssuers();

  // Act
  EXPECT_CALL(*refill_unblinded_tokens_delegate_mock_,
              OnDidRefillUnblindedTokens())
      .Times(0);

  EXPECT_CALL(*refill_unblinded_tokens_delegate_mock_,
              OnFailedToRefillUnblindedTokens())
      .Times(0);

  EXPECT_CALL(*refill_unblinded_tokens_delegate_mock_,
              OnWillRetryRefillingUnblindedTokens(_))
      .Times(0);

  EXPECT_CALL(*refill_unblinded_tokens_delegate_mock_,
              OnDidRetryRefillingUnblindedTokens())
      .Times(0);

  const WalletInfo wallet = GetWalletForTesting();
  refill_unblinded_tokens_->MaybeRefill(wallet);

  // Assert
  EXPECT_EQ(50, privacy::UnblindedTokenCount());
}

TEST_F(BatAdsRefillUnblindedTokensTest, RefillIfBelowTheMinimumThreshold) {
  // Arrange
  privacy::SetUnblindedTokens(19);

  const URLResponseMap url_responses = {
      {// Request signed tokens
       "/v3/confirmation/token/27a39b2f-9b2e-4eb0-bbb2-2f84447496e7",
       {{net::HTTP_CREATED, R"(
            {
              "nonce": "abcb67a5-0a73-43ec-bbf9-51288ba76bb7"
            }
          )"}}},
      {// Get signed tokens
       "/v3/confirmation/token/"
       "27a39b2f-9b2e-4eb0-bbb2-2f84447496e7?nonce=abcb67a5-0a73-43ec-bbf9-"
       "51288ba76bb7",
       {{net::HTTP_OK, R"(
            {
              "batchProof": "WQ3ijykF8smhAs+boORkMqgBN0gtn5Bd9bm47rAWtA60kJZtR/JfCSmTsMGjO110pDkaklRrnjYj5CrEH9DbDA==",
              "signedTokens": [
                "qqnNMJOm13B9Tu0kwEul6t24v7s89vnyJ7kavuvOrA0=",
                "JC+muES3lQoSORDgWbdkoh1z5UKsmmJKy3olDyFX9i4=",
                "XAHBMtNcOKwUdl1bsamTYGT3t00YmEGN6pBAJ29Nkl4=",
                "KrmWoq7LuCQ/QcK5WUN1xnX19ma6YQPTn5DBxJdzERo=",
                "fLTKUA0oz3iXjw9mJlb60O4hWHxlEorP/F77xMwH9VI=",
                "EntJAEqn/qIIs0enIQAwxIfpcjrm2AFl6x9fhJm6aG4=",
                "ACd7y9wYX5FNKJlhQfh3KSVYZuKd45QNTgSK5z6rB2A=",
                "XiYF1p+7iMzT5qcEhKiOCAg8++ow67rnQMHKK+RiyUI=",
                "3tEvU6mX7TpMGU5aVGVv0ApSEkwAS1jwIO1LK+MDhTs=",
                "kNsIWwWoTbHmdRtPGuWxJn9XOODGW20VbBJRlcl+DH0=",
                "ePtjvTzQQAPyQflsQP/SmFbdqXsN/b7rP82maEbwVBQ=",
                "2IM6WJcU1aA4vl1LCsXlzB0RiIor8XGFyQSTQMh+zFQ=",
                "4likGUXuGIDkOGrsBwTPFJvqP75y3ItLPTzF9ILhmmo=",
                "pEwZYE00b6JxtzzkeZIBSDxW5KYFIDD0UiCfUk4YLQI=",
                "vH319scmRKyBi2f9YIacIincZ2ZAZRHQYXXNB3KeVVI=",
                "IGyFB3wxqtuEIN3k6HiasfBAsarz/rRVB+FR4biv4yA=",
                "rvk76wWfS8zrySNgYAThSegIBX3yguSPHjlexP5d0x8=",
                "GI8TUzH/4LuZB0wEMVc6NZ4eskDgPeHrDBh1T2mmUEo=",
                "Qr0MNTvXOcB8/tiGqGQs4Ea+ORdXTEoVU7dNwCrRbDI=",
                "pPSYsksow7Hp2l2qUfCgxZdmY0IMxvwL9RNTg1tM0HY=",
                "KrQ6Gabd+8yM41IRDnIY2l9lywGSIyFLGLS+Lk6JnTc=",
                "gNoD0Dprnl0uHsqI/q8oE9DUojqV+OZwOqCUQ4y/3ik=",
                "WJVX7rhWhYckpRUROU5B3tLHKSfHX+zdwWp1mFtvdQ4=",
                "0rFH5BCbQlaoH8rTHFPa+ErCVBMV5AoQv16Qr/QymRY=",
                "2Nf+LpaM4gSs3gDG/hoEvtlk6reqO7NzrHpGwG6Exmw=",
                "MPeVBj4yeDQ4QyzXAaKPyrHj59F5IRSGrLmU8gng8R0=",
                "mtD3KwVntx6rDkmTkKTPVAXzZdormH79JXHx+kjJqhQ=",
                "fgQc7wl3L0o2PdvXWUjmsss2zfvRmvz7o084vm+TJBI=",
                "NLOsz0rLk4SVR1oalwgWgxci6aXCIltkihPbLXZmcRM=",
                "ruoGfGm2/nsRNKGORjS86ktiyA2w22bBKctTy58Yb1A=",
                "Zoub2Y6C8NFRd1rfAR6T34wbhB6f8JoHM9Rk/RbBQ0Y="
              ],
              "publicKey": "crDVI1R6xHQZ4D9cQu4muVM5MaaM1QcOT4It8Y/CYlw="
            }
          )"}}}};
  MockUrlResponses(ads_client_mock_, url_responses);

  ON_CALL(*token_generator_mock_, Generate(_))
      .WillByDefault(Return(privacy::GetTokens(31)));

  BuildAndSetIssuers();

  // Act
  EXPECT_CALL(*refill_unblinded_tokens_delegate_mock_,
              OnDidRefillUnblindedTokens());

  EXPECT_CALL(*refill_unblinded_tokens_delegate_mock_,
              OnFailedToRefillUnblindedTokens())
      .Times(0);

  EXPECT_CALL(*refill_unblinded_tokens_delegate_mock_,
              OnWillRetryRefillingUnblindedTokens(_))
      .Times(0);

  EXPECT_CALL(*refill_unblinded_tokens_delegate_mock_,
              OnDidRetryRefillingUnblindedTokens())
      .Times(0);

  const WalletInfo wallet = GetWalletForTesting();
  refill_unblinded_tokens_->MaybeRefill(wallet);

  // Assert
  EXPECT_EQ(50, privacy::UnblindedTokenCount());
}

}  // namespace ads
