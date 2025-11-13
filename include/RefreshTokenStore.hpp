#pragma once

#include <string>
#include <nlohmann/json.hpp>

class RefreshTokenStore
{
public:
    RefreshTokenStore();
    ~RefreshTokenStore();

    // 创建并持久化 refresh token，返回明文 token 字符串（会在 DB 中保存其 hash）
    std::string CreateRefreshToken(const std::string &sub, int expiry_seconds);

    // 验证 token，有效则返回 true 并填充 out_sub
    bool VerifyRefreshToken(const std::string &token, std::string &out_sub);

    // 撤销 token
    void RevokeToken(const std::string &token);

private:
    // no file-backed storage in DB-backed implementation
    std::string random_hex(size_t len);
};
