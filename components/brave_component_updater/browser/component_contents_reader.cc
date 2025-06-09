// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_component_updater/browser/component_contents_reader.h"

#include <utility>
#include <vector>

#include "base/files/file_util.h"
#include "base/memory/ptr_util.h"
#include "base/task/thread_pool.h"
#include "base/trace_event/trace_event.h"
#include "brave/components/brave_component_updater/browser/component_contents_verifier.h"

namespace component_updater {

namespace {

template <typename DataType>
std::optional<DataType> Verify(std::optional<DataType> data,
                               std::unique_ptr<ContentChecker> checker) {
  if (!checker || !data) {
    // File is not signed or can't be read.
    return data;
  }
  if (!checker->VerifyContents(base::as_byte_span(*data))) {
    // Invalid signature.
    return std::nullopt;
  }
  return data;
}

std::optional<std::vector<uint8_t>> ReadFileToBytesAndVerify(
    const base::FilePath& path,
    std::unique_ptr<ContentChecker> checker) {
  TRACE_EVENT_BEGIN("brave.adblock", "ReadDATFileData", "path",
                    path.MaybeAsASCII());
  auto data = Verify(base::ReadFileToBytes(path), std::move(checker));
  TRACE_EVENT_END("brave.adblock", "size", (data ? data->size() : 0));
  return data;
}

std::optional<std::string> ReadFileToStringAndVerify(
    const base::FilePath& path,
    std::unique_ptr<ContentChecker> checker) {
  TRACE_EVENT_BEGIN("brave.adblock", "GetDATFileAsString", "path",
                    path.MaybeAsASCII());
  std::string content;
  if (!base::ReadFileToString(path, &content)) {
    TRACE_EVENT_END("brave.adblock", "size", 0);
    return std::nullopt;
  }
  auto data = Verify<std::string>(std::move(content), std::move(checker));
  TRACE_EVENT_END("brave.adblock", "size", (data ? data->size() : 0));
  return data;
}

}  // namespace

ComponentContentsReader::ComponentContentsReader(
    const base::FilePath& component_root)
    : component_root_(component_root),
      verifier_(CreateContentsVerifier(component_root)) {}

ComponentContentsReader::~ComponentContentsReader() = default;

// static
std::unique_ptr<ComponentContentsReader> ComponentContentsReader::Create(
    const base::FilePath& component_root) {
  auto reader = base::WrapUnique(new ComponentContentsReader(component_root));
  if (reader->verifier_ && !reader->verifier_->IsValid()) {
    return nullptr;
  }
  return reader;
}

// static
std::unique_ptr<ComponentContentsReader>
ComponentContentsReader::CreateBypassForTesting(
    const base::FilePath& component_root) {
  // Doesn't check for |verifier_| is valid, it allows to read an unsigned
  // component. But if component if signed it will check the signature.
  return base::WrapUnique(new ComponentContentsReader(component_root));
}

const base::FilePath& ComponentContentsReader::GetComponentRootDeprecated()
    const {
  return component_root_;
}

void ComponentContentsReader::GetFileAsString(
    const base::FilePath& relative_path,
    OnGetAsString on_data,
    const base::TaskPriority priority) {
  GetFileAsString(
      relative_path, std::move(on_data),
      base::ThreadPool::CreateTaskRunner({base::MayBlock(), priority}));
}

void ComponentContentsReader::GetFileAsString(
    const base::FilePath& relative_path,
    OnGetAsString on_data,
    scoped_refptr<base::TaskRunner> file_task_runner) {
  auto checker =
      verifier_ ? verifier_->CreateContentChecker(relative_path) : nullptr;

  file_task_runner->PostTaskAndReplyWithResult(
      FROM_HERE,
      base::BindOnce(&ReadFileToStringAndVerify,
                     component_root_.Append(relative_path), std::move(checker)),
      std::move(on_data));
}

void ComponentContentsReader::GetFileAsBytes(
    const base::FilePath& relative_path,
    OnGetAsBytes on_data,
    const base::TaskPriority priority) {
  GetFileAsBytes(
      relative_path, std::move(on_data),
      base::ThreadPool::CreateTaskRunner({base::MayBlock(), priority}));
}

void ComponentContentsReader::GetFileAsBytes(
    const base::FilePath& relative_path,
    OnGetAsBytes on_data,
    scoped_refptr<base::TaskRunner> file_task_runner) {
  CHECK(file_task_runner);
  auto checker =
      verifier_ ? verifier_->CreateContentChecker(relative_path) : nullptr;

  file_task_runner->PostTaskAndReplyWithResult(
      FROM_HERE,
      base::BindOnce(&ReadFileToBytesAndVerify,
                     component_root_.Append(relative_path), std::move(checker)),
      std::move(on_data));
}

}  // namespace component_updater
