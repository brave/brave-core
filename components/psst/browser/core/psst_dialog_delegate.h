/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PSST_BROWSER_CORE_PSST_DIALOG_DELEGATE_H_
#define BRAVE_COMPONENTS_PSST_BROWSER_CORE_PSST_DIALOG_DELEGATE_H_

#include "base/functional/callback_forward.h"
#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "content/public/browser/web_contents.h"

namespace psst {

class COMPONENT_EXPORT(PSST_BROWSER_CORE) PsstDialogDelegate {
 public:
  class Observer : public base::CheckedObserver {
   public:
    virtual void OnSetRequestDone(const std::string& url,
                                  const std::optional<std::string>& error) = 0;
    virtual void OnSetCompleted(const std::vector<std::string>& applied_checks,
                                const std::vector<std::string>& errors) = 0;
  };

  void AddObserver(Observer* obs);
  void RemoveObserver(Observer* obs);
  bool HasObserver(Observer* observer);

  using ConsentCallback =
      base::OnceCallback<void(const std::vector<std::string>& disabled_checks)>;
  using ShareCallback = base::OnceCallback<void()>;

  struct ShowDialogData {
    ShowDialogData(const bool is_new_version,
                   const std::string& site_name,
                   base::Value::List request_infos,
                   ConsentCallback apply_changes_callback,
                   ConsentCallback cancel_callback,
                   base::OnceClosure never_ask_me_callback);
    ~ShowDialogData();

    bool is_new_version;
    std::string site_name;
    base::Value::List request_infos;
    ConsentCallback apply_changes_callback;
    ConsentCallback cancel_callback;
    base::OnceClosure never_ask_me_callback;
  };

  PsstDialogDelegate();
  virtual ~PsstDialogDelegate();
  virtual void ShowPsstConsentDialog(
      content::WebContents* contents,
      std::unique_ptr<ShowDialogData> show_dialog_data);
  virtual void SetProgressValue(content::WebContents* contents,
                                const double value) = 0;
  virtual void SetRequestDone(content::WebContents* contents,
                              const std::string& url,
                              const std::optional<std::string>& error);
  virtual void SetCompletedView(content::WebContents* contents,
                                const std::vector<std::string>& applied_checks,
                                const std::vector<std::string>& errors,
                                ShareCallback share_cb);
  virtual void Close(content::WebContents* contents) = 0;

  ShowDialogData* GetShowDialogData();

 private:
  std::unique_ptr<ShowDialogData> show_dialog_data_;
  base::ObserverList<Observer> observer_list_;
};

}  // namespace psst

#endif  //  BRAVE_COMPONENTS_PSST_BROWSER_CORE_PSST_DIALOG_DELEGATE_H_
