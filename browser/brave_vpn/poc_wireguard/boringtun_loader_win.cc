#include "brave/browser/brave_vpn/poc_wireguard/boringtun_loader.h"

#include <windows.h>

#include "base/logging.h"

namespace wireguard {

namespace {

constexpr char kLibName[] = "boringtun.dll";

class WinBoringtunLoader final : public BoringtunLoader {
 public:
  ~WinBoringtunLoader() override { Unload(); }

  bool Load(const std::string& path) override;
  void Unload() override;

 private:
  HMODULE handle_ = nullptr;

  template <typename FnPtr>
  bool Resolve(const char* sym, FnPtr& out);
};

template <typename FnPtr>
bool WinBoringtunLoader::Resolve(const char* sym, FnPtr& out) {
  FARPROC p = ::GetProcAddress(handle_, sym);
  if (!p) {
    LOG(ERROR) << "GetProcAddress(\"" << sym
               << "\") failed: " << ::GetLastError();
    return false;
  }
  // FARPROC is a pointer-to-function; reinterpret_cast is the correct
  // and standard way to convert it to a typed function pointer on Windows.
  out = reinterpret_cast<FnPtr>(p);
  return true;
}

bool WinBoringtunLoader::Load(const std::string& path) {
  // LoadLibraryExA with LOAD_WITH_ALTERED_SEARCH_PATH makes the DLL's own
  // directory the first search location for its dependencies, which matches
  // the RTLD_LOCAL | RTLD_NOW semantics of the POSIX loader.
  handle_ = ::LoadLibraryExA(path.c_str(), /*hFile=*/nullptr,
                             LOAD_WITH_ALTERED_SEARCH_PATH);
  if (!handle_) {
    LOG(ERROR) << "LoadLibraryExA(\"" << path
               << "\") failed: " << ::GetLastError();
    return false;
  }

  bool ok = true;
  ok &= Resolve("set_logging_function", set_logging_function);
  ok &= Resolve("new_tunnel", new_tunnel);
  ok &= Resolve("tunnel_free", tunnel_free);
  ok &= Resolve("wireguard_write", wireguard_write);
  ok &= Resolve("wireguard_read", wireguard_read);
  ok &= Resolve("wireguard_tick", wireguard_tick);
  ok &= Resolve("wireguard_force_handshake", wireguard_force_handshake);
  ok &= Resolve("wireguard_stats", wireguard_stats);

  if (!ok) {
    Unload();
    return false;
  }

  loaded_ = true;
  LOG(INFO) << "Loaded " << path;
  return true;
}

void WinBoringtunLoader::Unload() {
  if (handle_) {
    ::FreeLibrary(handle_);
    handle_ = nullptr;
    loaded_ = false;
  }
}

}  // namespace

std::unique_ptr<BoringtunLoader> CreateBoringtunLoader() {
  return std::make_unique<WinBoringtunLoader>();
}

const char* BoringtunLibraryName() {
  return kLibName;
}

}  // namespace wireguard
