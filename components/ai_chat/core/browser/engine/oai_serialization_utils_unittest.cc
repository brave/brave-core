/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/engine/oai_serialization_utils.h"

#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace ai_chat {

TEST(OAISerializationUtilsTest, MemoryContentBlockToDict_StringValues) {
  auto block = mojom::MemoryContentBlock::New();
  block->memory["name"] = mojom::MemoryValue::NewStringValue("Alice");
  block->memory["city"] = mojom::MemoryValue::NewStringValue("NYC");

  auto dict = MemoryContentBlockToDict(*block);

  const std::string* name = dict.FindString("name");
  ASSERT_TRUE(name);
  EXPECT_EQ(*name, "Alice");

  const std::string* city = dict.FindString("city");
  ASSERT_TRUE(city);
  EXPECT_EQ(*city, "NYC");
}

TEST(OAISerializationUtilsTest, MemoryContentBlockToDict_ListValues) {
  auto block = mojom::MemoryContentBlock::New();
  block->memory["hobbies"] =
      mojom::MemoryValue::NewListValue({"reading", "coding"});

  auto dict = MemoryContentBlockToDict(*block);

  const base::ListValue* hobbies = dict.FindList("hobbies");
  ASSERT_TRUE(hobbies);
  ASSERT_EQ(hobbies->size(), 2u);
  EXPECT_EQ((*hobbies)[0].GetString(), "reading");
  EXPECT_EQ((*hobbies)[1].GetString(), "coding");
}

TEST(OAISerializationUtilsTest, MemoryContentBlockToDict_MixedValues) {
  auto block = mojom::MemoryContentBlock::New();
  block->memory["name"] = mojom::MemoryValue::NewStringValue("Bob");
  block->memory["langs"] = mojom::MemoryValue::NewListValue({"C++", "Python"});

  auto dict = MemoryContentBlockToDict(*block);

  const std::string* name = dict.FindString("name");
  ASSERT_TRUE(name);
  EXPECT_EQ(*name, "Bob");

  const base::ListValue* langs = dict.FindList("langs");
  ASSERT_TRUE(langs);
  ASSERT_EQ(langs->size(), 2u);
  EXPECT_EQ((*langs)[0].GetString(), "C++");
  EXPECT_EQ((*langs)[1].GetString(), "Python");
}

TEST(OAISerializationUtilsTest, MemoryContentBlockToDict_Empty) {
  auto block = mojom::MemoryContentBlock::New();
  auto dict = MemoryContentBlockToDict(*block);
  EXPECT_TRUE(dict.empty());
}

TEST(OAISerializationUtilsTest, FileContentBlockToDict) {
  auto block = mojom::FileContentBlock::New();
  block->filename = "test.pdf";
  block->file_data = GURL("data:application/pdf;base64,abc123");

  auto dict = FileContentBlockToDict(*block);

  const std::string* filename = dict.FindString("filename");
  ASSERT_TRUE(filename);
  EXPECT_EQ(*filename, "test.pdf");

  const std::string* file_data = dict.FindString("file_data");
  ASSERT_TRUE(file_data);
  EXPECT_EQ(*file_data, "data:application/pdf;base64,abc123");
}

TEST(OAISerializationUtilsTest, ImageContentBlockToDict) {
  auto block = mojom::ImageContentBlock::New();
  block->image_url = GURL("data:image/png;base64,xyz789");

  auto dict = ImageContentBlockToDict(*block);

  const std::string* url = dict.FindString("url");
  ASSERT_TRUE(url);
  EXPECT_EQ(*url, "data:image/png;base64,xyz789");
}

}  // namespace ai_chat
