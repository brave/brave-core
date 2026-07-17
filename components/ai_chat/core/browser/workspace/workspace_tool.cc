// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/workspace/workspace_tool.h"

#include <utility>

#include "base/functional/bind.h"
#include "base/json/json_reader.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "brave/components/ai_chat/core/browser/tools/tool_input_properties.h"
#include "brave/components/ai_chat/core/browser/tools/tool_utils.h"
#include "brave/components/ai_chat/core/browser/workspace/workspace_service.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"

namespace ai_chat {

namespace {

std::string ValueOrEmpty(const std::string* value) {
  return value ? *value : std::string();
}

}  // namespace

WorkspaceTool::WorkspaceTool(Op op,
                             std::string name,
                             std::string description,
                             std::optional<base::DictValue> input_properties,
                             std::optional<std::vector<std::string>> required,
                             bool requires_permission,
                             base::WeakPtr<WorkspaceService> service,
                             std::string conversation_id)
    : op_(op),
      name_(std::move(name)),
      description_(std::move(description)),
      input_properties_(std::move(input_properties)),
      required_(std::move(required)),
      requires_permission_(requires_permission),
      service_(std::move(service)),
      conversation_id_(std::move(conversation_id)) {}

WorkspaceTool::~WorkspaceTool() = default;

std::string_view WorkspaceTool::Name() const {
  return name_;
}

std::string_view WorkspaceTool::Description() const {
  return description_;
}

std::optional<base::DictValue> WorkspaceTool::InputProperties() const {
  if (!input_properties_) {
    return std::nullopt;
  }
  return input_properties_->Clone();
}

std::optional<std::vector<std::string>> WorkspaceTool::RequiredProperties()
    const {
  return required_;
}

std::variant<bool, mojom::PermissionChallengePtr>
WorkspaceTool::RequiresUserInteractionBeforeHandling(
    const mojom::ToolUseEvent& tool_use) const {
  if (!requires_permission_ || permission_granted_) {
    return false;
  }
  // The user can grant a per-conversation blanket allowance for writes.
  if (service_ && service_->AreWritesAllowed(conversation_id_)) {
    return false;
  }
  return mojom::PermissionChallenge::New();
}

void WorkspaceTool::UserPermissionGranted(const std::string& tool_use_id) {
  permission_granted_ = true;
}

void WorkspaceTool::UseTool(const std::string& input_json,
                            UseToolCallback callback) {
  if (!service_) {
    std::move(callback).Run(
        CreateContentBlocksForText("Error: no workspace is available"), {});
    return;
  }

  // Models may emit an empty string for a tool taking no required parameters.
  const std::string& raw = input_json.empty() ? "{}" : input_json;
  auto parsed = base::JSONReader::ReadAndReturnValueWithError(
      raw, base::JSON_PARSE_CHROMIUM_EXTENSIONS);
  if (!parsed.has_value() || !parsed->is_dict()) {
    std::string detail =
        parsed.has_value()
            ? std::string("arguments were valid JSON but not an object")
            : base::StrCat(
                  {"could not parse arguments as JSON: ", parsed.error().message});
    std::move(callback).Run(
        CreateContentBlocksForText(base::StrCat(
            {"Error: ", detail, " (received ", base::NumberToString(raw.size()),
             " bytes). If the content is large, the tool-call arguments may "
             "have been truncated or mis-escaped; create the file with a "
             "smaller body and append the rest with insert."})),
        {});
    return;
  }
  const base::DictValue* dict = &parsed->GetDict();

  auto on_result = base::BindOnce(&WorkspaceTool::OnResult,
                                  weak_factory_.GetWeakPtr(),
                                  std::move(callback));

  switch (op_) {
    case Op::kListDir: {
      service_->ListDir(conversation_id_,
                        ValueOrEmpty(dict->FindString("path")),
                        dict->FindInt("depth").value_or(0),
                        std::move(on_result));
      return;
    }
    case Op::kReadFile: {
      const std::string* path = dict->FindString("path");
      if (!path) {
        std::move(on_result).Run(
            base::unexpected("missing required 'path' argument"));
        return;
      }
      std::optional<int> start_line;
      std::optional<int> end_line;
      if (const base::ListValue* range = dict->FindList("view_range")) {
        if (range->size() >= 1 && (*range)[0].is_int()) {
          start_line = (*range)[0].GetInt();
        }
        if (range->size() >= 2 && (*range)[1].is_int()) {
          end_line = (*range)[1].GetInt();
        }
      }
      service_->ReadFile(conversation_id_, *path, start_line, end_line,
                        std::move(on_result));
      return;
    }
    case Op::kGrep: {
      const std::string* pattern = dict->FindString("pattern");
      if (!pattern) {
        std::move(on_result).Run(
            base::unexpected("missing required 'pattern' argument"));
        return;
      }
      service_->Grep(conversation_id_, ValueOrEmpty(dict->FindString("path")),
                    *pattern, ValueOrEmpty(dict->FindString("include")),
                    std::move(on_result));
      return;
    }
    case Op::kGlob: {
      const std::string* pattern = dict->FindString("pattern");
      if (!pattern) {
        std::move(on_result).Run(
            base::unexpected("missing required 'pattern' argument"));
        return;
      }
      service_->Glob(conversation_id_, ValueOrEmpty(dict->FindString("path")),
                    *pattern, std::move(on_result));
      return;
    }
    case Op::kRepoMap: {
      service_->RepoMap(conversation_id_,
                       ValueOrEmpty(dict->FindString("path")),
                       std::move(on_result));
      return;
    }
    case Op::kCreateFile: {
      const std::string* path = dict->FindString("path");
      const std::string* file_text = dict->FindString("file_text");
      if (!path || !file_text) {
        std::move(on_result).Run(base::unexpected(
            "missing required 'path' or 'file_text' argument"));
        return;
      }
      service_->CreateFile(conversation_id_, *path, *file_text,
                          std::move(on_result));
      return;
    }
    case Op::kStrReplace: {
      const std::string* path = dict->FindString("path");
      const std::string* old_str = dict->FindString("old_str");
      const std::string* new_str = dict->FindString("new_str");
      if (!path || !old_str || !new_str) {
        std::move(on_result).Run(base::unexpected(
            "missing required 'path', 'old_str' or 'new_str' argument"));
        return;
      }
      service_->StrReplace(conversation_id_, *path, *old_str, *new_str,
                          std::move(on_result));
      return;
    }
    case Op::kInsert: {
      const std::string* path = dict->FindString("path");
      std::optional<int> insert_line = dict->FindInt("insert_line");
      const std::string* new_str = dict->FindString("new_str");
      if (!path || !insert_line || !new_str) {
        std::move(on_result).Run(base::unexpected(
            "missing required 'path', 'insert_line' or 'new_str' argument"));
        return;
      }
      service_->Insert(conversation_id_, *path, *insert_line, *new_str,
                      std::move(on_result));
      return;
    }
    case Op::kAppendFile: {
      const std::string* path = dict->FindString("path");
      const std::string* content = dict->FindString("content");
      if (!path || !content) {
        std::move(on_result).Run(base::unexpected(
            "missing required 'path' or 'content' argument"));
        return;
      }
      service_->AppendFile(conversation_id_, *path, *content,
                          std::move(on_result));
      return;
    }
    case Op::kUndoEdit: {
      const std::string* path = dict->FindString("path");
      if (!path) {
        std::move(on_result).Run(
            base::unexpected("missing required 'path' argument"));
        return;
      }
      service_->UndoEdit(conversation_id_, *path, std::move(on_result));
      return;
    }
  }
}

void WorkspaceTool::OnResult(UseToolCallback callback,
                             workspace::FileOpResult result) {
  if (result.has_value()) {
    std::move(callback).Run(CreateContentBlocksForText(result.value()), {});
  } else {
    std::move(callback).Run(
        CreateContentBlocksForText(base::StrCat({"Error: ", result.error()})),
        {});
  }
}

std::vector<std::unique_ptr<WorkspaceTool>> BuildWorkspaceTools(
    base::WeakPtr<WorkspaceService> service,
    const std::string& conversation_id) {
  std::vector<std::unique_ptr<WorkspaceTool>> tools;

  tools.push_back(std::make_unique<WorkspaceTool>(
      WorkspaceTool::Op::kListDir, "list_dir",
      "List files and directories within the workspace folder.",
      CreateInputProperties(
          {{"path",
            StringProperty("Directory path relative to the workspace root. "
                           "Leave empty to list the root.")},
           {"depth",
            IntegerProperty("How many directory levels to descend. Default 2.")}}),
      std::nullopt, /*requires_permission=*/false, service, conversation_id));

  tools.push_back(std::make_unique<WorkspaceTool>(
      WorkspaceTool::Op::kReadFile, "view_file",
      "Read a text file within the workspace, with 1-indexed line numbers.",
      CreateInputProperties(
          {{"path", StringProperty("File path relative to the workspace root.")},
           {"view_range",
            ArrayProperty(
                "Optional [start, end] 1-indexed line range to return. "
                "Use end of -1 to read through the end of the file.",
                IntegerProperty("A line number."))}}),
      std::vector<std::string>{"path"}, /*requires_permission=*/false, service,
      conversation_id));

  tools.push_back(std::make_unique<WorkspaceTool>(
      WorkspaceTool::Op::kGrep, "grep",
      "Search file contents within the workspace for a regular expression.",
      CreateInputProperties(
          {{"pattern", StringProperty("RE2 regular expression to search for.")},
           {"path",
            StringProperty("Directory to search under, relative to the "
                           "workspace root. Empty searches the whole "
                           "workspace.")},
           {"include",
            StringProperty("Optional glob restricting which files are "
                           "searched by their relative path, e.g. *.ts")}}),
      std::vector<std::string>{"pattern"}, /*requires_permission=*/false,
      service, conversation_id));

  tools.push_back(std::make_unique<WorkspaceTool>(
      WorkspaceTool::Op::kGlob, "glob",
      "Find files in the workspace whose relative path matches a glob.",
      CreateInputProperties(
          {{"pattern",
            StringProperty("Glob to match file paths, e.g. **/*.ts")},
           {"path",
            StringProperty("Directory to search under, relative to the "
                           "workspace root. Empty searches the whole "
                           "workspace.")}}),
      std::vector<std::string>{"pattern"}, /*requires_permission=*/false,
      service, conversation_id));

  tools.push_back(std::make_unique<WorkspaceTool>(
      WorkspaceTool::Op::kRepoMap, "repo_map",
      "Get a structural overview of the workspace: for each source file, its "
      "path and top-level definitions (classes, functions, etc.). Useful to "
      "understand the codebase before reading individual files.",
      CreateInputProperties(
          {{"path",
            StringProperty("Directory to map, relative to the workspace root. "
                           "Empty maps the whole workspace.")}}),
      std::nullopt, /*requires_permission=*/false, service, conversation_id));

  tools.push_back(std::make_unique<WorkspaceTool>(
      WorkspaceTool::Op::kCreateFile, "create_file",
      "Create a new file in the workspace. Fails if the file already exists. "
      "IMPORTANT: keep file_text small. For anything longer than a few lines, "
      "create the file with an empty or short file_text and then add the rest "
      "with repeated append_file calls, one small chunk per call. A large "
      "file_text in a single call can exceed the model's output limit and be "
      "rejected as invalid, so prefer create_file + append_file for real "
      "files.",
      CreateInputProperties(
          {{"path", StringProperty("New file path relative to the workspace "
                                   "root.")},
           {"file_text", StringProperty("Full contents of the new file.")}}),
      std::vector<std::string>{"path", "file_text"},
      /*requires_permission=*/true, service, conversation_id));

  tools.push_back(std::make_unique<WorkspaceTool>(
      WorkspaceTool::Op::kStrReplace, "str_replace",
      "Replace an exact, unique block of text in a workspace file. old_str "
      "must match exactly once, including whitespace.",
      CreateInputProperties(
          {{"path", StringProperty("File path relative to the workspace "
                                   "root.")},
           {"old_str",
            StringProperty("Exact existing text to replace. Must be unique in "
                           "the file.")},
           {"new_str", StringProperty("Replacement text.")}}),
      std::vector<std::string>{"path", "old_str", "new_str"},
      /*requires_permission=*/true, service, conversation_id));

  tools.push_back(std::make_unique<WorkspaceTool>(
      WorkspaceTool::Op::kInsert, "insert",
      "Insert text into a workspace file after a given 1-indexed line (0 for "
      "the start of the file).",
      CreateInputProperties(
          {{"path", StringProperty("File path relative to the workspace "
                                   "root.")},
           {"insert_line",
            IntegerProperty("Line number to insert after (0 = start).")},
           {"new_str", StringProperty("Text to insert.")}}),
      std::vector<std::string>{"path", "insert_line", "new_str"},
      /*requires_permission=*/true, service, conversation_id));

  tools.push_back(std::make_unique<WorkspaceTool>(
      WorkspaceTool::Op::kAppendFile, "append_file",
      "Append content to the end of a workspace file, creating it if needed. "
      "Use this to build a large file across several smaller calls instead of "
      "one huge create_file.",
      CreateInputProperties(
          {{"path", StringProperty("File path relative to the workspace "
                                   "root.")},
           {"content", StringProperty("Text to append to the end of the "
                                      "file.")}}),
      std::vector<std::string>{"path", "content"},
      /*requires_permission=*/true, service, conversation_id));

  tools.push_back(std::make_unique<WorkspaceTool>(
      WorkspaceTool::Op::kUndoEdit, "undo_edit",
      "Revert the most recent edit made to a file in the workspace.",
      CreateInputProperties(
          {{"path", StringProperty("File path relative to the workspace "
                                   "root.")}}),
      std::vector<std::string>{"path"}, /*requires_permission=*/true, service,
      conversation_id));

  return tools;
}

}  // namespace ai_chat
