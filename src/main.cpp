#include <windows.h>
#include <string>
#include <filesystem>
#include <curl/curl.h>
#include <sstream>

namespace fs = std::filesystem;

std::string ERROR_PREFIX = "Error:";
std::string TEST_ERROR_FLAG = "--test-error";
std::string CATBOX_API_URL = "https://catbox.moe/user/api.php";
std::string USER_AGENT = "Mozilla/5.0";
std::string CA_BUNDLE_FILE = "curl-ca-bundle.crt";
std::string FORM_FIELD_REQTYPE = "reqtype";
std::string FORM_FIELD_FILE = "fileToUpload";
std::string FORM_VALUE_UPLOAD = "fileupload";

void CopyToClipboard(const std::string& text) {
    if (OpenClipboard(nullptr)) {
        EmptyClipboard();
        HGLOBAL hGlob = GlobalAlloc(GMEM_MOVEABLE, text.size() + 1);
        if (hGlob) {
            memcpy(GlobalLock(hGlob), text.data(), text.size());
            GlobalUnlock(hGlob);
            SetClipboardData(CF_TEXT, hGlob);
        }
        CloseClipboard();
    }
}

size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    auto* response = static_cast<std::string*>(userp);
    response->append(static_cast<char*>(contents), size * nmemb);
    return size * nmemb;
}

bool UploadToCatbox(const fs::path& filepath, std::string& result, bool testError = false) {
    if (testError) {
        result = "test error";
        return false;
    }

    char exePath[MAX_PATH];
    GetModuleFileNameA(NULL, exePath, MAX_PATH);
    fs::path exeDir = fs::path(exePath).parent_path();
    fs::path caBundlePath = exeDir / CA_BUNDLE_FILE;

    CURL* curl = curl_easy_init();
    if (!curl) {
        result = "Failed to initialize CURL";
        return false;
    }

    curl_mime* mime = curl_mime_init(curl);
    
    curl_mimepart* reqtype_part = curl_mime_addpart(mime);
    curl_mime_name(reqtype_part, FORM_FIELD_REQTYPE.c_str());
    curl_mime_data(reqtype_part, FORM_VALUE_UPLOAD.c_str(), CURL_ZERO_TERMINATED);

    curl_mimepart* file_part = curl_mime_addpart(mime);
    curl_mime_name(file_part, FORM_FIELD_FILE.c_str());
    curl_mime_filedata(file_part, filepath.string().c_str());

    curl_easy_setopt(curl, CURLOPT_URL, CATBOX_API_URL.c_str());
    curl_easy_setopt(curl, CURLOPT_MIMEPOST, mime);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &result);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, USER_AGENT.c_str());
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
    curl_easy_setopt(curl, CURLOPT_CAINFO, caBundlePath.string().c_str());

    CURLcode res = curl_easy_perform(curl);
    bool success = (res == CURLE_OK);

    if (!success) {
        result = std::string(curl_easy_strerror(res));
    }

    curl_mime_free(mime);
    curl_easy_cleanup(curl);
    return success;
}

int main(int argc, char* argv[]) {
    if (argc > 1) {
        fs::path filepath(argv[1]);

        bool testError = (argc > 2 && std::string(argv[2]) == TEST_ERROR_FLAG);

        if (!fs::exists(filepath)) {
            MessageBoxA(nullptr, (std::string(ERROR_PREFIX) + " File does not exist: " + filepath.string()).c_str(), "Error", MB_OK | MB_ICONERROR);
            return 1;
        }

        std::string result;
        bool success = UploadToCatbox(filepath, result, testError);

        if (!success) {
            MessageBoxA(nullptr, (std::string(ERROR_PREFIX) + " " + result).c_str(), "Error", MB_OK | MB_ICONERROR);
            return 1;
        } else {
            CopyToClipboard(result);
            std::ostringstream msg;
            msg << "Upload complete!\nURL copied to clipboard:\n" << result;
            MessageBoxA(nullptr, msg.str().c_str(), "Catbox Upload", MB_OK | MB_ICONINFORMATION);
            return 0;
        }
    } else {
        MessageBoxA(nullptr, "No file specified.", "Error", MB_OK | MB_ICONERROR);
        return 1;
    }
}
