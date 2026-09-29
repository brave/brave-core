/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_SPEEDREADER_PAGE_DISTILLER_H_
#define BRAVE_BROWSER_SPEEDREADER_PAGE_DISTILLER_H_

#include <string>

#include "base/functional/callback_forward.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "base/values.h"
#include "brave/components/speedreader/speedreader_util.h"
#include "content/public/browser/weak_document_ptr.h"
#include "url/gurl.h"

namespace content {
class RenderFrameHost;
class WebContents;
}  // namespace content

namespace speedreader {

class PageDistiller {
 public:
  enum class State {
    kUnknown,
    kNotDistillable,
    kDistillable,
    kDistilled,
  };

  class Observer : public base::CheckedObserver {
   public:
    virtual void OnPageDistillStateChanged(State state) {}

   protected:
    ~Observer() override = default;
  };

  // |source_url| is the url of the document |content| was distilled from. The
  // content belongs to that document only and must never be shown as the
  // content of another one.
  using DistillContentCallback = base::OnceCallback<
      void(bool success, const GURL& source_url, std::string content)>;
  using TextToSpeechContentCallback = base::OnceCallback<void(base::Value)>;

  State GetState() const;

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  void GetDistilledHTML(DistillContentCallback callback);
  void GetDistilledText(DistillContentCallback callback);
  void GetTextToSpeak(TextToSpeechContentCallback callback);

 protected:
  explicit PageDistiller(content::WebContents* web_contents);
  virtual ~PageDistiller();

  void SetWebContents(content::WebContents* web_contents);
  void UpdateState(State state);

 private:
  // Returns the document the distillation was started for, or nullptr if it is
  // gone, i.e. a navigation has committed in the meantime. Distillation is
  // asynchronous, its result must never be applied to another document.
  content::RenderFrameHost* GetSourceDocument(
      const content::WeakDocumentPtr& source_document) const;

  void StartDistill(DistillContentCallback callback);
  void OnGetOuterHTML(content::WeakDocumentPtr source_document,
                      DistillContentCallback callback,
                      base::Value result);
  void OnGetTextToSpeak(TextToSpeechContentCallback callback,
                        base::Value result);
  void OnPageDistilled(content::WeakDocumentPtr source_document,
                       GURL source_url,
                       DistillContentCallback callback,
                       DistillationResult result,
                       std::string original_data,
                       std::string transformed);

  void AddStyleSheet(DistillContentCallback callback,
                     bool success,
                     const GURL& source_url,
                     std::string html_content);
  void ExtractText(DistillContentCallback callback,
                   bool success,
                   const GURL& source_url,
                   std::string html_content);

  State state_ = State::kUnknown;
  raw_ptr<content::WebContents> web_contents_ = nullptr;

  base::ObserverList<Observer> observers_;

  base::WeakPtrFactory<PageDistiller> weak_factory_{this};
};

}  // namespace speedreader

#endif  // BRAVE_BROWSER_SPEEDREADER_PAGE_DISTILLER_H_
