/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SPEEDREADER_SPEEDREADER_URL_LOADER_H_
#define BRAVE_COMPONENTS_SPEEDREADER_SPEEDREADER_URL_LOADER_H_

#include <tuple>

#include "base/memory/raw_ptr.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/task/single_thread_task_runner.h"
#include "brave/components/body_sniffer/body_sniffer_url_loader.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "services/network/public/mojom/url_loader.mojom.h"
#include "url/gurl.h"

namespace body_sniffer {
class BodySnifferThrottle;
}  // namespace body_sniffer

namespace speedreader {

class SpeedreaderResultDelegate;
class SpeedreaderRewriterService;
class SpeedReaderThrottle;

// Loads the whole response body and tries to Speedreader-distill it.
// Cargoculted from |`SniffingURLLoader|.
// Note that common functionality between this class and DeAmp has
// been moved to component/sniffer
//
// This loader has five states:
// kWaitForBody: The initial state until the body is received (=
//               OnStartLoadingResponseBody() is called) or the response is
//               finished (= OnComplete() is called). When body is provided, the
//               state is changed to kLoading. Otherwise the state goes to
//               kCompleted.
// kLoading: Receives the body from the source loader and distills the page.
//            The received body is kept in this loader until distilling
//            is finished. When all body has been received and distilling is
//            done, this loader will dispatch queued messages like
//            OnStartLoadingResponseBody() to the destination
//            loader client, and then the state is changed to kSending.
// kSending: Receives the body and sends it to the destination loader client.
//           The state changes to kCompleted after all data is sent.
// kCompleted: All data has been sent to the destination loader.
// kAborted: Unexpected behavior happens. Watchers, pipes and the binding from
//           the source loader to |this| are stopped. All incoming messages from
//           the destination (through network::mojom::URLLoader) are ignored in
class SpeedReaderURLLoader : public body_sniffer::BodySnifferURLLoader {
 public:
  ~SpeedReaderURLLoader() override;

  // mojo::PendingRemote<network::mojom::URLLoader> controls the lifetime of the
  // loader.
  static std::tuple<mojo::PendingRemote<network::mojom::URLLoader>,
                    mojo::PendingReceiver<network::mojom::URLLoaderClient>,
                    SpeedReaderURLLoader*>
  CreateLoader(base::WeakPtr<body_sniffer::BodySnifferThrottle> throttle,
               base::WeakPtr<SpeedreaderResultDelegate> delegate,
               const GURL& response_url,
               scoped_refptr<base::SingleThreadTaskRunner> task_runner,
               SpeedreaderRewriterService* rewriter_service);

 private:
  SpeedReaderURLLoader(
      base::WeakPtr<body_sniffer::BodySnifferThrottle> throttle,
      base::WeakPtr<SpeedreaderResultDelegate> delegate,
      const GURL& response_url,
      mojo::PendingRemote<network::mojom::URLLoaderClient>
          destination_url_loader_client,
      scoped_refptr<base::SingleThreadTaskRunner> task_runner,
      SpeedreaderRewriterService* rewriter_service);

  void OnBodyReadable(MojoResult) override;
  void OnBodyWritable(MojoResult) override;
  void MaybeLaunchSpeedreader();

  void OnCompleteSending() override;
  base::WeakPtr<SpeedreaderResultDelegate> delegate_;

  // Not Owned
  raw_ptr<SpeedreaderRewriterService> rewriter_service_ = nullptr;
};

}  // namespace speedreader

#endif  // BRAVE_COMPONENTS_SPEEDREADER_SPEEDREADER_URL_LOADER_H_
