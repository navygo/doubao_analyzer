#include "Jwt.hpp"
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <ctime>
#include <cstdlib>
#include <vector>
#include <sstream>
#include <iomanip>
#include <nlohmann/json.hpp>

// 小型 base64url 编解码器
static std::string base64_url_encode(const std::string &in)
{
    static const char *b64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    int val = 0, valb = -6;
    for (unsigned char c : in)
    {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0)
        {
            out.push_back(b64[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6)
        out.push_back(b64[((val << 8) >> (valb + 8)) & 0x3F]);
    while (out.size() % 4)
        out.push_back('=');

    // 转为 base64url
    for (auto &ch : out)
    {
        if (ch == '+')
            ch = '-';
        else if (ch == '/')
            ch = '_';
    }
    // 去掉填充
    while (!out.empty() && out.back() == '=')
        out.pop_back();
    return out;
}

static std::string base64_url_decode(const std::string &in)
{
    std::string b64 = in;
    for (auto &ch : b64)
    {
        if (ch == '-')
            ch = '+';
        else if (ch == '_')
            ch = '/';
    }
    // pad
    while (b64.size() % 4)
        b64.push_back('=');

    std::string out;
    std::vector<int> T(256, -1);
    for (int i = 0; i < 64; i++)
        T[(unsigned char)"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"[i]] = i;
    int val = 0, valb = -8;
    for (unsigned char c : b64)
    {
        if (T[c] == -1)
            break;
        val = (val << 6) + T[c];
        valb += 6;
        if (valb >= 0)
        {
            out.push_back(char((val >> valb) & 0xFF));
            valb -= 8;
        }
    }
    return out;
}

static std::string get_secret()
{
    const char *env = std::getenv("JWT_SECRET");
    if (env && env[0] != '\0')
        return std::string(env);
    // 如果没有设置环境变量，使用一个弱默认值（仅用于开发）
    return std::string("dev-secret-change-me");
}

namespace jwt
{

    std::string GenerateToken(const std::string &sub, int expiry_seconds)
    {
        nlohmann::json header = {{"alg", "HS256"}, {"typ", "JWT"}};
        std::time_t now = std::time(nullptr);
        nlohmann::json payload = {
            {"sub", sub},
            {"iat", now},
            {"exp", now + expiry_seconds}};

        std::string header_enc = base64_url_encode(header.dump());
        std::string payload_enc = base64_url_encode(payload.dump());
        std::string signing_input = header_enc + "." + payload_enc;

        std::string secret = get_secret();

        unsigned int len = EVP_MAX_MD_SIZE;
        unsigned char digest[EVP_MAX_MD_SIZE];
        HMAC(EVP_sha256(), secret.data(), (int)secret.size(),
             (unsigned char *)signing_input.data(), signing_input.size(), digest, &len);

        std::string sig(reinterpret_cast<char *>(digest), len);
        std::string sig_enc = base64_url_encode(sig);

        return signing_input + "." + sig_enc;
    }

    bool VerifyToken(const std::string &token, nlohmann::json &out_claims)
    {
        try
        {
            size_t p1 = token.find('.');
            size_t p2 = token.find('.', p1 + 1);
            if (p1 == std::string::npos || p2 == std::string::npos)
                return false;
            std::string header_enc = token.substr(0, p1);
            std::string payload_enc = token.substr(p1 + 1, p2 - p1 - 1);
            std::string signature_enc = token.substr(p2 + 1);

            std::string signing_input = header_enc + "." + payload_enc;

            std::string secret = get_secret();

            unsigned int len = EVP_MAX_MD_SIZE;
            unsigned char digest[EVP_MAX_MD_SIZE];
            HMAC(EVP_sha256(), secret.data(), (int)secret.size(),
                 (unsigned char *)signing_input.data(), signing_input.size(), digest, &len);

            std::string sig(reinterpret_cast<char *>(digest), len);
            std::string expected_sig_enc = base64_url_encode(sig);

            if (expected_sig_enc != signature_enc)
                return false;

            std::string payload_json = base64_url_decode(payload_enc);
            out_claims = nlohmann::json::parse(payload_json);

            // 检查 exp
            if (out_claims.contains("exp"))
            {
                std::time_t now = std::time(nullptr);
                long exp = out_claims["exp"].get<long>();
                if (now > exp)
                    return false;
            }

            return true;
        }
        catch (...)
        {
            return false;
        }
    }

} // namespace jwt
