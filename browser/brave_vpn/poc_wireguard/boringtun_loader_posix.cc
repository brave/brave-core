#include "brave/browser/brave_vpn/poc_wireguard/boringtun_loader.h"

#include <dlfcn.h>

#include "base/memory/raw_ptr.h"
#include "base/logging.h"

namespace wireguard {

namespace {

#if BUILDFLAG(IS_MAC)
constexpr char kLibName[] = "libboringtun.dylib";
#elif BUILDFLAG(IS_LINUX)
constexpr char kLibName[] = "libboringtun.so";
#else
#error "Unsupported POSIX platform"
#endif

class PosixBoringtunLoader final : public BoringtunLoader {
 public:
  ~PosixBoringtunLoader() override { Unload(); }

  bool Load(const std::string& path) override;
  void Unload() override;

 private:
  raw_ptr<void> handle_ = nullptr;

  template <typename FnPtr>
  bool Resolve(const char* sym, FnPtr& out);
};

template <typename FnPtr>
bool PosixBoringtunLoader::Resolve(const char* sym, FnPtr& out) {
  // Clear any prior error, then attempt resolution.
  ::dlerror();
  void* p = ::dlsym(handle_, sym);
  const char* err = ::dlerror();
  if (err) {
    LOG(ERROR) << "dlsym(\"" << sym << "\") failed: " << err;
    return false;
  }
  out = reinterpret_cast<FnPtr>(p);
  return true;
}

bool PosixBoringtunLoader::Load(const std::string& path) {
  handle_ = ::dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
  if (!handle_) {
    LOG(ERROR) << "dlopen(\"" << path << "\") failed: " << ::dlerror();
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

void PosixBoringtunLoader::Unload() {
  if (handle_) {
    ::dlclose(handle_);
    handle_ = nullptr;
    loaded_ = false;
  }
}

}  // namespace

std::unique_ptr<BoringtunLoader> CreateBoringtunLoader() {
  return std::make_unique<PosixBoringtunLoader>();
}

const char* BoringtunLibraryName() {
  return kLibName;
}

}  // namespace wireguard
