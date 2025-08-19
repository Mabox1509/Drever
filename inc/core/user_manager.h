#ifndef _USER_MANAGER_H
#define _USER_MANAGER_H
//[INCLUDES]
#include <unordered_map>
#include <shared_mutex>
#include <mutex>
#include <chrono>
#include <vector>
#include <string>
#include <memory>


//[NAMESPACE]
namespace UserManager
{
    //[TYPES]
    enum class UserRole : uint8_t
    {
        Admin,
        Developer,
        Tester,
    };
    class User
    {
    private:
        std::string username;
        std::string nickname;
        std::string password;
        std::chrono::system_clock::time_point creation_date;
        std::chrono::system_clock::time_point last_login;
        UserRole role;
        char cache[1024];
        std::vector<std::string> ips;

        mutable std::shared_mutex mutex;

    public:
        // Constructors
        User(const std::string& _user, const std::string& _nick, const std::string& _pass); // CREATE
        User(const std::string& _user);                                                     // LOAD
        ~User();

        // Save/load
        void Save();

        // Password
        bool CheckPassword(const std::string& _pass);
        void SetPassword(const std::string& _pass);
        std::string GetPassword() const;

        // Nickname
        void SetNickname(const std::string& _nick);
        std::string GetNickname() const;

        // Role
        void SetRole(UserRole _role);
        UserRole GetRole() const;

        // Timestamps
        std::chrono::system_clock::time_point GetLastLogin() const;
        void UpdateLastLogin();
        std::chrono::system_clock::time_point GetCreationDate() const;

        // IPs
        size_t GetIpsCount() const;
        bool AddIp(const std::string& _ip);
        std::string GetIp(size_t _index) const;

        // Cache
        void WriteCache(size_t offset, const void* data, size_t size);
        void ReadCache(size_t offset, void* out_data, size_t size) const;

        // Username getter
        std::string GetUsername() const;
    };

    typedef struct login_t
    {
        std::shared_ptr<User> user;
        std::string ip;

    } login_t;


    //[VARIABLES]
    extern std::mutex users_mutex;
    extern std::unordered_map<std::string, std::weak_ptr<User>> users;

    extern std::mutex logins_mutex;
    extern std::unordered_map<int, login_t> logins;

    //[FUNCTIONS]
    void Init();

    std::shared_ptr<User>  CreateUser(const std::string& _usr, const std::string& _nick, const std::string& _password);
    std::shared_ptr<User>  GetUser(const std::string& _usr);

    int8_t Login(int _socket, const std::string& _usr, const std::string& _password, const std::string& _ip);
    bool Logout (int _sid);
    login_t* GetSession(int _sid);


}
#endif //!_USER_MANAGER_H