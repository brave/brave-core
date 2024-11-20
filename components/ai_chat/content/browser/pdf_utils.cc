/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/content/browser/pdf_utils.h"

#include "base/strings/strcat.h"
#include "components/strings/grit/components_strings.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/web_contents.h"
#include "pdf/buildflags.h"
#include "services/strings/grit/services_strings.h"
#include "ui/accessibility/ax_node.h"
#include "ui/accessibility/ax_tree_manager.h"
#include "ui/base/l10n/l10n_util.h"

namespace ai_chat {

namespace {

ui::AXNode* FindPdfRoot(const ui::AXNode* start_node) {
  if (!start_node) {
    return nullptr;
  }
  for (const auto& node : start_node->GetAllChildren()) {
    if (node->GetRole() == ax::mojom::Role::kPdfRoot) {
      return node;
    }
    ui::AXNode* result = FindPdfRoot(node);
    if (result) {
      return result;
    }
  }
  return nullptr;
}

}  // namespace

bool IsPdf(content::WebContents* web_contents) {
  return web_contents->GetContentsMimeType() == "application/pdf";
}

ui::AXNode* GetPdfRoot(content::RenderFrameHost* primary_rfh) {
  ui::AXTreeManager* ax_tree_manager = nullptr;
  // FindPdfChildFrame
  primary_rfh->ForEachRenderFrameHost(
      [&ax_tree_manager](content::RenderFrameHost* rfh) {
        if (!rfh->GetProcess()->IsPdf()) {
          return;
        }
        ui::AXTreeID ax_tree_id = rfh->GetAXTreeID();
        if (ax_tree_id.type() == ax::mojom::AXTreeIDType::kUnknown) {
          return;
        }
        ax_tree_manager = ui::AXTreeManager::FromID(ax_tree_id);
      });
  if (!ax_tree_manager) {
    return nullptr;
  }
  return FindPdfRoot(ax_tree_manager->GetRoot());
}

bool IsPdfLoaded(const ui::AXNode* pdf_root) {
  if (!pdf_root || pdf_root->GetChildCount() < 2 ||
      pdf_root->GetChildAtIndex(0)->GetRole() != ax::mojom::Role::kBanner ||
      pdf_root->GetChildAtIndex(0)->IsEmptyLeaf() ||
      pdf_root->GetChildAtIndex(0)->GetChildAtIndex(0)->GetRole() !=
          ax::mojom::Role::kStatus) {
    return false;
  }

#if BUILDFLAG(ENABLE_PDF)
  const auto& name =
      pdf_root->GetChildAtIndex(0)->GetChildAtIndex(0)->GetStringAttribute(
          ax::mojom::StringAttribute::kName);
  if (name == l10n_util::GetStringUTF8(IDS_PDF_LOADING_TO_A11Y_TREE)) {
    return false;
  }
#endif

  return true;
}

std::string ExtractPdfContent(const ui::AXNode* pdf_root) {
  // Skip status subtree and get text from region siblings
  if (!pdf_root || pdf_root->GetChildCount() < 2 ||
      pdf_root->GetChildAtIndex(0)->GetRole() != ax::mojom::Role::kBanner) {
    return std::string();
  }
  std::string pdf_content;
  const auto& children = pdf_root->GetAllChildren();
  for (auto it = children.cbegin() + 1; it != children.cend(); ++it) {
    const ui::AXNode* node = *it;
    if (node->GetRole() == ax::mojom::Role::kRegion) {
      base::StrAppend(&pdf_content, {node->GetTextContentUTF8(),
                                     it == children.cend() - 1 ? "" : "\n"});
    }
  }
  return pdf_content;
}

}  // namespace ai_chat
