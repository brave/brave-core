// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/renderer/page_text_distilling.h"

#include <iterator>
#include <map>
#include <memory>
#include <optional>
#include <queue>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include "base/compiler_specific.h"
#include "base/containers/contains.h"
#include "base/containers/fixed_flat_map.h"
#include "base/containers/span.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "base/numerics/safe_math.h"
#include "base/ranges/algorithm.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/ai_chat/resources/custom_site_distiller_scripts/grit/custom_site_distiller_scripts_generated.h"
#include "content/public/renderer/render_frame.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "third_party/blink/public/mojom/script/script_evaluation_params.mojom-shared.h"
#include "third_party/blink/public/platform/web_string.h"
#include "third_party/blink/public/web/web_document.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "third_party/blink/public/web/web_script_source.h"
#include "ui/accessibility/ax_enums.mojom-shared.h"
#include "ui/accessibility/ax_mode.h"
#include "ui/accessibility/ax_node.h"
#include "ui/accessibility/ax_node_data.h"
#include "ui/accessibility/ax_tree.h"
#include "ui/accessibility/ax_tree_update.h"
#include "ui/base/resource/resource_bundle.h"

namespace ai_chat {

namespace {

static const ax::mojom::Role kContentParentRoles[]{
    ax::mojom::Role::kMain,
    ax::mojom::Role::kArticle,
};

static const ax::mojom::Role kContentRoles[]{
    ax::mojom::Role::kHeading,
    ax::mojom::Role::kParagraph,
    ax::mojom::Role::kNote,
};

static const ax::mojom::Role kRolesToSkip[]{
    ax::mojom::Role::kAudio,
    ax::mojom::Role::kBanner,
    ax::mojom::Role::kButton,
    ax::mojom::Role::kComplementary,
    ax::mojom::Role::kContentInfo,
    ax::mojom::Role::kFooter,
    ax::mojom::Role::kImage,
    ax::mojom::Role::kLabelText,
    ax::mojom::Role::kNavigation,
    ax::mojom::Role::kSectionFooter,
    /* input elements */
    ax::mojom::Role::kTextField,
    ax::mojom::Role::kTextFieldWithComboBox,
    ax::mojom::Role::kComboBoxSelect,
    ax::mojom::Role::kListBox,
    ax::mojom::Role::kListBoxOption,
    ax::mojom::Role::kCheckBox,
    ax::mojom::Role::kRadioButton,
    ax::mojom::Role::kSlider,
    ax::mojom::Role::kSpinButton,
    ax::mojom::Role::kSearchBox,
};

void GetContentRootNodes(const ui::AXNode* root,
                         std::vector<const ui::AXNode*>* content_root_nodes) {
  std::queue<const ui::AXNode*> queue;
  queue.push(root);
  while (!queue.empty()) {
    const ui::AXNode* node = queue.front();
    queue.pop();
    // If a main or article node is found, add it to the list of content root
    // nodes and continue. Do not explore children for nested article nodes.
    if (base::Contains(kContentParentRoles, node->GetRole())) {
      content_root_nodes->push_back(node);
      continue;
    }
    for (auto iter = node->UnignoredChildrenBegin();
         iter != node->UnignoredChildrenEnd(); ++iter) {
      queue.push(iter.get());
    }
  }
}

void AddContentNodesToVector(const ui::AXNode* node,
                             std::vector<const ui::AXNode*>* content_nodes) {
  if (base::Contains(kContentRoles, node->GetRole())) {
    content_nodes->emplace_back(node);
    return;
  }
  if (base::Contains(kRolesToSkip, node->GetRole())) {
    return;
  }
  for (auto iter = node->UnignoredChildrenBegin();
       iter != node->UnignoredChildrenEnd(); ++iter) {
    AddContentNodesToVector(iter.get(), content_nodes);
  }
}

void AddTextNodesToVector(const ui::AXNode* node,
                          std::vector<std::u16string>* strings) {
  const ui::AXNodeData& node_data = node->data();

  if (base::Contains(kRolesToSkip, node_data.role)) {
    return;
  }

  if (node_data.role == ax::mojom::Role::kStaticText) {
    if (node_data.HasStringAttribute(ax::mojom::StringAttribute::kName)) {
      strings->push_back(
          node_data.GetString16Attribute(ax::mojom::StringAttribute::kName));
    }
    return;
  }

  for (const ui::AXNode* child : node->children()) {
    AddTextNodesToVector(child, strings);
  }
}

}  // namespace

void DistillPageText(
    content::RenderFrame* render_frame,
    int32_t global_world_id,
    int32_t isolated_world_id,
    base::OnceCallback<void(const std::optional<std::string>&)> callback) {
  if (ai_chat::features::IsCustomSiteDistillerScriptsEnabled()) {
    std::string host =
        render_frame->GetWebFrame()->GetSecurityOrigin().Host().Utf8();
    std::optional<std::pair<std::string, bool>> site_script =
        LoadSiteScriptForHost(host);

    if (site_script.has_value()) {
      int32_t world_id =
          site_script->second ? global_world_id : isolated_world_id;
      DistillPageTextViaSiteScript(render_frame, site_script->first, world_id,
                                   std::move(callback));
      return;
    }
  }

  auto snapshotter = render_frame->CreateAXTreeSnapshotter(
      ui::AXMode::kWebContents | ui::AXMode::kHTML | ui::AXMode::kScreenReader);
  ui::AXTreeUpdate snapshot;
  snapshotter->Snapshot(
      /* max_nodes= */ 9000, /* timeout= */ base::Seconds(4), &snapshot);
  ui::AXTree tree(snapshot);

  std::vector<const ui::AXNode*> content_root_nodes;
  std::vector<const ui::AXNode*> content_nodes;
  GetContentRootNodes(tree.root(), &content_root_nodes);

  for (const ui::AXNode* content_root_node : content_root_nodes) {
    std::vector<const ui::AXNode*> content_nodes_this_root;
    AddContentNodesToVector(content_root_node, &content_nodes_this_root);
    // If we didn't get any content for this root node, then fallback to using
    // the text directly from the root node. We do this because at least we've
    // identified this root node should be where the important content is.
    if (content_nodes_this_root.empty()) {
      content_nodes.emplace_back(content_root_node);
    } else {
      base::ranges::move(content_nodes_this_root,
                         std::back_inserter(content_nodes));
    }
  }

  std::vector<std::u16string> text_node_contents;
  for (const ui::AXNode* content_node : content_nodes) {
    AddTextNodesToVector(content_node, &text_node_contents);
  }

  std::string contents_text =
      base::UTF16ToUTF8(base::JoinString(text_node_contents, u" "));

  if (contents_text.empty()) {
    blink::WebScriptSource source = blink::WebScriptSource(
        blink::WebString::FromASCII("document.body.innerText"));

    auto on_script_executed =
        [](base::OnceCallback<void(const std::optional<std::string>&)> callback,
           std::optional<base::Value> value, base::TimeTicks start_time) {
          if (value->is_string()) {
            std::move(callback).Run(value->GetString());
            return;
          }

          std::move(callback).Run({});
        };

    render_frame->GetWebFrame()->RequestExecuteScript(
        isolated_world_id, base::span_from_ref(source),
        blink::mojom::UserActivationOption::kDoNotActivate,
        blink::mojom::EvaluationTiming::kAsynchronous,
        blink::mojom::LoadEventBlockingOption::kDoNotBlock,
        base::BindOnce(on_script_executed, std::move(callback)),
        blink::BackForwardCacheAware::kAllow,
        blink::mojom::WantResultOption::kWantResult,
        blink::mojom::PromiseResultOption::kAwait);
    return;
  }

  std::move(callback).Run(contents_text);
}

void DistillPageTextViaSiteScript(
    content::RenderFrame* render_frame,
    std::string_view script_content,
    int32_t world_id,
    base::OnceCallback<void(const std::optional<std::string>&)> callback) {
  CHECK(ai_chat::features::IsCustomSiteDistillerScriptsEnabled());
  // TODO (jonathansampson): Wrap scripts at build/transpile-time instead
  // This produces an injected script that resembles the following:
  // (() => {
  //   function distillPrimaryColumn (level) { ... }
  //   function distill(level) {
  //     return distillPrimaryColumn(level);
  //   }
  //   return distill(3);
  // })())
  std::string script = absl::StrFormat(
      R"((()=> {
        %s
        return distill(3);
      })())",
      script_content);

  blink::WebScriptSource source =
      blink::WebScriptSource(blink::WebString::FromUTF8(script));

  auto on_script_executed =
      [](base::OnceCallback<void(const std::optional<std::string>&)> callback,
         std::optional<base::Value> value, base::TimeTicks start_time) {
        if (value && value->is_string() && !value->GetString().empty()) {
          std::move(callback).Run(value->GetString());
        } else {
          std::move(callback).Run({});
        }
      };

  // Execute the combined script as a single source
  render_frame->GetWebFrame()->RequestExecuteScript(
      world_id, base::span_from_ref(source),
      blink::mojom::UserActivationOption::kDoNotActivate,
      blink::mojom::EvaluationTiming::kAsynchronous,
      blink::mojom::LoadEventBlockingOption::kDoNotBlock,
      base::BindOnce(on_script_executed, std::move(callback)),
      blink::BackForwardCacheAware::kAllow,
      blink::mojom::WantResultOption::kWantResult,
      // Because we are using a promise to resolve the result, we will use the
      // `kAwait` option to ensure the promise is resolved before the callback
      // is invoked.
      blink::mojom::PromiseResultOption::kAwait);
}

std::optional<std::pair<std::string, bool>> LoadSiteScriptForHost(
    std::string_view host) {
  static constexpr auto kHostToScriptResource =
      base::MakeFixedFlatMap<std::string_view, std::pair<int, bool>>({
          {"github.com",
           {IDR_CUSTOM_SITE_DISTILLER_SCRIPTS_GITHUB_COM_BUNDLE_JS, false}},
          {"x.com", {IDR_CUSTOM_SITE_DISTILLER_SCRIPTS_X_COM_BUNDLE_JS, true}},
      });

  auto it = kHostToScriptResource.find(
      net::registry_controlled_domains::GetDomainAndRegistry(
          host, net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES));

  if (it == kHostToScriptResource.end()) {
    return std::nullopt;
  }

  return std::make_optional(std::make_pair(
      ui::ResourceBundle::GetSharedInstance().LoadDataResourceString(
          it->second.first),
      it->second.second));
}

}  // namespace ai_chat
