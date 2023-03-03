/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PRIVACY_CHALLENGE_BYPASS_RISTRETTO_CHALLENGE_BYPASS_RISTRETTO_UNITTEST_CONSTANTS_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PRIVACY_CHALLENGE_BYPASS_RISTRETTO_CHALLENGE_BYPASS_RISTRETTO_UNITTEST_CONSTANTS_H_

namespace ads::privacy::cbr {

constexpr char kInvalidBase64[] = "FOOBAR";

constexpr char kSigningKeyBase64[] =
    R"(qsA+Hif/fQ3fNLMa37qbCLJjlzauLaqbjFNff/PkFAY=)";

constexpr char kPublicKeyBase64[] =
    R"(QnShwT9vRebch3WDu28nqlTaNCU5MaOF1n4VV4Q3K1g=)";

constexpr char kTokenBase64[] =
    R"(IXDCnZnVEJ0orkbZfr2ut2NQPQ0ofdervKBmQ2hyjcClGCjA3/ExbBumO0ua5cxwo//nN0uKQ60kknru8hRXx0FMYHu7ahizDiX18HVBawWBzA46pyrBtJDfiomK6HQI)";

constexpr char kBlindedTokenBase64[] =
    R"(mNRViqFD8ZPpjRZi4Xwj1UEsU1j9qPNc4R/BoiWsVi0=)";

constexpr char kSignedTokenBase64[] =
    R"(jGZR7JREp+zoqxgsMOa32F+zhhBw/0d/HeVhl9iPVWU=)";

constexpr char kUnblindedTokenBase64[] =
    R"(IXDCnZnVEJ0orkbZfr2ut2NQPQ0ofdervKBmQ2hyjcClGCjA3/ExbBumO0ua5cxwo//nN0uKQ60kknru8hRXx0DWhwHwuFlxmot8WgVbnQ0XtPx7q9BG0jbI00AJStwN)";

constexpr char kVerificationSignatureBase64[] =
    R"(V7Gilxl5TNv7pTqq8Sftmu+O+HgJ44Byn8PhDkDIwnsgncGiCduuoRMNagUnN7AXalaQdy1GedKj5thKFeyUcQ==)";

constexpr char kTokenPreimageBase64[] =
    R"(IXDCnZnVEJ0orkbZfr2ut2NQPQ0ofdervKBmQ2hyjcClGCjA3/ExbBumO0ua5cxwo//nN0uKQ60kknru8hRXxw==)";

}  // namespace ads::privacy::cbr

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PRIVACY_CHALLENGE_BYPASS_RISTRETTO_CHALLENGE_BYPASS_RISTRETTO_UNITTEST_CONSTANTS_H_
