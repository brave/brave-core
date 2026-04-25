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

namespace content {
class WebContents;
}

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

  using DistillContentCallback =
      base::OnceCallback<void(bool success, std::string content)>;
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
  void StartDistill(DistillContentCallback callback);
  void OnGetOuterHTML(DistillContentCallback callback, base::Value result);
  void OnGetTextToSpeak(TextToSpeechContentCallback callback,
                        base::Value result);
  void OnPageDistilled(DistillContentCallback callback,
                       DistillationResult result,
                       std::string original_data,
                       std::string transformed);

  void AddStyleSheet(DistillContentCallback callback,
                     bool success,
                     std::string html_content);
  void ExtractText(DistillContentCallback callback,
                   bool success,
                   std::string html_content);

  State state_ = State::kUnknown;
  raw_ptr<content::WebContents> web_contents_ = nullptr;

  base::ObserverList<Observer> observers_;

  base::WeakPtrFactory<PageDistiller> weak_factory_{this};
};

}  // namespace speedreader

#endif  // BRAVE_BROWSER_SPEEDREADER_PAGE_DISTILLER_H_
