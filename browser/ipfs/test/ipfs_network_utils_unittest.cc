/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ipfs/ipfs_network_utils.h"

#include <memory>
#include <string>
#include <utility>

#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "brave/browser/ipfs/ipfs_blob_context_getter_factory.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/test_browser_context.h"
#include "services/network/public/cpp/data_element.h"
#include "services/network/public/cpp/resource_request.h"
#include "storage/browser/blob/blob_data_builder.h"
#include "storage/browser/blob/blob_data_item.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ipfs {

class IpfsNetwrokUtilsUnitTest : public testing::Test {
 public:
  IpfsNetwrokUtilsUnitTest() {}
  ~IpfsNetwrokUtilsUnitTest() override = default;

  void SetUp() override {
    browser_context_ = std::make_unique<content::TestBrowserContext>();
    blob_getter_factory_ =
        std::make_unique<IpfsBlobContextGetterFactory>(browser_context_.get());
  }

  base::FilePath CreateCustomTestFile(const base::FilePath& dir,
                                      const std::string& filename,
                                      const std::string& content) {
    base::FilePath file_path = dir.AppendASCII(filename);

    base::WriteFile(file_path, content);
    return file_path;
  }

  IpfsBlobContextGetterFactory* blob_getter_factory() {
    return blob_getter_factory_.get();
  }

  void ValidateRequest(base::OnceClosure callback,
                       std::unique_ptr<network::ResourceRequest> request) {
    ASSERT_TRUE(request.get());
    ASSERT_EQ(request->request_body->elements()->size(), size_t(1));
    ASSERT_EQ(request->request_body->elements()->front().type(),
              network::mojom::DataElementDataView::Tag::kDataPipe);
    if (callback)
      std::move(callback).Run();
  }

 private:
  content::BrowserTaskEnvironment task_environment_;
  std::unique_ptr<content::BrowserContext> browser_context_;
  std::unique_ptr<IpfsBlobContextGetterFactory> blob_getter_factory_;
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

TEST_F(IpfsNetwrokUtilsUnitTest, CreateRequestForFileTest) {
  base::ScopedTempDir dir;
  ASSERT_TRUE(dir.CreateUniqueTempDir());
  std::string content = "test\n\rmultiline\n\rcontent";
  std::string filename = "test_name";
  base::FilePath upload_file_path =
      CreateCustomTestFile(dir.GetPath(), filename, content);
  size_t file_size = content.size();
  std::string mime_type = "test/type";
  std::string mime_boundary = "mime_boundary";
  base::RunLoop run_loop;
  auto upload_callback =
      base::BindOnce(&IpfsNetwrokUtilsUnitTest::ValidateRequest,
                     base::Unretained(this), run_loop.QuitClosure());
  CreateRequestForFile(upload_file_path, blob_getter_factory(), mime_type,
                       filename, std::move(upload_callback), file_size);
  run_loop.Run();
}

TEST_F(IpfsNetwrokUtilsUnitTest, CreateRequestForTextTest) {
  std::string text = "test\n\rmultiline\n\rcontent";
  std::string filename = "test_name";
  std::string mime_type = "test/type";
  std::string mime_boundary = "mime_boundary";
  base::RunLoop run_loop;
  auto upload_callback =
      base::BindOnce(&IpfsNetwrokUtilsUnitTest::ValidateRequest,
                     base::Unretained(this), run_loop.QuitClosure());
  CreateRequestForText(text, filename, blob_getter_factory(),
                       std::move(upload_callback));
  run_loop.Run();
}

TEST_F(IpfsNetwrokUtilsUnitTest, CreateRequestForFolderTest) {
  base::ScopedTempDir dir;
  ASSERT_TRUE(dir.CreateUniqueTempDir());
  std::string content = "test\n\rmultiline\n\rcontent";
  std::string filename = "test_name";
  CreateCustomTestFile(dir.GetPath(), filename, content);
  std::string mime_type = "test/type";
  std::string mime_boundary = "mime_boundary";
  base::RunLoop run_loop;
  auto upload_callback =
      base::BindOnce(&IpfsNetwrokUtilsUnitTest::ValidateRequest,
                     base::Unretained(this), run_loop.QuitClosure());
  CreateRequestForFolder(dir.GetPath(), blob_getter_factory(),
                         std::move(upload_callback));
  run_loop.Run();
}

}  // namespace ipfs
