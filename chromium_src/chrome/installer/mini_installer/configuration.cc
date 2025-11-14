#ifdef UNSAFE_BUFFERS_BUILD
// TODO(crbug.com/40285824): Remove this and convert code to safer constructs.
#pragma allow_unsafe_buffers
#endif

#include "chrome/installer/mini_installer/configuration.h"

#include "build/branding_buildflags.h"
#include "chrome/installer/mini_installer/appid.h"

#define Initialize() Initialize(HMODULE module)

#define kAppGuid kAppGuid; previous_version_ = nullptr

#if defined(OFFICIAL_BUILD) && !BUILDFLAG(GOOGLE_CHROME_BRANDING)
#define BUILDFLAG_INTERNAL_GOOGLE_CHROME_BRANDING() (1)
#define NEED_TO_UNDEF_BUILDFLAG_INTERNAL_GOOGLE_CHROME_BRANDING
#endif

#include <chrome/installer/mini_installer/configuration.cc>

#if defined(NEED_TO_UNDEF_BUILDFLAG_INTERNAL_GOOGLE_CHROME_BRANDING)
#undef BUILDFLAG_INTERNAL_GOOGLE_CHROME_BRANDING
#undef NEED_TO_UNDEF_BUILDFLAG_INTERNAL_GOOGLE_CHROME_BRANDING
#endif

#undef kAppGuid
#undef Initialize

namespace mini_installer {

void Configuration::ReadResources(HMODULE module) {
  HRSRC resource_info_block =
      FindResource(module, MAKEINTRESOURCE(ID_PREVIOUS_VERSION), RT_RCDATA);
  if (!resource_info_block)
    return;

  HGLOBAL data_handle = LoadResource(module, resource_info_block);
  if (!data_handle)
    return;

  // The data is a Unicode string, so it must be a multiple of two bytes.
  DWORD version_size = SizeofResource(module, resource_info_block);
  if (!version_size || (version_size & 0x01) != 0)
    return;

  void* version_data = LockResource(data_handle);
  if (!version_data)
    return;

  const wchar_t* version_string = reinterpret_cast<wchar_t*>(version_data);
  size_t version_len = version_size / sizeof(wchar_t);

  // The string must be terminated.
  if (version_string[version_len - 1])
    return;

  previous_version_ = version_string;
}

}  // namespace mini_installer
