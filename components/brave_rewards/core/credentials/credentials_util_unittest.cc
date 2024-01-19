/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "brave/components/brave_rewards/core/credentials/credentials_util.h"
#include "brave/components/brave_rewards/core/rewards_callbacks.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=PromotionUtilTest.*

namespace brave_rewards::internal {
namespace credential {

class PromotionUtilTest : public testing::Test {
 public:
  mojom::CredsBatch GetCredsBatch() {
    mojom::CredsBatch creds;

    creds.creds = R"([
          "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"
        ])";

    creds.blinded_creds = R"([
          "Gggq6QFD8GszbAO2Lsjms9QtaIUGWyfcAeeXmTN0Jw0=",
          "gLmphI+RsPU5yz+q2XYENT7/Uaff+XiycP2EVVBfigY=",
          "jlc4M10scQHkUGwOVHMgbwA8RYvX9AO0rmH4aMB3RF0=",
          "ZJO37nIin+EbTFljcBI3nlYnGtlrHuWK2qpL3T1Ncyk=",
          "KtBca7FBlQ4NViuUy5L6ATpnVUy+dDNqUEJA55jLznc=",
          "uO3p3VcWjme2yPyWv6oW3tJZkssQhbK4+v71I7ll72E=",
          "8GRAJi6QWLmHAObOstTNxwhyPpovIXMq/dQygYg1i1s=",
          "5sR8RMl3G4ccNTA3cAQi2MrRZw8oimtis0LpekYVaUc=",
          "AoSWLibiRwhJrDgSSloKxJmhuNpUV6ujYMHK89sNAWA=",
          "hMN0GZoIohkYZgctWUbUFWf8QVXZtjmWlIwliQtyjSs=",
          "pMfJ2H+AdeIXjhXCzNmAoVNdPETRPfpWcwrRU328MWk=",
          "QknZlZdJMzPqSdzklI/rXrJseg1lQwgDo7gYAH++m0U=",
          "Tn63o5SFWpPWkWf6U7Eo9cwDiO57mz8xkkqYU8cXmyY=",
          "DBYuiyLicXPdSBoSAvQ+NIZCpWcmKfln+VWGftSiACE=",
          "stq8pyNaIoHba5mUnxqnOT4hfrm8oHDSjUGnvwwlmFM=",
          "UqiXg4LVAIKswfKQ3R6QHKjLs4isaWPvFUM68pogYEs=",
          "Ih6uOcRgTilvlhIFd8EtbIsWC7rZGo9KTjJFlt7t9iA=",
          "FgxolUdYYtPiwskea6S62Eilbj3hFz3tRIN6UEsbVRw=",
          "cvpRU/QSuIpFslB3V92ih36mNvjx6/1/F0Veksj+yhA=",
          "NKIlnAJowWE/a/yeJsHHQDPy3I0qF2A5eTfshgOKHAQ="
        ])";

    creds.signed_creds = R"([
          "whyLpcq84WBfWSvRevORFeyhfdqLQnINPMpbtt8kJUM=",
          "1qgtLfj8MJihUhYRl5rE0TJZcTEAIwjxVc4QxpGlzRA=",
          "WJ3VUVIFLP3s5l4+gmEg8CeSiZ/jcAyx5mnHwZ96L30=",
          "zL/vNT8LcvHXm3ckNEKBCwM5ApL16gAieFePvAZfKUQ=",
          "qkFCSzokORAJJwAJrTgpfYY9J8uIZjuAe6jax+q0Pmo=",
          "npfth11Vvm9tTO773xZ8SY1b0orUHVJG3380XKMGvSw=",
          "FgJvxc1NQAJRyFUXh/2gGch+hiDnfMc3EC36d5zy9mY=",
          "4sCN5isvdPu5a/eqG+otvivCg91ua2Fu3aJDxDWspHs=",
          "ZGDqbP6a7S+o1UL3P8dGZp55SueW/1GXwk3FpCL5txM=",
          "dLWseCdi7zR3hOAdml7c5HvIWOHyQ0BhhfpjpIBRghM=",
          "WDiJnPj8SfTRPCI2u6cAG8GMSiSF3aRk9bIRruoR2wo=",
          "grCk/Ktag4ACaChEB5tPixuZB6SHz14YnN25p0YDuTc=",
          "Hu3yQVKi/Y8e/0QfNZ9ZAXOEDEJTjEwoKcm2VbtfClA=",
          "RvOPReTvHlv1JzNbwoGBtX6GeKmp2M8qVgbutrxXZ2s=",
          "MrEtLGpzpElqppRoW+45+OXLlTbXWXRzurqQsGmYQmM=",
          "0s3fmZAS8adnuGD90HQeKYwdDMP1+97QHD7FOhugayw=",
          "hPh7nzr7odsV6VwHhKlwDIFKloGQTpbi23qllJCi8B8=",
          "GtRRTXAmPk1MNFOhzx6+cRSwZP0uFXeDcNxfj4jHv28=",
          "iKCMZF+7eHxr/3Aeh4rjIM/b0GU7x5e9ZkHO3GqiXl4=",
          "6KoAiaSu8fCBaywEayQYOQASELa9yqL245GVMbBlmWc="
        ])";

    creds.public_key = "rqQ1Tz26C4mv33ld7xpcLhuX1sWaD+s7VMnuX6cokT4=";
    creds.batch_proof =
        "xdWq0jwSs2Z9lhfpEUR1nYX/f3Q4LUa9Y1kmhGMD1At/"
        "tqGTJ0ogFREiBwhCflUl2AoQmAUSsELbHrFtC/dgAQ==";  // NOLINT

    return creds;
  }
};

TEST_F(PromotionUtilTest, UnBlindCredsWorksCorrectly) {
  auto unblinded_encoded_tokens = UnBlindCreds(GetCredsBatch());

  EXPECT_TRUE(unblinded_encoded_tokens.has_value());
  EXPECT_EQ(unblinded_encoded_tokens->size(), 20u);
}

TEST_F(PromotionUtilTest, UnBlindCredsCredsNotCorrect) {
  auto creds = GetCredsBatch();
  creds.blinded_creds = creds.signed_creds;

  auto unblinded_encoded_tokens = UnBlindCreds(std::move(creds));

  EXPECT_FALSE(unblinded_encoded_tokens.has_value());
  EXPECT_EQ(unblinded_encoded_tokens.error(),
            "Failed to verify and unblind batch DLEQ proof");
}

}  // namespace credential
}  // namespace brave_rewards::internal
