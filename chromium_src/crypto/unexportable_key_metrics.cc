/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#define BRAVE_ALL_ALGORITHMS \
  SignatureVerifier::SignatureAlgorithm::ECDSA_SHA256,
#define BRAVE_MEASURE_VIRTUAL_TMP_OPERATIONS \
  case SignatureVerifier::SignatureAlgorithm::ECDSA_SHA384:
#define BRAVE_MEASURE_TMP_OPERATIONS_INTERNAL \
  case SignatureVerifier::SignatureAlgorithm::ECDSA_SHA384:
#define BRAVE_UNEXPORTED_KEY_METRICS_ALGORITHM_TO_STRING \
  case SignatureVerifier::SignatureAlgorithm::ECDSA_SHA384:

#include "src/crypto/unexportable_key_metrics.cc"

#undef BRAVE_ALL_ALGORITHMS
#undef BRAVE_MEASURE_VIRTUAL_TMP_OPERATIONS
#undef BRAVE_MEASURE_TMP_OPERATIONS_INTERNAL
#undef BRAVE_UNEXPORTED_KEY_METRICS_ALGORITHM_TO_STRING
