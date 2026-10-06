# Class `blackbox::BlackboxException` & Error Codes

Defined in header `<blackbox/exception.hpp>`  
Namespace: `blackbox`

`blackbox-essential` handles control-plane failures and operational errors using strongly-typed exceptions derived from `std::exception`.

---

## 1. Class Synopsis

```cpp
namespace blackbox {

enum class ErrorCode : int32_t {
    SUCCESS = 0,
    ERR_XDP_ATTACH_FAILED = -1,
    ERR_XDP_DETACH_FAILED = -2,
    ERR_BPF_OBJECT_NOT_FOUND = -3,
    ERR_BPF_MAP_NOT_FOUND = -4,
    ERR_BPF_MAP_UPDATE_FAILED = -5,
    ERR_BPF_MAP_LOOKUP_FAILED = -6,
    ERR_INTERFACE_NOT_FOUND = -7,
    ERR_TPM_INITIALIZATION_FAILED = -8,
    ERR_TPM_QUOTE_FAILED = -9,
    ERR_MODEL_CONFIG_INVALID = -10,
    ERR_RING_BUFFER_FULL = -11,
    ERR_PERMISSION_DENIED = -12,
    ERR_INTERNAL_FAULT = -99
};

class BLACKBOX_API BlackboxException : public std::exception {
public:
    explicit BlackboxException(ErrorCode code, std::string message, int system_code = 0) noexcept;
    ~BlackboxException() override = default;

    [[nodiscard]] const char* what() const noexcept override;
    [[nodiscard]] ErrorCode error_code() const noexcept;
    [[nodiscard]] int system_code() const noexcept;

private:
    ErrorCode code_;
    std::string message_;
    int system_code_{0};
};

} // namespace blackbox
```

---

## 2. Idiomatic Exception Handling

```cpp
#include <blackbox/blackbox.hpp>
#include <iostream>

int main() {
    try {
        blackbox::XdpConfig config{
            .interface_name = "eth0",
            .bpf_object_path = "/usr/local/lib/bpf/xdp_filter.o"
        };

        blackbox::XdpManager xdp(config);
        xdp.attach();

    } catch (const blackbox::BlackboxException& ex) {
        std::cerr << "[Blackbox Fatal Error]\n"
                  << "  Code       : " << static_cast<int>(ex.error_code()) << "\n"
                  << "  Description: " << ex.what() << "\n"
                  << "  System Err : " << ex.system_code() << " (" 
                  << std::strerror(ex.system_code()) << ")\n";
        return 1;
    }

    return 0;
}
```

