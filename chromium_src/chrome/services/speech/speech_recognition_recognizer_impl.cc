/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/chrome/services/speech/soda/sherpa_onnx_soda_client.h"

// Skip the SodaClientImpl header to avoid class redefinition, then rename
// all SodaClientImpl usage to SherpaOnnxSodaClient. Since our header is
// already included above, SherpaOnnxSodaClient is fully defined and the
// original CreateSodaClient() will create our Sherpa-ONNX client.
#define CHROME_SERVICES_SPEECH_SODA_SODA_CLIENT_IMPL_H_
#define SodaClientImpl SherpaOnnxSodaClient

#include <chrome/services/speech/speech_recognition_recognizer_impl.cc>

#undef SodaClientImpl
