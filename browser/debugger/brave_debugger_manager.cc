#include "brave/browser/debugger/brave_debugger_manager.h"

#include "brave/browser/debugger/brave_debugger_client_host.h"
#include "content/public/browser/web_contents.h"

BraveDebuggerManager::BraveDebuggerManager(content::WebContents* web_contents)
    : WebContentsObserver(web_contents),
      content::WebContentsUserData<BraveDebuggerManager>(*web_contents) {}

BraveDebuggerManager::~BraveDebuggerManager() = default;

void BraveDebuggerManager::AttachDebugger(const url::Origin& origin,
                                         const std::string& target_id,
                                         const std::string& version,
                                         AttachCallback callback) {
  BraveDebuggerClientHost* client_host = GetOrCreateClientHost(origin);
  if (!client_host) {
    std::move(callback).Run(false, "Failed to create debugger client");
    return;
  }

  client_host->AttachToTarget(target_id, version, std::move(callback));
}

void BraveDebuggerManager::DetachDebugger(const url::Origin& origin,
                                         const std::string& target_id,
                                         AttachCallback callback) {
  BraveDebuggerClientHost* client_host = GetOrCreateClientHost(origin);
  if (!client_host) {
    std::move(callback).Run(false, "No debugger client for origin");
    return;
  }

  client_host->DetachFromTarget(target_id, std::move(callback));
}

void BraveDebuggerManager::SendDebuggerCommand(const url::Origin& origin,
                                              const std::string& target_id,
                                              const std::string& method,
                                              base::Value::Dict params,
                                              CommandCallback callback) {
  BraveDebuggerClientHost* client_host = GetOrCreateClientHost(origin);
  if (!client_host) {
    std::move(callback).Run(false, base::Value::Dict(), "No debugger client for origin");
    return;
  }

  client_host->SendCommand(target_id, method, std::move(params), std::move(callback));
}

void BraveDebuggerManager::GetDebuggerTargets(const url::Origin& origin,
                                             TargetsCallback callback) {
  BraveDebuggerClientHost* client_host = GetOrCreateClientHost(origin);
  if (!client_host) {
    std::move(callback).Run(std::vector<base::Value::Dict>());
    return;
  }

  client_host->GetTargets(std::move(callback));
}

void BraveDebuggerManager::WebContentsDestroyed() {
  // Clean up all client hosts
  client_hosts_.clear();
}

BraveDebuggerClientHost* BraveDebuggerManager::GetOrCreateClientHost(
    const url::Origin& origin) {
  auto it = client_hosts_.find(origin);
  if (it != client_hosts_.end()) {
    return it->second.get();
  }

  // Create new client host for this origin
  auto client_host = std::make_unique<BraveDebuggerClientHost>(
      web_contents(), origin);
  BraveDebuggerClientHost* raw_ptr = client_host.get();
  client_hosts_[origin] = std::move(client_host);

  return raw_ptr;
}

void BraveDebuggerManager::RemoveClientHost(const url::Origin& origin) {
  client_hosts_.erase(origin);
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(BraveDebuggerManager);