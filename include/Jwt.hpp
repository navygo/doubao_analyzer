#pragma once

#include <string>
#include <nlohmann/json.hpp>

namespace jwt
{
    // 生成一个基于 HMAC-SHA256 的 JWT，sub 为主体（如 username），expiry_seconds 为有效期（秒）
    std::string GenerateToken(const std::string &sub, int expiry_seconds);

    // 验证 token 的签名和过期时间，验证通过后将 payload 填充到 out_claims，并返回 true
    bool VerifyToken(const std::string &token, nlohmann::json &out_claims);
}
