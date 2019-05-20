/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_EXTENSIONS_BRAVE_COMPONENT_EXTENSION_H_
#define BRAVE_BROWSER_EXTENSIONS_BRAVE_COMPONENT_EXTENSION_H_

#include <string>

#include "base/files/file_path.h"

class BraveComponentExtension {
 public:
  BraveComponentExtension();
  virtual ~BraveComponentExtension();
  void Register(const std::string& component_name,
                const std::string& component_id,
                const std::string& component_base64_public_key);
  bool Unregister();

 protected:
  virtual void OnComponentRegistered(const std::string& component_id);
  virtual void OnComponentReady(const std::string& component_id,
                                const base::FilePath& install_dir,
                                const std::string& manifest);

 private:
  std::string component_name_;
  std::string component_id_;
  std::string component_base64_public_key_;
};

#endif  // BRAVE_BROWSER_EXTENSIONS_BRAVE_COMPONENT_EXTENSION_H_
