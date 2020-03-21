/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/strings/string_split.h"
#include "brave/components/brave_rewards/resources/grit/brave_rewards_resources.h"
#include "bat/ledger/internal/bat_helper.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/resource/resource_bundle.h"

// npm run test -- brave_unit_tests --filter=NicewarePartialUnitTest.*

std::string getHexBytes(std::vector<uint8_t> seed) {
  return braveledger_bat_helper::uint8ToHex(seed);
}

TEST(NicewarePartialUnitTest, InvalidNumberOfWords) {
  // != 16 words
  const std::string& passPhrase = "rickshaw fleecy handwrote"
    " diurnal coarsest rose outreasoning coined jowly"
    " undefiled parched kielbasa decapitate ninetales";
  std::vector<uint8_t> nSeed;
  size_t written = 0;
  // get wordlist
  const std::string word_list =
      ui::ResourceBundle::GetSharedInstance().GetRawDataResource(
          IDR_BRAVE_REWARDS_NICEWARE_LIST).as_string();
  auto data_split = base::SplitString(
      word_list,
      DICTIONARY_DELIMITER,
      base::TRIM_WHITESPACE,
      base::SPLIT_WANT_NONEMPTY);
  int result = braveledger_bat_helper::niceware_mnemonic_to_bytes(
      passPhrase,
      &nSeed,
      &written,
      data_split);

  ASSERT_TRUE(result != 0 && written == 0);
}

TEST(NicewarePartialUnitTest, InvalidWordInList) {
  // contains a word not in the list - ninetales
  const std::string& passPhrase = "sherlock rickshaw fleecy handwrote"
    " diurnal coarsest rose outreasoning coined jowly"
    " undefiled parched kielbasa decapitate ninetales"
    " vermonter";
  std::vector<uint8_t> nSeed;
  size_t written = 0;
  const std::string word_list =
      ui::ResourceBundle::GetSharedInstance().GetRawDataResource(
          IDR_BRAVE_REWARDS_NICEWARE_LIST).as_string();
  auto data_split = base::SplitString(
      word_list,
      DICTIONARY_DELIMITER,
      base::TRIM_WHITESPACE,
      base::SPLIT_WANT_NONEMPTY);
  int result = braveledger_bat_helper::niceware_mnemonic_to_bytes(
      passPhrase,
      &nSeed,
      &written,
      data_split);

  ASSERT_TRUE(result != 0 && written == 0);
}

TEST(NicewarePartialUnitTest, ValidWordListPassOne) {
  const std::string& passPhrase = "sherlock rickshaw fleecy handwrote"
    " diurnal coarsest rose outreasoning coined jowly undefiled parched"
    " kielbasa decapitate throughout vermonter";
  std::vector<uint8_t> nSeed;
  size_t written = 0;
  const std::string word_list =
      ui::ResourceBundle::GetSharedInstance().GetRawDataResource(
          IDR_BRAVE_REWARDS_NICEWARE_LIST).as_string();
  auto data_split = base::SplitString(
      word_list,
      DICTIONARY_DELIMITER,
      base::TRIM_WHITESPACE,
      base::SPLIT_WANT_NONEMPTY);
  int result = braveledger_bat_helper::niceware_mnemonic_to_bytes(
      passPhrase,
      &nSeed,
      &written,
      data_split);

  ASSERT_TRUE(result == 0 && getHexBytes(nSeed) ==
    "c874bcc95057603c3ce024babe889753258a74aaec759bcb7641330ee251f549");
}

TEST(NicewarePartialUnitTest, ValidWordListPassTwo) {
  const std::string& passPhrase = "unskillfully robber quadraphonic"
    " horsed breviary punish beta wrapper whale betokened"
    " calix cableway combatted jury palliate senegalese";
  std::vector<uint8_t> nSeed;
  size_t written = 0;
  const std::string word_list =
      ui::ResourceBundle::GetSharedInstance().GetRawDataResource(
          IDR_BRAVE_REWARDS_NICEWARE_LIST).as_string();
  auto data_split = base::SplitString(
      word_list,
      DICTIONARY_DELIMITER,
      base::TRIM_WHITESPACE,
      base::SPLIT_WANT_NONEMPTY);
  int result = braveledger_bat_helper::niceware_mnemonic_to_bytes(
      passPhrase,
      &nSeed,
      &written,
      data_split);

  ASSERT_TRUE(result == 0 && getHexBytes(nSeed) ==
    "f14ebdc3ae2965ee1728ad2910aefdeafa4e10be1b2f1a822644753e9ab7c62b");
}

TEST(NicewarePartialUnitTest, ValidWordListPassThree) {
  const std::string& passPhrase = "hemline crumby foothill sui"
    " vaporizing permutational pakistan rattish maturational"
    " beading bucketing nonzebra religiosity ridable amazement"
    " peening";
  std::vector<uint8_t> nSeed;
  size_t written = 0;
  const std::string word_list =
      ui::ResourceBundle::GetSharedInstance().GetRawDataResource(
          IDR_BRAVE_REWARDS_NICEWARE_LIST).as_string();
  auto data_split = base::SplitString(
      word_list,
      DICTIONARY_DELIMITER,
      base::TRIM_WHITESPACE,
      base::SPLIT_WANT_NONEMPTY);
  int result = braveledger_bat_helper::niceware_mnemonic_to_bytes(
      passPhrase,
      &nSeed,
      &written,
      data_split);

  ASSERT_TRUE(result == 0 && getHexBytes(nSeed) ==
    "62c62f615234d9f4f4319f4f9a82b12d82d50e5b188791b4b786bcd203f19de7");
}

TEST(NicewarePartialUnitTest, ValidWordListPassFour) {
  const std::string& passPhrase = "A";
  std::vector<uint8_t> nSeed;
  size_t written = 0;
  const std::string word_list =
      ui::ResourceBundle::GetSharedInstance().GetRawDataResource(
          IDR_BRAVE_REWARDS_NICEWARE_LIST).as_string();
  auto data_split = base::SplitString(
      word_list,
      DICTIONARY_DELIMITER,
      base::TRIM_WHITESPACE,
      base::SPLIT_WANT_NONEMPTY);
  int result = braveledger_bat_helper::niceware_mnemonic_to_bytes(
      passPhrase,
      &nSeed,
      &written,
      data_split);

  ASSERT_TRUE(result == 0 && getHexBytes(nSeed) == "0000");
}

TEST(NicewarePartialUnitTest, ValidWordListPassFive) {
  const std::string& passPhrase = "zyzzyva";
  std::vector<uint8_t> nSeed;
  size_t written = 0;
  const std::string word_list =
      ui::ResourceBundle::GetSharedInstance().GetRawDataResource(
          IDR_BRAVE_REWARDS_NICEWARE_LIST).as_string();
  auto data_split = base::SplitString(
      word_list,
      DICTIONARY_DELIMITER,
      base::TRIM_WHITESPACE,
      base::SPLIT_WANT_NONEMPTY);
  int result = braveledger_bat_helper::niceware_mnemonic_to_bytes(
      passPhrase,
      &nSeed,
      &written,
      data_split);

  ASSERT_TRUE(result == 0 && getHexBytes(nSeed) == "ffff");
}

TEST(NicewarePartialUnitTest, ValidWordListPassSix) {
  const std::string& passPhrase = "A bioengineering Balloted gobbledegooK"
    " cReneled Written depriving zyzzyva";
  std::vector<uint8_t> nSeed;
  size_t written = 0;
  const std::string word_list =
      ui::ResourceBundle::GetSharedInstance().GetRawDataResource(
          IDR_BRAVE_REWARDS_NICEWARE_LIST).as_string();
  auto data_split = base::SplitString(
      word_list,
      DICTIONARY_DELIMITER,
      base::TRIM_WHITESPACE,
      base::SPLIT_WANT_NONEMPTY);
  int result = braveledger_bat_helper::niceware_mnemonic_to_bytes(
      passPhrase,
      &nSeed,
      &written,
      data_split);

  ASSERT_TRUE(result == 0 && getHexBytes(nSeed) ==
    "000011d40c8c5af72e53fe3c36a9ffff");
}
