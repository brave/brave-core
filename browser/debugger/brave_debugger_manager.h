#ifndef BRAVE_BROWSER_DEBUGGER_BRAVE_DEBUGGER_MANAGER_H_
#define BRAVE_BROWSER_DEBUGGER_BRAVE_DEBUGGER_MANAGER_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "base/values.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"
#include "url/origin.h"

class BraveDebuggerClientHost;

// Manages debugger instances per WebContents - bridge between renderer and CDP
// This class manages debugger clients per origin and provides the interface
// for renderer processes to access debugger functionality
class BraveDebuggerManager : public content::WebContentsObserver,
                            public content::WebContentsUserData<BraveDebuggerManager> {
 public:
  using AttachCallback = base::OnceCallback<void(bool success, const std::string& error)>;
  using CommandCallback = base::OnceCallback<void(bool success,
                                                  base::Value::Dict result,
                                                  const std::string& error)>;
  using TargetsCallback = base::OnceCallback<void(std::vector<base::Value::Dict>)>;

  ~BraveDebuggerManager() override;

  // Called from renderer via IPC (to be implemented)
  void AttachDebugger(const url::Origin& origin,
                     const std::string& target_id,
                     const std::string& version,
                     AttachCallback callback);

  void DetachDebugger(const url::Origin& origin,
                     const std::string& target_id,
                     AttachCallback callback);

  void SendDebuggerCommand(const url::Origin& origin,
                          const std::string& target_id,
                          const std::string& method,
                          base::Value::Dict params,
                          CommandCallback callback);

  void GetDebuggerTargets(const url::Origin& origin,
                         TargetsCallback callback);

  // WebContentsObserver implementation
  void WebContentsDestroyed() override;

 private:
  friend class content::WebContentsUserData<BraveDebuggerManager>;
  explicit BraveDebuggerManager(content::WebContents* web_contents);

  BraveDebuggerClientHost* GetOrCreateClientHost(const url::Origin& origin);
  void RemoveClientHost(const url::Origin& origin);

  // Map of origins to their debugger clients
  std::map<url::Origin, std::unique_ptr<BraveDebuggerClientHost>> client_hosts_;

  base::WeakPtrFactory<BraveDebuggerManager> weak_factory_{this};

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

#endif  // BRAVE_BROWSER_DEBUGGER_BRAVE_DEBUGGER_MANAGER_H_