#ifndef BRAVE_BROWSER_DEBUGGER_BRAVE_DEBUGGER_CLIENT_HOST_H_
#define BRAVE_BROWSER_DEBUGGER_BRAVE_DEBUGGER_CLIENT_HOST_H_

#include <map>
#include <string>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "base/values.h"
#include "content/public/browser/devtools_agent_host.h"
#include "content/public/browser/devtools_agent_host_client.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"
#include "url/origin.h"

namespace content {
class RenderFrameHost;
}

// Manages CDP communication for Brave debugger API - similar to ExtensionDevToolsClientHost
// but designed for web-based access with puppeteer permissions
class BraveDebuggerClientHost : public content::DevToolsAgentHostClient,
                               public content::WebContentsObserver {
 public:
  using CommandCallback = base::OnceCallback<void(bool success,
                                                  base::Value::Dict result,
                                                  const std::string& error)>;
  using AttachCallback = base::OnceCallback<void(bool success, const std::string& error)>;
  using TargetsCallback = base::OnceCallback<void(std::vector<base::Value::Dict>)>;

  BraveDebuggerClientHost(content::WebContents* web_contents,
                         const url::Origin& requesting_origin);
  ~BraveDebuggerClientHost() override;

  BraveDebuggerClientHost(const BraveDebuggerClientHost&) = delete;
  BraveDebuggerClientHost& operator=(const BraveDebuggerClientHost&) = delete;

  // Main API methods (matching chrome.debugger API)
  void AttachToTarget(const std::string& target_id,
                     const std::string& required_version,
                     AttachCallback callback);
  void DetachFromTarget(const std::string& target_id, AttachCallback callback);
  void SendCommand(const std::string& target_id,
                   const std::string& method,
                   base::Value::Dict params,
                   CommandCallback callback);
  void GetTargets(TargetsCallback callback);

  // DevToolsAgentHostClient implementation
  void DispatchProtocolMessage(content::DevToolsAgentHost* agent_host,
                               base::span<const uint8_t> message) override;
  void AgentHostClosed(content::DevToolsAgentHost* agent_host) override;
  bool MayAttachToURL(const GURL& url, bool is_webui) override;
  bool MayAttachToRenderFrameHost(
      content::RenderFrameHost* render_frame_host) override;
  std::string GetTypeForMetrics() override;

  // WebContentsObserver implementation
  void WebContentsDestroyed() override;

 private:
  struct PendingCommand {
    PendingCommand();
    PendingCommand(int id, CommandCallback callback, const std::string& target_id);
    ~PendingCommand();

    int id;
    CommandCallback callback;
    std::string target_id;
  };

  bool HasPuppeteerPermission() const;
  std::vector<scoped_refptr<content::DevToolsAgentHost>> GetAllowedTargets();
  void OnCommandResponse(int command_id, base::Value::Dict result);
  void OnCommandError(int command_id, const std::string& error);
  void DetachAll();

  raw_ptr<content::WebContents> web_contents_;
  url::Origin requesting_origin_;

  // Map of target IDs to attached agent hosts
  std::map<std::string, scoped_refptr<content::DevToolsAgentHost>> attached_agents_;

  // Map of command IDs to pending callbacks
  std::map<int, PendingCommand> pending_commands_;
  int next_command_id_ = 1;

  base::WeakPtrFactory<BraveDebuggerClientHost> weak_factory_{this};
};

#endif  // BRAVE_BROWSER_DEBUGGER_BRAVE_DEBUGGER_CLIENT_HOST_H_