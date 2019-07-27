/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/brave_page_graph/scripts/script_tracker.h"
#include <map>
#include <vector>
#include "third_party/blink/renderer/bindings/core/v8/script_source_code.h"
#include "third_party/blink/renderer/core/dom/dom_node_ids.h"
#include "third_party/blink/renderer/platform/weborigin/kurl.h"
#include "brave/third_party/blink/brave_page_graph/logging.h"
#include "brave/third_party/blink/brave_page_graph/types.h"

using ::blink::DOMNodeId;
using ::blink::KURL;
using ::blink::ScriptSourceCode;
using ::std::map;
using ::std::vector;

namespace brave_page_graph {

ScriptTracker::ScriptTracker() {}

ScriptTracker::~ScriptTracker() {}

void ScriptTracker::AddScriptUrlForElm(const KURL& url,
    const DOMNodeId node_id) {
  const UrlHash url_hash(url.GetString().Impl()->GetHash());
  if (node_id_to_script_url_hashes_.count(node_id) == 0) {
    node_id_to_script_url_hashes_.emplace(node_id, vector<UrlHash>());
  }
  node_id_to_script_url_hashes_.at(node_id).push_back(url_hash);

  if (script_src_hash_to_node_ids_.count(url_hash) == 0) {
    script_src_hash_to_node_ids_.emplace(url_hash, DOMNodeIdList());
  }
  script_src_hash_to_node_ids_.at(url_hash).push_back(node_id);
}

void ScriptTracker::AddScriptSourceForElm(const ScriptSourceCode& code,
    const DOMNodeId node_id) {
  const SourceCodeHash code_hash(code.Source().ToString().Impl()->GetHash());
  if (node_id_to_source_hashes_.count(node_id) == 0) {
    node_id_to_source_hashes_.emplace(node_id, vector<SourceCodeHash>());
  }
  node_id_to_source_hashes_.at(node_id).push_back(code_hash);

  if (source_hash_to_node_ids_.count(code_hash) == 0) {
    source_hash_to_node_ids_.emplace(code_hash, DOMNodeIdList());
  }
  source_hash_to_node_ids_.at(code_hash).push_back(node_id);
}

void ScriptTracker::AddCodeFetchedFromUrl(
    const ScriptSourceCode& code, const KURL& url) {
  const SourceCodeHash code_hash(code.Source().ToString().Impl()->GetHash());
  const UrlHash url_hash(url.GetString().Impl()->GetHash());

  // There should be no situations where we're receiving script code
  // from an unknown URL.
  LOG_ASSERT(script_src_hash_to_node_ids_.count(url_hash) > 0);
  script_url_hash_to_source_hash_.emplace(url_hash, code_hash);
  source_hash_to_script_url_hash_.emplace(code_hash, url_hash);
}

void ScriptTracker::AddExtensionCodeFetchedFromUrl(
    const SourceCodeHash code_hash, const UrlHash url_hash) {
  extension_script_url_hash_to_source_hash_.emplace(url_hash, code_hash);
  extension_source_hash_to_script_url_hash_.emplace(code_hash, url_hash);
}

void ScriptTracker::SetScriptIdForCode(const ScriptId script_id,
    const ScriptSourceCode& code) {
  const SourceCodeHash code_hash(code.Source().ToString().Impl()->GetHash());
  // There should be no situtions where V8 has compiled source code that
  // we don't know about (TODO: handle cases of partial compilation,
  // eval, and similar).
  LOG_ASSERT(
    source_hash_to_script_url_hash_.count(code_hash) > 0 ||
    source_hash_to_node_ids_.count(code_hash) > 0 ||
    extension_source_hash_to_script_url_hash_.count(code_hash) > 0);

  if (extension_source_hash_to_script_url_hash_.count(code_hash) == 1) {
    extension_source_hash_to_script_id_.emplace(code_hash, script_id);
    script_id_to_extension_source_hash_.emplace(script_id, code_hash);
  } else {
    source_hash_to_script_id_.emplace(code_hash, script_id);
    script_id_to_source_hash_.emplace(script_id, code_hash);
  }
}

ScriptTrackerScriptSource ScriptTracker::GetSourceOfScript(
    const ScriptId script_id) const {
  // Make sure that we know about this script id, and that its
  // associated with either code from an extension, or from a page,
  // but not both.
  LOG_ASSERT(script_id_to_extension_source_hash_.count(script_id)
    + script_id_to_source_hash_.count(script_id) == 1);

  if (script_id_to_extension_source_hash_.count(script_id) == 1) {
    return kScriptTrackerScriptSourceExtension;
  }

  if (script_id_to_source_hash_.count(script_id) == 1) {
    return kScriptTrackerScriptSourcePage;
  }

  return kScriptTrackerScriptSourceUnknown;
}

DOMNodeIdList ScriptTracker::GetElmsForScriptId(
    const ScriptId script_id) const {
  DOMNodeIdList node_ids;

  // If we've never seen this code before, then we trivially can't know
  // what HTML nodes it belongs to, so return an empty vector.
  if (script_id_to_source_hash_.count(script_id) == 0) {
    return node_ids;
  }

  const SourceCodeHash source_hash = script_id_to_source_hash_.at(script_id);

  if (source_hash_to_node_ids_.count(source_hash) > 0) {
    for (const auto& a_node_id : source_hash_to_node_ids_.at(source_hash)) {
      node_ids.push_back(a_node_id);
    }
  }

  if (source_hash_to_script_url_hash_.count(source_hash) > 0) {
    const UrlHash url_hash = source_hash_to_script_url_hash_.at(source_hash);
    if (script_src_hash_to_node_ids_.count(url_hash) > 0) {
      for (const auto& a_node_id : script_src_hash_to_node_ids_.at(url_hash)) {
        node_ids.push_back(a_node_id);
      }
    }
  }

  return node_ids;
}

ScriptIdList ScriptTracker::GetScriptIdsForElm(
    const DOMNodeId node_id) const {
  ScriptIdList script_ids;

  const bool node_has_urls = node_id_to_script_url_hashes_.count(node_id) > 0;
  const bool node_has_sources = node_id_to_source_hashes_.count(node_id) > 0;

  if (node_has_urls == false && node_has_sources == false) {
    return script_ids;
  }

  if (node_has_urls) {
    for (const UrlHash& url_hash : node_id_to_script_url_hashes_.at(node_id)) {
      if (script_url_hash_to_source_hash_.count(url_hash) > 0) {
        const SourceCodeHash code_hash =
          script_url_hash_to_source_hash_.at(url_hash);
        const ScriptId script_id_for_code_hash =
          source_hash_to_script_id_.at(code_hash);
        script_ids.push_back(script_id_for_code_hash);
      }
    }
  }

  if (node_has_sources) {
    for (const auto& code_hash : node_id_to_source_hashes_.at(node_id)) {
      const ScriptId script_id_for_code_hash =
        source_hash_to_script_id_.at(code_hash);
      script_ids.push_back(script_id_for_code_hash);
    }
  }

  return script_ids;
}

void ScriptTracker::AddScriptId(const ScriptId script_id,
    const SourceCodeHash hash) {
  // Make sure we've either never seen this script before, or that it
  // appears to be the same script.
  LOG_ASSERT(script_id_hashes_.count(script_id) == 0 ||
    script_id_hashes_.at(script_id) == hash);
  script_id_hashes_.emplace(script_id, hash);
}

void ScriptTracker::AddScriptIdAlias(const ScriptId script_id,
                                     const ScriptId parent_script_id) {
  if (script_id == parent_script_id) {
    return;
  }

  LOG_ASSERT(script_id_aliases_.count(script_id) == 0 ||
      script_id_aliases_.at(script_id) == parent_script_id);
  script_id_aliases_.emplace(script_id, parent_script_id);
}

ScriptId ScriptTracker::ResolveScriptId(const ScriptId script_id) const {
  if (script_id_aliases_.count(script_id) == 1) {
    return script_id_aliases_.at(script_id);
  }
  return script_id;
}

}  // namespace brave_page_graph
