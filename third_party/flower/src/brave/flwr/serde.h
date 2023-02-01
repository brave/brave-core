/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_THIRD_PARTY_FLOWER_SRC_BRAVE_FLWR_INCLUDE_SERDE_H_
#define BRAVE_THIRD_PARTY_FLOWER_SRC_BRAVE_FLWR_INCLUDE_SERDE_H_

#include <string>

#include "brave/third_party/flower/src/proto/flwr/proto/fleet.pb.h"
#include "brave/third_party/flower/src/proto/flwr/proto/transport.pb.h"

/**
 * Deserialize bytes to float vector
 */
std::vector<float> GetVectorFromString(std::string string);

/**
 * Serialize float vector into bytes
 */
std::string GetStringFromVector(std::vector<float> vector);

/**
 * Get flower::Parameters from list of vectors
 */
flower::Parameters GetParametersFromVectors(
    std::vector<std::vector<float>> parameters_vectors);

/**
 * Get list of vectors fromm flower::Parameters
 */
std::vector<std::vector<float>> GetVectorsFromParameters(
    flower::Parameters parameters_msg);

/**
 * Build anonymous pull task request message
 */
flower::PullTaskInsRequest BuildAnonymousPullTaskInsRequestMessage();

#endif  // BRAVE_THIRD_PARTY_FLOWER_SRC_BRAVE_FLWR_INCLUDE_SERDE_H_
