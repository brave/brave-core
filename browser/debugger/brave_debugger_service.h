#ifndef BRAVE_BROWSER_DEBUGGER_BRAVE_DEBUGGER_SERVICE_H_
#define BRAVE_BROWSER_DEBUGGER_BRAVE_DEBUGGER_SERVICE_H_

#include "brave/common/brave_debugger.mojom.h"
#include "content/public/browser/document_service.h"
#include "content/public/browser/render_frame_host.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"

// Browser-side implementation of the BraveDebugger Mojo interface
class BraveDebuggerService : public content::DocumentService<brave::mojom::BraveDebugger> {
 public:
  static void Create(content::RenderFrameHost* render_frame_host,
                    mojo::PendingReceiver<brave::mojom::BraveDebugger> receiver);

  BraveDebuggerService(content::RenderFrameHost& render_frame_host,
                      mojo::PendingReceiver<brave::mojom::BraveDebugger> receiver);
  ~BraveDebuggerService() override;

  BraveDebuggerService(const BraveDebuggerService&) = delete;
  BraveDebuggerService& operator=(const BraveDebuggerService&) = delete;

  // brave::mojom::BraveDebugger implementation:
  void AttachToTarget(const url::Origin& origin,
                     const std::string& target_id,
                     const std::string& version,
                     AttachToTargetCallback callback) override;

  void DetachFromTarget(const url::Origin& origin,
                       const std::string& target_id,
                       DetachFromTargetCallback callback) override;

  void SendCommand(const url::Origin& origin,
                  const std::string& target_id,
                  const std::string& method,
                  base::Value::Dict params,
                  SendCommandCallback callback) override;

  void GetTargets(const url::Origin& origin,
                 GetTargetsCallback callback) override;
};

#endif  // BRAVE_BROWSER_DEBUGGER_BRAVE_DEBUGGER_SERVICE_H_