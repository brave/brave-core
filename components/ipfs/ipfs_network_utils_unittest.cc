/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ipfs/ipfs_network_utils.h"

#include <memory>
#include <string>

#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "storage/browser/blob/blob_data_builder.h"
#include "storage/browser/blob/blob_data_item.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ipfs {

class IpfsNetwrokUtilsUnitTest : public testing::Test {
 public:
  IpfsNetwrokUtilsUnitTest() {}
  ~IpfsNetwrokUtilsUnitTest() override = default;

 private:
};

TEST_F(IpfsNetwrokUtilsUnitTest, AddMultipartHeaderForUploadWithFileName) {
  const char ref_output[] =
      "--boundary\r\nContent-Disposition: form-data; name=\"value name\"; "
      "filename=\"value\"\r\nContent-Type: content type\r\n\r\n"
      "--boundary\r\nAbspath: file_abs_path\r\nContent-Disposition: form-data; "
      "name=\"value name\"; "
      "filename=\"value\"\r\nContent-Type: \r\n\r\n";
  std::string post_data;
  AddMultipartHeaderForUploadWithFileName("value name", "value", std::string(),
                                          "boundary", "content type",
                                          &post_data);
  AddMultipartHeaderForUploadWithFileName(
      "value name", "value", "file_abs_path", "boundary", "", &post_data);
  EXPECT_STREQ(ref_output, post_data.c_str());
}

TEST_F(IpfsNetwrokUtilsUnitTest, BuildFileBlob) {
  base::FilePath upload_file_path(FILE_PATH_LITERAL("test_file"));
  size_t file_size = 100;
  std::string mime_type = "test/type";
  std::string filename = "test_name";
  std::string mime_boundary = "mime_boundary";
  std::unique_ptr<storage::BlobDataBuilder> builder = BuildBlobWithFile(
      upload_file_path, file_size, mime_type, filename, mime_boundary);

  EXPECT_EQ(builder->items().size(), (size_t)3);
  EXPECT_EQ(builder->items()[0]->item()->type(),
            storage::BlobDataItem::Type::kBytes);
  EXPECT_EQ(builder->items()[1]->item()->type(),
            storage::BlobDataItem::Type::kFile);
  EXPECT_EQ(builder->items()[2]->item()->type(),
            storage::BlobDataItem::Type::kBytes);
}

TEST_F(IpfsNetwrokUtilsUnitTest, FileSizeCalculation) {
  base::ScopedTempDir dir;
  ASSERT_TRUE(dir.CreateUniqueTempDir());
  base::FilePath file_path =
      dir.GetPath().Append(FILE_PATH_LITERAL("test.file"));

  std::string content = "test\n\rmultiline\n\rcontent";
  ASSERT_TRUE(base::WriteFile(file_path, content));
  EXPECT_EQ(CalculateFileSize(file_path), (int64_t)content.size());

  base::FilePath nonexistent_file_path =
      dir.GetPath().Append(FILE_PATH_LITERAL("fake.file"));
  EXPECT_EQ(CalculateFileSize(nonexistent_file_path), -1);
}

}  // namespace ipfs
