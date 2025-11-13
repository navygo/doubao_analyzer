#include "RefreshTokenStore.hpp"
#include "ConfigManager.hpp"
#include "DatabaseManager.hpp"
#include "utils.hpp"
#include <openssl/sha.h>
#include <random>
#include <ctime>

static std::string sha256_hex(const std::string &input)
{
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char *>(input.data()), input.size(), hash);
    static const char *hex = "0123456789abcdef";
    std::string out;
    out.reserve(SHA256_DIGEST_LENGTH * 2);
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i)
    {
        unsigned char b = hash[i];
        out.push_back(hex[(b >> 4) & 0xF]);
        out.push_back(hex[b & 0xF]);
    }
    return out;
}

std::string RefreshTokenStore::random_hex(size_t len)
{
    static const char *hex = "0123456789abcdef";
    thread_local std::mt19937_64 gen((std::random_device())());
    std::uniform_int_distribution<> dis(0, 15);
    std::string out;
    out.reserve(len);
    for (size_t i = 0; i < len; ++i)
        out.push_back(hex[dis(gen)]);
    return out;
}

RefreshTokenStore::RefreshTokenStore() {}
RefreshTokenStore::~RefreshTokenStore() {}

std::string RefreshTokenStore::CreateRefreshToken(const std::string &sub, int expiry_seconds)
{
    std::string token = random_hex(64);
    std::string token_hash = sha256_hex(token);

    std::time_t now = std::time(nullptr);
    long created_at = static_cast<long>(now);
    long expires_at = static_cast<long>(now + expiry_seconds);

    ConfigManager cfg;
    cfg.load_config();
    DatabaseManager db(cfg.get_database_config());
    if (!db.initialize())
    {
        // DB unavailable: fallback to a local file store so refresh can be tested
        try
        {
            std::string path = "./refresh_tokens_local.json";
            nlohmann::json arr = nlohmann::json::array();
            if (utils::file_exists(path))
            {
                std::ifstream in(path);
                in >> arr;
            }
            nlohmann::json rec;
            rec["token_hash"] = token_hash;
            rec["user_id"] = sub;
            rec["created_at"] = created_at;
            rec["expires_at"] = expires_at;
            arr.push_back(rec);
            std::ofstream out(path);
            out << arr.dump(2);
        }
        catch (...)
        {
        }
        return token;
    }

    db.create_refresh_token_record(token_hash, sub, created_at, expires_at);
    return token;
}

bool RefreshTokenStore::VerifyRefreshToken(const std::string &token, std::string &out_sub)
{
    std::string token_hash = sha256_hex(token);
    ConfigManager cfg;
    cfg.load_config();
    DatabaseManager db(cfg.get_database_config());
    if (!db.initialize())
    {
        // fallback to local file store
        try
        {
            std::string path = "./refresh_tokens_local.json";
            if (!utils::file_exists(path))
                return false;
            nlohmann::json arr;
            std::ifstream in(path);
            in >> arr;
            std::time_t now = std::time(nullptr);
            for (auto &rec : arr)
            {
                if (rec.contains("token_hash") && rec["token_hash"].get<std::string>() == token_hash)
                {
                    long expires_at = rec.value("expires_at", 0l);
                    if (expires_at < static_cast<long>(now))
                        return false;
                    out_sub = rec.value("user_id", "");
                    return true;
                }
            }
        }
        catch (...)
        {
            return false;
        }
        return false;
    }
    return db.verify_refresh_token_record(token_hash, out_sub);
}

void RefreshTokenStore::RevokeToken(const std::string &token)
{
    std::string token_hash = sha256_hex(token);
    ConfigManager cfg;
    cfg.load_config();
    DatabaseManager db(cfg.get_database_config());
    if (!db.initialize())
    {
        // fallback: remove from local file store
        try
        {
            std::string path = "./refresh_tokens_local.json";
            if (!utils::file_exists(path))
                return;
            nlohmann::json arr;
            std::ifstream in(path);
            in >> arr;
            nlohmann::json newarr = nlohmann::json::array();
            for (auto &rec : arr)
            {
                if (!(rec.contains("token_hash") && rec["token_hash"].get<std::string>() == token_hash))
                {
                    newarr.push_back(rec);
                }
            }
            std::ofstream out(path);
            out << newarr.dump(2);
        }
        catch (...)
        {
        }
        return;
    }
    db.revoke_refresh_token_record(token_hash);
}
