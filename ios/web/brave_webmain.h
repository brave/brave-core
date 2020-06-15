#include <memory>

namespace web {
struct WebMainParams;
class WebMainRunner;

class BraveWebMain {
public:
    BraveWebMain(WebMainParams params);
    virtual ~BraveWebMain();
public:
    std::unique_ptr<WebMainRunner> web_main_runner_;
};
}
