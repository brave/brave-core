#include "brave/browser/debugger/brave_debugger_client_host.h"

#include <tuple>
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/string_view_util.h"
#include "brave/components/permissions/contexts/brave_puppeteer_permission_context.h"
#include "content/browser/devtools/render_frame_devtools_agent_host.h"
#include "content/browser/renderer_host/frame_tree_node.h"
#include "content/browser/renderer_host/render_frame_host_impl.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/devtools_agent_host.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"

using content::DevToolsAgentHost;

BraveDebuggerClientHost::BraveDebuggerClientHost(
    content::WebContents* web_contents,
    const url::Origin& requesting_origin)
    : WebContentsObserver(web_contents),
      web_contents_(web_contents),
      requesting_origin_(requesting_origin) {}

BraveDebuggerClientHost::~BraveDebuggerClientHost() {
  DetachAll();
}

std::string BraveDebuggerClientHost::GetTypeForMetrics() {
  return "Other";
}

BraveDebuggerClientHost::PendingCommand::PendingCommand() = default;
BraveDebuggerClientHost::PendingCommand::PendingCommand(int id, CommandCallback callback, const std::string& target_id)
    : id(id), callback(std::move(callback)), target_id(target_id) {}
BraveDebuggerClientHost::PendingCommand::~PendingCommand() = default;

void BraveDebuggerClientHost::AttachToTarget(const std::string& target_id,
                                            const std::string& required_version,
                                            AttachCallback callback) {
  LOG(ERROR) << "BraveDebugger: AttachToTarget called with target_id=" << target_id;

  if (!HasPuppeteerPermission()) {
    LOG(ERROR) << "BraveDebugger: AttachToTarget - Permission denied";
    std::move(callback).Run(false, "Permission denied");
    return;
  }

  // Check if already attached to this target
  if (attached_agents_.find(target_id) != attached_agents_.end()) {
    LOG(ERROR) << "BraveDebugger: AttachToTarget - Already attached to target";
    std::move(callback).Run(false, "Already attached to target");
    return;
  }

  // Find the target among allowed targets
  auto allowed_targets = GetAllowedTargets();
  LOG(ERROR) << "BraveDebugger: AttachToTarget - Found " << allowed_targets.size() << " allowed targets";

  scoped_refptr<DevToolsAgentHost> target_agent;
  for (const auto& agent : allowed_targets) {
    LOG(ERROR) << "BraveDebugger: AttachToTarget - Checking agent id=" << agent->GetId() << " vs target_id=" << target_id;
    if (agent->GetId() == target_id) {
      target_agent = agent;
      break;
    }
  }

  if (!target_agent) {
    LOG(ERROR) << "BraveDebugger: AttachToTarget - Target not found or not allowed";
    std::move(callback).Run(false, "Target not found or not allowed");
    return;
  }

  // Check if target is already attached by someone else
  if (target_agent->IsAttached()) {
    LOG(ERROR) << "BraveDebugger: AttachToTarget - Target already attached by another client";
    std::move(callback).Run(false, "Target already attached by another client");
    return;
  }

  // Attempt to attach
  LOG(ERROR) << "BraveDebugger: AttachToTarget - Attempting to attach to target";
  if (target_agent->AttachClient(this)) {
    attached_agents_[target_id] = target_agent;
    LOG(ERROR) << "BraveDebugger: AttachToTarget - Successfully attached";
    std::move(callback).Run(true, "");
  } else {
    LOG(ERROR) << "BraveDebugger: AttachToTarget - Failed to attach to target";
    std::move(callback).Run(false, "Failed to attach to target");
  }
}

void BraveDebuggerClientHost::DetachFromTarget(const std::string& target_id,
                                              AttachCallback callback) {
  auto it = attached_agents_.find(target_id);
  if (it == attached_agents_.end()) {
    std::move(callback).Run(false, "Not attached to target");
    return;
  }

  it->second->DetachClient(this);
  attached_agents_.erase(it);
  std::move(callback).Run(true, "");
}

void BraveDebuggerClientHost::SendCommand(const std::string& target_id,
                                         const std::string& method,
                                         base::Value::Dict params,
                                         CommandCallback callback) {
  auto it = attached_agents_.find(target_id);
  if (it == attached_agents_.end()) {
    std::move(callback).Run(false, base::Value::Dict(), "Not attached to target");
    return;
  }

  int command_id = next_command_id_++;
  pending_commands_.try_emplace(command_id, command_id, std::move(callback), target_id);

  // Build CDP command
  base::Value::Dict protocol_request;
  protocol_request.Set("id", command_id);
  protocol_request.Set("method", method);
  if (!params.empty()) {
    protocol_request.Set("params", std::move(params));
  }

  std::string json = base::WriteJson(protocol_request).value_or("");
  it->second->DispatchProtocolMessage(this, base::as_byte_span(json));
}

void BraveDebuggerClientHost::GetTargets(TargetsCallback callback) {
  LOG(ERROR) << "BraveDebugger: GetTargets called";

  if (!HasPuppeteerPermission()) {
    LOG(ERROR) << "BraveDebugger: No Puppeteer permission";
    std::move(callback).Run(std::vector<base::Value::Dict>());
    return;
  }

  LOG(ERROR) << "BraveDebugger: Has permission, getting allowed targets";
  auto allowed_targets = GetAllowedTargets();
  std::vector<base::Value::Dict> targets;

  for (const auto& agent : allowed_targets) {
    base::Value::Dict target;
    target.Set("id", agent->GetId());
    target.Set("type", agent->GetType());
    target.Set("title", agent->GetTitle());
    target.Set("url", agent->GetURL().spec());
    target.Set("attached", agent->IsAttached());
    targets.push_back(std::move(target));
  }

  std::move(callback).Run(std::move(targets));
}

void BraveDebuggerClientHost::DispatchProtocolMessage(
    DevToolsAgentHost* agent_host,
    base::span<const uint8_t> message) {
  std::string_view message_str = base::as_string_view(message);
  std::optional<base::Value> result = base::JSONReader::Read(
      message_str, base::JSON_REPLACE_INVALID_CHARACTERS);

  if (!result || !result->is_dict()) {
    LOG(ERROR) << "Invalid CDP message received: " << message_str;
    return;
  }

  base::Value::Dict& dictionary = result->GetDict();
  std::optional<int> id = dictionary.FindInt("id");

  if (id.has_value()) {
    // This is a response to a command we sent
    auto it = pending_commands_.find(*id);
    if (it != pending_commands_.end()) {
      base::Value::Dict* error_dict = dictionary.FindDict("error");
      if (error_dict) {
        std::string* error_message = error_dict->FindString("message");
        std::string error = error_message ? *error_message : "Unknown error";
        std::move(it->second.callback).Run(false, base::Value::Dict(), error);
      } else {
        base::Value::Dict* result_dict = dictionary.FindDict("result");
        base::Value::Dict result_copy = result_dict ? result_dict->Clone() : base::Value::Dict();
        std::move(it->second.callback).Run(true, std::move(result_copy), "");
      }
      pending_commands_.erase(it);
    }
  }
  // Note: Events (messages without id) could be handled here if needed
}

void BraveDebuggerClientHost::AgentHostClosed(DevToolsAgentHost* agent_host) {
  // Find and remove the closed agent from our attached_agents_
  for (auto it = attached_agents_.begin(); it != attached_agents_.end(); ++it) {
    if (it->second.get() == agent_host) {
      attached_agents_.erase(it);
      break;
    }
  }

  // Cancel any pending commands for this agent
  for (auto it = pending_commands_.begin(); it != pending_commands_.end();) {
    if (attached_agents_.find(it->second.target_id) == attached_agents_.end()) {
      std::move(it->second.callback).Run(false, base::Value::Dict(), "Target closed");
      it = pending_commands_.erase(it);
    } else {
      ++it;
    }
  }
}

bool BraveDebuggerClientHost::MayAttachToURL(const GURL& url, bool is_webui) {
  if (is_webui) {
    return false;
  }
  return HasPuppeteerPermission();
}

bool BraveDebuggerClientHost::MayAttachToRenderFrameHost(
    content::RenderFrameHost* render_frame_host) {
  if (!HasPuppeteerPermission()) {
    return false;
  }

  // Only allow attaching to frames within the same WebContents
  return content::WebContents::FromRenderFrameHost(render_frame_host) == web_contents_;
}

void BraveDebuggerClientHost::WebContentsDestroyed() {
  DetachAll();
}

bool BraveDebuggerClientHost::HasPuppeteerPermission() const {
  if (!web_contents_) {
    return false;
  }

  return BravePuppeteerPermissionContext::IsOriginAllowedForPuppeteerMode(
      web_contents_->GetBrowserContext(), requesting_origin_);
}

std::vector<scoped_refptr<DevToolsAgentHost>>
BraveDebuggerClientHost::GetAllowedTargets() {
  std::vector<scoped_refptr<DevToolsAgentHost>> allowed_targets;

  if (!web_contents_) {
    return allowed_targets;
  }

  // Get all existing DevTools agent hosts from the system
  auto all_hosts = DevToolsAgentHost::GetOrCreateAll();

  LOG(ERROR) << "BraveDebugger: Total DevToolsAgentHosts found: " << all_hosts.size();

  // Filter for iframe-type hosts that belong to our WebContents
  for (const auto& host : all_hosts) {
    LOG(ERROR) << "BraveDebugger: Checking host type=" << host->GetType()
              << " url=" << host->GetURL().spec()
              << " title=" << host->GetTitle()
              << " webcontent_match=" << (host->GetWebContents() == web_contents_);

    // Only include iframe targets, not page/tab targets
    if (host->GetType() != DevToolsAgentHost::kTypeFrame) {
      continue;
    }

    // Check if this host belongs to our WebContents
    if (host->GetWebContents() == web_contents_) {
      LOG(ERROR) << "BraveDebugger: Adding allowed iframe target: " << host->GetType();
      allowed_targets.push_back(host);
    }
  }

  LOG(ERROR) << "BraveDebugger: Final allowed iframe targets: " << allowed_targets.size();
  return allowed_targets;
}

void BraveDebuggerClientHost::DetachAll() {
  // Detach from all agents
  for (const auto& pair : attached_agents_) {
    pair.second->DetachClient(this);
  }
  attached_agents_.clear();

  // Cancel all pending commands
  for (auto& pair : pending_commands_) {
    std::move(pair.second.callback).Run(false, base::Value::Dict(), "Detached");
  }
  pending_commands_.clear();
}