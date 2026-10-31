/**
 ******************************************************************************
 * ReXGlue                                                                    *
 ******************************************************************************
 *
 * @author  Tom Clay, 2026
 */

#ifndef REX_NET_CURL_SESSION_H_
#define REX_NET_CURL_SESSION_H_

#include <rex/platform.h>

// WinHTTP is the HTTPS backend on Windows. Everywhere else libcurl fills the
// same role, so the clients that talk to Thunderstore, Game Jolt and the
// netplay web API keep one session object and hand it whole requests.
#if !REX_PLATFORM_WIN32 && defined(REX_HAVE_CURL)

#include <mutex>
#include <string>
#include <vector>

#include <curl/curl.h>

namespace rexgamejolt {

namespace detail {
// curl_easy_init() will initialise the library implicitly, but that path is
// not thread safe and sessions are created on worker threads. Do it once, up
// front, from whichever thread gets here first.
inline void EnsureCurlGlobalInit() {
  static std::once_flag once;
  std::call_once(once, [] { curl_global_init(CURL_GLOBAL_DEFAULT); });
}
}  // namespace detail

// One easy handle per session. curl keeps the connection - and its TLS
// handshake - alive across requests issued on the same handle, which is what
// the WinHTTP sessions get from their HINTERNET.
class CurlSession {
 public:
  CurlSession() {
    detail::EnsureCurlGlobalInit();
    handle_ = curl_easy_init();
  }

  ~CurlSession() {
    if (handle_) {
      curl_easy_cleanup(handle_);
    }
  }

  CurlSession(const CurlSession&) = delete;
  CurlSession& operator=(const CurlSession&) = delete;

  bool valid() const { return handle_ != nullptr; }

  // Returns true only when the transfer completed and the status was 2xx, so
  // callers can treat it exactly like the WinHTTP sessions it stands in for.
  // `extra_headers` is a list of complete "Name: value" lines.
  bool Request(const std::string& url, const std::string& method,
               const std::string& request_body, std::string& out_response, int timeout_ms,
               const std::vector<std::string>& extra_headers = {}) {
    if (!handle_) {
      return false;
    }

    // Reset rather than re-init so the connection pool on this handle
    // survives into the next request.
    curl_easy_reset(handle_);

    curl_slist* headers = nullptr;
    for (const std::string& header : extra_headers) {
      headers = curl_slist_append(headers, header.c_str());
    }

    curl_easy_setopt(handle_, CURLOPT_URL, url.c_str());
    curl_easy_setopt(handle_, CURLOPT_USERAGENT, "ReXGlue/1.0");
    curl_easy_setopt(handle_, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(handle_, CURLOPT_MAXREDIRS, 8L);
    curl_easy_setopt(handle_, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(handle_, CURLOPT_TIMEOUT_MS, static_cast<long>(timeout_ms));
    curl_easy_setopt(handle_, CURLOPT_CONNECTTIMEOUT_MS, static_cast<long>(timeout_ms));
    curl_easy_setopt(handle_, CURLOPT_WRITEFUNCTION, &CurlSession::WriteCallback);
    curl_easy_setopt(handle_, CURLOPT_WRITEDATA, &out_response);

    if (method == "GET") {
      curl_easy_setopt(handle_, CURLOPT_HTTPGET, 1L);
    } else {
      curl_easy_setopt(handle_, CURLOPT_CUSTOMREQUEST, method.c_str());
    }
    if (!request_body.empty()) {
      curl_easy_setopt(handle_, CURLOPT_POSTFIELDS, request_body.c_str());
      curl_easy_setopt(handle_, CURLOPT_POSTFIELDSIZE,
                       static_cast<long>(request_body.size()));
    }
    if (headers) {
      curl_easy_setopt(handle_, CURLOPT_HTTPHEADER, headers);
    }

    const CURLcode result = curl_easy_perform(handle_);
    long status = 0;
    if (result == CURLE_OK) {
      curl_easy_getinfo(handle_, CURLINFO_RESPONSE_CODE, &status);
    }

    if (headers) {
      curl_slist_free_all(headers);
    }
    // The handle still points at out_response; clear it so a later reset or
    // teardown can never write through a dangling pointer.
    curl_easy_setopt(handle_, CURLOPT_WRITEDATA, nullptr);
    curl_easy_setopt(handle_, CURLOPT_HTTPHEADER, nullptr);

    return result == CURLE_OK && status >= 200 && status < 300;
  }

  bool Get(const std::string& url, std::string& out, int timeout_ms) {
    return Request(url, "GET", {}, out, timeout_ms);
  }

  // The transport error text for the last failed Request, for logging.
  static const char* ErrorText(CURLcode code) { return curl_easy_strerror(code); }

 private:
  static size_t WriteCallback(char* ptr, size_t size, size_t nmemb, void* userdata) {
    const size_t bytes = size * nmemb;
    auto* out = static_cast<std::string*>(userdata);
    if (!out) {
      return 0;
    }
    out->append(ptr, bytes);
    return bytes;
  }

  CURL* handle_ = nullptr;
};

}  // namespace rexgamejolt

#endif  // !REX_PLATFORM_WIN32 && REX_HAVE_CURL

#endif  // REX_NET_CURL_SESSION_H_
