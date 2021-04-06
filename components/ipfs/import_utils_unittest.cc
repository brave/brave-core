/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ipfs/import_utils.h"

#include <string>

#include "testing/gtest/include/gtest/gtest.h"

namespace ipfs {

class IpfsImportUtilsUnitTest : public testing::Test {
 public:
  IpfsImportUtilsUnitTest() {}
  ~IpfsImportUtilsUnitTest() override = default;

 private:
};

TEST_F(IpfsImportUtilsUnitTest, AddMultipartHeaderForUploadWithFileName) {
  const char ref_output[] =
      "--boundary\r\nContent-Disposition: form-data; name=\"value name\"; "
      "filename=\"value\"\r\nContent-Type: content type\r\n\r\n"
      "--boundary\r\nContent-Disposition: form-data; name=\"value name\"; "
      "filename=\"value\"\r\nContent-Type: \r\n\r\n";
  std::string post_data;
  AddMultipartHeaderForUploadWithFileName("value name", "value",
                                         "boundary", "content type",
                                         &post_data);
  AddMultipartHeaderForUploadWithFileName("value name", "value",
                                         "boundary", "", &post_data);
  EXPECT_STREQ(ref_output, post_data.c_str());
}

}  // namespace ipfs
