#include "brave/browser/debugger/brave_debugger_service.h"

#include "brave/browser/debugger/brave_debugger_manager.h"
#include "content/public/browser/web_contents.h"

// static
void BraveDebuggerService::Create(
    content::RenderFrameHost* render_frame_host,
    mojo::PendingReceiver<brave::mojom::BraveDebugger> receiver) {
  new BraveDebuggerService(*render_frame_host, std::move(receiver));
}

BraveDebuggerService::BraveDebuggerService(
    content::RenderFrameHost& render_frame_host,
    mojo::PendingReceiver<brave::mojom::BraveDebugger> receiver)
    : DocumentService(render_frame_host, std::move(receiver)) {}

BraveDebuggerService::~BraveDebuggerService() = default;

void BraveDebuggerService::AttachToTarget(const url::Origin& origin,
                                         const std::string& target_id,
                                         const std::string& version,
                                         AttachToTargetCallback callback) {
  content::WebContents* web_contents =
      content::WebContents::FromRenderFrameHost(&render_frame_host());
  if (!web_contents) {
    std::move(callback).Run(false, "WebContents not found");
    return;
  }

  BraveDebuggerManager* manager =
      BraveDebuggerManager::GetOrCreateForWebContents(web_contents);
  if (!manager) {
    std::move(callback).Run(false, "Failed to create debugger manager");
    return;
  }

  manager->AttachDebugger(origin, target_id, version, std::move(callback));
}

void BraveDebuggerService::DetachFromTarget(const url::Origin& origin,
                                           const std::string& target_id,
                                           DetachFromTargetCallback callback) {
  content::WebContents* web_contents =
      content::WebContents::FromRenderFrameHost(&render_frame_host());
  if (!web_contents) {
    std::move(callback).Run(false, "WebContents not found");
    return;
  }

  BraveDebuggerManager* manager =
      BraveDebuggerManager::GetOrCreateForWebContents(web_contents);
  if (!manager) {
    std::move(callback).Run(false, "Debugger manager not found");
    return;
  }

  manager->DetachDebugger(origin, target_id, std::move(callback));
}

void BraveDebuggerService::SendCommand(const url::Origin& origin,
                                      const std::string& target_id,
                                      const std::string& method,
                                      base::Value::Dict params,
                                      SendCommandCallback callback) {
  content::WebContents* web_contents =
      content::WebContents::FromRenderFrameHost(&render_frame_host());
  if (!web_contents) {
    std::move(callback).Run(false, base::Value::Dict(), "WebContents not found");
    return;
  }

  BraveDebuggerManager* manager =
      BraveDebuggerManager::GetOrCreateForWebContents(web_contents);
  if (!manager) {
    std::move(callback).Run(false, base::Value::Dict(), "Debugger manager not found");
    return;
  }

  manager->SendDebuggerCommand(origin, target_id, method, std::move(params),
                              std::move(callback));
}

void BraveDebuggerService::GetTargets(const url::Origin& origin,
                                     GetTargetsCallback callback) {
  content::WebContents* web_contents =
      content::WebContents::FromRenderFrameHost(&render_frame_host());
  if (!web_contents) {
    std::move(callback).Run(std::vector<brave::mojom::DebuggerTargetPtr>());
    return;
  }

  BraveDebuggerManager* manager =
      BraveDebuggerManager::GetOrCreateForWebContents(web_contents);
  if (!manager) {
    std::move(callback).Run(std::vector<brave::mojom::DebuggerTargetPtr>());
    return;
  }

  manager->GetDebuggerTargets(origin, base::BindOnce(
      [](GetTargetsCallback callback, std::vector<base::Value::Dict> targets) {
        std::vector<brave::mojom::DebuggerTargetPtr> mojo_targets;
        for (const auto& target : targets) {
          auto mojo_target = brave::mojom::DebuggerTarget::New();

          if (const std::string* id = target.FindString("id")) {
            mojo_target->id = *id;
          }
          if (const std::string* type = target.FindString("type")) {
            mojo_target->type = *type;
          }
          if (const std::string* title = target.FindString("title")) {
            mojo_target->title = *title;
          }
          if (const std::string* url = target.FindString("url")) {
            mojo_target->url = *url;
          }
          if (std::optional<bool> attached = target.FindBool("attached")) {
            mojo_target->attached = *attached;
          }

          mojo_targets.push_back(std::move(mojo_target));
        }
        std::move(callback).Run(std::move(mojo_targets));
      },
      std::move(callback)));
}