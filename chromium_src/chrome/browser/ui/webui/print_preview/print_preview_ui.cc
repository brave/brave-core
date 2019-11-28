#include "chrome/grit/component_extension_resources.h"
#include "brave/browser/resources/extensions/grit/brave_extensions_resources.h"

#undef IDR_PDF_VIEWPORT_JS
#define IDR_PDF_VIEWPORT_JS  IDR_BRAVE_PDF_VIEWPORT_JS
#include "../../../../../../../chrome/browser/ui/webui/print_preview/print_preview_ui.cc"
#undef IDR_PDF_VIEWPORT_JS
