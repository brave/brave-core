import writer_configuration

def GetConfigurationForBuild(defines):
    base = writer_configuration.GetConfigurationForBuild({'_chromium'})
    return _merge_dicts(_BRAVE_VALUES, base)

_BRAVE_VALUES = {
    'build': 'brave',
    'app_name': 'Brave',
    'doc_url':
        'https://support.brave.com/hc/en-us/articles/360039248271-Group-Policy',
    'frame_name': 'Brave Frame',
    'webview_name': 'Brave WebView',
    'win_config': {
        'win': {
            'reg_mandatory_key_name':
                'Software\\Policies\\BraveSoftware\\Brave',
            'reg_recommended_key_name':
                'Software\\Policies\\BraveSoftware\\Brave\\Recommended',
            'mandatory_category_path': ['brave'],
            'recommended_category_path': ['brave_recommended'],
            'category_path_strings': {
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
    return {
        k: _merge_dicts(src.get(k, {}), v) if isinstance(v, dict) else
           src.get(k, v)
        for k, v in dst.items()
    }
