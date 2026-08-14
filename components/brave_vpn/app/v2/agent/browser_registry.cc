/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/app/v2/agent/browser_registry.h"

#include <memory>
#include <utility>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/notimplemented.h"
#include "base/sequence_checker.h"
#include "base/strings/strcat.h"
#include "base/task/sequenced_task_runner.h"
#include "brave/components/brave_vpn/app/v2/agent/browser_host_impl.h"
#include "build/build_config.h"
#include "components/named_mojo_ipc_server/named_mojo_ipc_server.h"

#if BUILDFLAG(IS_WIN)
#include "base/win/win_util.h"
#endif  // BUILDFLAG(IS_WIN)

namespace brave_vpn::v2 {
namespace {
// Oldest contract the agent still speaks; mojom::kProtocolVersion is the
// newest. Bumped only when support for older browsers is deliberately dropped,
// which makes every such bump a compatibility break worth reviewing.
constexpr uint32_t kMinSupportedProtocolVersion = 1;

named_mojo_ipc_server::EndpointOptions GetEndpointOptions(
    mojo::NamedPlatformChannel::ServerName server_name) {
  named_mojo_ipc_server::EndpointOptions endpoint_options(
      std::move(server_name),
      named_mojo_ipc_server::EndpointOptions::kUseIsolatedConnection);
#if BUILDFLAG(IS_WIN)
  std::wstring user_sid;
  CHECK(base::win::GetUserSidString(&user_sid));
  endpoint_options.security_descriptor =
      base::StrCat({L"D:(A;;GA;;;SY)(A;;GA;;;BA)(A;;GA;;;", user_sid, L")"});
  endpoint_options.include_peer_process_info = true;
#endif  // BUILDFLAG(IS_WIN)
  return endpoint_options;
}
}  // namespace

BrowserRegistry::BrowserRegistry(
    mojo::NamedPlatformChannel::ServerName server_name)
    : host_server_(std::make_unique<named_mojo_ipc_server::NamedMojoIpcServer<
                       brave_vpn::mojom::BrowserHostProvider>>(
          GetEndpointOptions(std::move(server_name)),
          base::BindRepeating(&BrowserRegistry::OnBrowserConnecting,
                              base::Unretained(this)))) {
  StartHostServer();
}

BrowserRegistry::BrowserRegistry(
    std::unique_ptr<named_mojo_ipc_server::IpcServer> host_server)
    : host_server_(std::move(host_server)) {
  StartHostServer();
}

BrowserRegistry::~BrowserRegistry() = default;

// static
std::unique_ptr<BrowserRegistry> BrowserRegistry::CreateForTesting(  // IN-TEST
    std::unique_ptr<named_mojo_ipc_server::IpcServer> host_server) {
  return base::WrapUnique(new BrowserRegistry(std::move(host_server)));
}

void BrowserRegistry::StartHostServer() {
  VLOG(1) << "Starting IPC server";
  host_server_->set_disconnect_handler(base::BindRepeating(
      &BrowserRegistry::OnHostProviderDisconnected, base::Unretained(this)));
  host_server_->StartServer();
}

brave_vpn::mojom::BrowserHostProvider* BrowserRegistry::OnBrowserConnecting(
    const named_mojo_ipc_server::ConnectionInfo& info) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  VLOG(1) << "New browser connecting (pid " << info.pid << ")";
  return &host_provider_;
}

void BrowserRegistry::Authenticate(
    uint32_t protocol_version,
    mojo::PendingRemote<mojom::BrowserEndpoint> browser_endpoint,
    mojo::PendingReceiver<mojom::BrowserHost> host,
    base::OnceCallback<void(mojom::BrowserAuthResult)> callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  // Called synchronously from BindBrowserHost(), so the calling browser is
  // still the current receiver. It is the only handle on the connection that
  // survives the verification hop below.
  const mojo::ReceiverId receiver_id = host_server_->current_receiver();

  // Every version in [kMinSupportedProtocolVersion, kProtocolVersion] is
  // accepted, so a browser and agent from different updates still interoperate.
  // A browser newer than this agent is refused, because it would go on to call
  // methods this build does not implement.
  if (protocol_version < kMinSupportedProtocolVersion ||
      protocol_version > mojom::kProtocolVersion) {
    VLOG(1) << "Refusing browser " << receiver_id << ": protocol version "
            << protocol_version << " outside supported range ["
            << kMinSupportedProtocolVersion << ", " << mojom::kProtocolVersion
            << "]";
    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE, base::BindOnce(std::move(callback),
                                  mojom::BrowserAuthResult::kVersionMismatch));
    return;
  }

  // One BrowserHost per connection, whether it is already bound or still being
  // verified. A browser that drops its host may ask again on the same
  // connection.
  if (host_sessions_.contains(receiver_id) ||
      pending_auth_.contains(receiver_id)) {
    VLOG(1) << "Refusing browser " << receiver_id
            << ": authentication in progress or succeeded already";
    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE,
        base::BindOnce(std::move(callback),
                       mojom::BrowserAuthResult::kHostAlreadyRequested));
    return;
  }

  pending_auth_.insert(receiver_id);
  PendingAuth pending{.receiver_id = receiver_id,
                      .browser_endpoint = std::move(browser_endpoint),
                      .host = std::move(host),
                      .reply = std::move(callback)};

  // TODO(https://github.com/brave/brave-browser/issues/54623)
  // Real peer verification will be asynchronous (inspecting the peer process,
  // checking signatures), so the hop is posted even while it is a no-op, and
  // nothing past this point may read dispatch state.
  NOTIMPLEMENTED_LOG_ONCE()
      << "Peer verification: accepting every browser for now";

  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(&BrowserRegistry::OnPeerVerified,
                                weak_factory_.GetWeakPtr(), std::move(pending),
                                /*verified=*/true));
}

void BrowserRegistry::OnPeerVerified(PendingAuth pending, bool verified) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  const mojo::ReceiverId receiver_id = pending.receiver_id;

  // The connection may have gone away mid-verification; the pending marker is
  // the only thing that still says whether it is around, and it is cleared by
  // OnHostProviderDisconnected().
  if (!pending_auth_.erase(receiver_id)) {
    VLOG(1) << "Browser " << receiver_id << " went away during verification";
    // The pipe is already closed, so this reply goes nowhere; run it anyway
    // rather than dropping a response callback.
    std::move(pending.reply).Run(mojom::BrowserAuthResult::kRejected);
    return;
  }

  if (!verified) {
    VLOG(1) << "Browser " << receiver_id << " failed verification";
    std::move(pending.reply).Run(mojom::BrowserAuthResult::kRejected);
    return;
  }

  host_sessions_[receiver_id] = std::make_unique<BrowserHostImpl>(
      std::move(pending.browser_endpoint), std::move(pending.host),
      base::BindOnce(&BrowserRegistry::OnHostDisconnected,
                     weak_factory_.GetWeakPtr(), receiver_id));

  VLOG(1) << "Browser " << receiver_id << " authenticated";
  std::move(pending.reply).Run(mojom::BrowserAuthResult::kAccepted);
}

void BrowserRegistry::OnHostProviderDisconnected() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  // Runs while the disconnecting receiver is still current; the server closes
  // it itself once this returns.
  const mojo::ReceiverId receiver_id = host_server_->current_receiver();
  VLOG(1) << "Browser " << receiver_id << " disconnected";

  // Per the mojom contract, losing the connection tears down its BrowserHost
  // too, and abandons any verification still in flight.
  host_sessions_.erase(receiver_id);
  pending_auth_.erase(receiver_id);
}

void BrowserRegistry::OnHostDisconnected(mojo::ReceiverId id) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  VLOG(1) << "Browser " << id << " dropped a session pipe";
  // Destroys the BrowserHostImpl whose pipe is invoking this, which is safe
  // for a mojo disconnect handler. The connection stays up, so the browser may
  // authenticate again.
  host_sessions_.erase(id);
}

}  // namespace brave_vpn::v2
