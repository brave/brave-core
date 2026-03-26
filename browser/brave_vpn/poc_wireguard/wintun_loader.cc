#include "brave/browser/brave_vpn/poc_wireguard/wintun_loader.h"

#include "base/logging.h"

namespace wireguard {

namespace {
constexpr char kLibName[] = "wintun.dll";
}  // namespace

WintunLoader::WintunLoader() = default;
WintunLoader::~WintunLoader() {
  Unload();
}

template <typename FnPtr>
bool WintunLoader::Resolve(const char* sym, FnPtr& out) {
  FARPROC p = ::GetProcAddress(handle_, sym);
  if (!p) {
    LOG(ERROR) << "GetProcAddress(\"" << sym
               << "\") failed: " << ::GetLastError();
    return false;
  }
  out = reinterpret_cast<FnPtr>(p);
  return true;
}

bool WintunLoader::Load(const std::string& path) {
  // LOAD_WITH_ALTERED_SEARCH_PATH makes wintun.dll's own directory the first
  // search location for its dependencies - same reasoning as boringtun_loader.
  handle_ = ::LoadLibraryExA(path.c_str(), /*hFile=*/nullptr,
                             LOAD_WITH_ALTERED_SEARCH_PATH);
  if (!handle_) {
    LOG(ERROR) << "LoadLibraryExA(\"" << path
               << "\") failed: " << ::GetLastError();
    return false;
  }

  bool ok = true;
  ok &= Resolve("WintunCreateAdapter", WintunCreateAdapter);
  ok &= Resolve("WintunOpenAdapter", WintunOpenAdapter);
  ok &= Resolve("WintunCloseAdapter", WintunCloseAdapter);
  ok &= Resolve("WintunGetAdapterLUID", WintunGetAdapterLUID);
  ok &= Resolve("WintunGetRunningDriverVersion", WintunGetRunningDriverVersion);
  ok &= Resolve("WintunSetLogger", WintunSetLogger);
  ok &= Resolve("WintunStartSession", WintunStartSession);
  ok &= Resolve("WintunEndSession", WintunEndSession);
  ok &= Resolve("WintunGetReadWaitEvent", WintunGetReadWaitEvent);
  ok &= Resolve("WintunReceivePacket", WintunReceivePacket);
  ok &= Resolve("WintunReleaseReceivePacket", WintunReleaseReceivePacket);
  ok &= Resolve("WintunAllocateSendPacket", WintunAllocateSendPacket);
  ok &= Resolve("WintunSendPacket", WintunSendPacket);

  if (!ok) {
    Unload();
    return false;
  }

  loaded_ = true;
  LOG(INFO) << "Loaded " << path;
  return true;
}

void WintunLoader::Unload() {
  if (handle_) {
    ::FreeLibrary(handle_);
    handle_ = nullptr;
    loaded_ = false;
  }
}

std::unique_ptr<WintunLoader> CreateWintunLoader() {
  return std::make_unique<WintunLoader>();
}

const char* WintunLibraryName() {
  return kLibName;
}

}  // namespace wireguard
