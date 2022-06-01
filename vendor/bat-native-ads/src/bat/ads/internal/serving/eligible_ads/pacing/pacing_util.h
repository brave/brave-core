/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_SERVING_ELIGIBLE_ADS_PACING_PACING_UTIL_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_SERVING_ELIGIBLE_ADS_PACING_PACING_UTIL_H_

#include <ios>
#include <ctime> 
#include <math.h>
#include <iostream>

#include "base/base64.h"
#include "bat/ads/internal/base/crypto_util.h"
#include "bat/ads/internal/base/logging_util.h"
#include "bat/ads/internal/serving/eligible_ads/pacing/pacing_random_util.h"

namespace ads {

template <typename T>
bool ShouldPaceAd(const T& ad) {
  // const double rand = GeneratePacingRandomNumber();
  const double rand = GeneratePacingHashedNumber(ad);
  if (rand < ad.ptr) {
    return false;
  }

  BLOG(2, std::fixed << "Pacing delivery for creative instance id "
                     << ad.creative_instance_id << " [Roll(" << ad.ptr
                     << "):" << rand << "]");

  return true;
}

template <typename T>
double GeneratePacingHashedNumber(const T& ad) {
  std::time_t current_system_time = time(0);
  std::tm* current_gmt_time = gmtime(&current_system_time);
  if (current_gmt_time != NULL) {
    char* current_gmt = asctime(current_gmt_time);
    char* gmt_dow = strtok(current_gmt, " ");
    char* gmt_month = strtok(NULL, " ");
    char* gmt_day = strtok(NULL, " ");

    std::string individual_id("wallet_id or cpu_id");
    std::string unhashed_key(individual_id + ad.campaign_id);
    unhashed_key.append(std::string(gmt_dow) + gmt_month + gmt_day);
    std::vector<uint8_t> hash_vector = security::Sha256Hash(unhashed_key);
    std::string hashed_key = base::Base64Encode(hash_vector);

    int tail_length = 2; 
    std::string hashed_key_tail = hashed_key.substr(hashed_key.length() - tail_length - 1, tail_length);
    std::string base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    double tail_sum = 0;
    for (int i = 0; i < tail_length; i++) {
      for (int j = 0; j < 64; j++) {
        if (hashed_key_tail[i] == base64_chars[j]) {
          tail_sum += (j * pow(64, i));
          break;
        }
      }
    }
    double threshold = tail_sum / pow(64, tail_length); // no minus 1 in denominator because return 1.0 is breaking

    std::cout << "Start Logging";
    std::cout << "\n";
    std::cout << unhashed_key;
    std::cout << "\n";
    std::cout << hashed_key;
    std::cout << "\n";
    std::cout << hashed_key_tail;
    std::cout << "\n";
    std::cout << tail_sum;
    std::cout << "\n";
    std::cout << threshold;
    std::cout << "\n";
    std::cout << "End Logging";
    std::cout << "\n";

    return threshold;
  }
  else {
    const double rand = GeneratePacingRandomNumber();
    return rand;
  }
}

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_SERVING_ELIGIBLE_ADS_PACING_PACING_UTIL_H_
