# Copyright (c) 2022 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import override_utils

@override_utils.override_function(globals())
def GetConfigurationForBuild(original_function, defines):
  base = original_function(defines)
  merged = _merge_dicts(_BRAVE_VALUES, base)

  # Remove Google.Policies namespace. Microsoft.Policies.Windows remains, as it is hardcoded in admx_writer.py.
  merged.pop('admx_using_namespaces', None)

  return merged

_BRAVE_VALUES = {
  'build': 'brave',
  'app_name': 'Brave',
  'doc_url':
    'https://support.brave.com/hc/en-us/articles/360039248271-Group-Policy',
  'frame_name': 'Brave Frame',
  'webview_name': 'Brave WebView',
  'win_config': {
    'win': {
      'reg_mandatory_key_name': 'Software\\Policies\\BraveSoftware\\Brave',
      'reg_recommended_key_name':
        'Software\\Policies\\BraveSoftware\\Brave\\Recommended',
      'mandatory_category_path': ['Cat_Brave', 'brave'],
      'recommended_category_path': ['Cat_Brave', 'brave_recommended'],
      'category_path_strings': {
        'Cat_Brave': 'Brave Software',
        'brave': 'Brave',
        'brave_recommended':
        'Brave - {doc_recommended}'
      },
      'namespace': 'BraveSoftware.Policies.Brave',
    },
  },
  'admx_prefix': 'brave',
  'linux_policy_path': '/etc/brave/policies/',
  'bundle_id': 'com.brave.ios.core',
}

def _merge_dicts(src, dst):
  result = dict(dst)
  for k, v in src.items():
    result[k] = _merge_dicts(v, dst.get(k, {})) if isinstance(v, dict) else v
  return result
