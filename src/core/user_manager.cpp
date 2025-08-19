//[INCLUDES]
#include "../../inc/core/user_manager.h"

#include <cstring>

#include "../../inc/utils/compression.h"
#include "../../inc/utils/string_utils.h"
#include "../../inc/utils/filesys.h"
#include "../../inc/log.h"


//[NAMESPACE]
namespace UserManager
{
    #pragma region USER CLASS
    // Constructors
    User::User(const std::string& _user, const std::string& _nick, const std::string& _pass)
    {
        username = _user;
        nickname = _nick;
        password = _pass;

        role = UserRole::Tester; // valor por defecto
        creation_date = std::chrono::system_clock::now();
        last_login = creation_date; // por defecto al momento de creación
        memset(cache, 0, sizeof(cache)); // limpiar el buffer de cache
        ips.clear(); // inicializar vector vacío

        Save();
    }
    User::User(const std::string& _user)
    {
        username = _user;

        // PATH
        std::string _path = "data/users/" + _user + ".usr";

        // READ FILE
        auto _compressed = FileSys::ReadBinary(_path);
        if (_compressed.empty())
        {
            Log::Error("Failed to read user file: %s", _path.c_str());
            return;
        }

        // DECOMPRESS
        auto _data = Compression::Decompress(std::vector<unsigned char>(_compressed.begin(), _compressed.end()));

        // INTERPRET
        size_t _seek = 0;

        // NICKNAME
        nickname = std::string(reinterpret_cast<char*>(_data.data() + _seek));
        _seek += nickname.length() + 1;

        // PASSWORD
        password = std::string(reinterpret_cast<char*>(_data.data() + _seek));
        _seek += password.length() + 1;

        // CREATION DATE
        int64_t _creation = 0;
        std::memcpy(&_creation, _data.data() + _seek, sizeof(_creation));
        creation_date = std::chrono::system_clock::time_point(std::chrono::seconds(_creation));
        _seek += sizeof(_creation);

        // LAST LOGIN
        int64_t _last = 0;
        std::memcpy(&_last, _data.data() + _seek, sizeof(_last));
        last_login = std::chrono::system_clock::time_point(std::chrono::seconds(_last));
        _seek += sizeof(_last);

        // ROLE
        std::memcpy(&role, _data.data() + _seek, 1);
        _seek += 1;

        // CACHE
        std::memcpy(cache, _data.data() + _seek, sizeof(cache));
        _seek += sizeof(cache);

        // IP COUNT
        uint32_t _ip_count = 0;
        std::memcpy(&_ip_count, _data.data() + _seek, sizeof(_ip_count));
        _seek += sizeof(_ip_count);

        ips.clear();
        for (uint32_t i = 0; i < _ip_count; i++)
        {
            char ip_buf[40] = {};
            std::memcpy(ip_buf, _data.data() + _seek, sizeof(ip_buf));
            ips.push_back(std::string(ip_buf));
            _seek += sizeof(ip_buf);
        }

        Log::Message("User %s loaded successfully.", username.c_str());
    }
    User::~User()
    {
        Save();
    }

    // Save/load
    void User::Save()
    {
        //LOCK
        std::unique_lock lock(mutex);

        Log::Message("Saving user %s...", username.c_str());

        //CALCULATE BUFFER SIZE
        size_t _size = 0;
        _size += nickname.length()+1;
        _size += password.length()+1;

        _size += sizeof(int64_t) * 2; // creation_date + last_login
        _size += 1; //Role
        _size += 1024; //Cache

        _size += 4; //Ip count
        _size += ips.size() * 40;

        //CREATE BUFFER
        char _buffer[_size];
        std::memset(_buffer, 0, _size);

        //SERIALIZA
        size_t _seek = 0;
        std::memcpy(_buffer + _seek, nickname.data(), nickname.size());  _seek += nickname.size()+1; //WRITE NAME
        std::memcpy(_buffer + _seek, password.data(), password.size());  _seek += password.size()+1; //WRITE PASSWORD
     
        int64_t _creation = std::chrono::duration_cast<std::chrono::seconds>(creation_date.time_since_epoch()).count(); //WRITE CREATION TIME
        std::memcpy(_buffer + _seek, &_creation, sizeof(_creation)); _seek += sizeof(_creation);

        int64_t _last = std::chrono::duration_cast<std::chrono::seconds>(last_login.time_since_epoch()).count(); //WRITE LAST LOGIN TIME
        std::memcpy(_buffer + _seek, &_last, sizeof(_last)); _seek += sizeof(_last);
        
        std::memcpy(_buffer + _seek, &role, 1); _seek += 1; //WRITE ROLE
        
        std::memcpy(_buffer + _seek, cache, sizeof(cache)); //WRITE CACHE
        _seek += sizeof(cache);

        uint32_t _ip_count = static_cast<uint32_t>(ips.size()); //WRITE IP'S
        std::memcpy(_buffer + _seek, &_ip_count, sizeof(_ip_count));
        _seek += sizeof(_ip_count);
        for (const auto& _ip : ips)
        {
            // Rellenar hasta 40 bytes incluyendo '\0'
            char ip_buf[40] = {};
            std::strncpy(ip_buf, _ip.c_str(), sizeof(ip_buf) - 1);
            std::memcpy(_buffer + _seek, ip_buf, sizeof(ip_buf));
            _seek += sizeof(ip_buf);
        }

        //COMPRESS
        auto _compress = Compression::Compress(std::vector<unsigned char>(_buffer, _buffer + _size));

        //SAVE
        std::string _path = "data/users/" + username + ".usr";
        FileSys::WriteBinary(_path,
    std::vector<char>(_compress.begin(), _compress.end()));

        Log::Message("User saved.");
    }

    // Password
    bool User::CheckPassword(const std::string& _pass)
    {
        std::shared_lock lock(mutex);
        return password == _pass;
    }
    void User::SetPassword(const std::string& _pass)
    {
        std::unique_lock lock(mutex);
        password = _pass;
    }
    std::string User::GetPassword() const
    {
        std::shared_lock lock(mutex);
        return password;
    }

    // Nickname
    void User::SetNickname(const std::string& _nick)
    {
        std::unique_lock lock(mutex);
        nickname = _nick;
    }
    std::string User::GetNickname() const
    {
        std::shared_lock lock(mutex);
        return nickname;
    }

    // Role
    void User::SetRole(UserRole _role)
    {
        std::unique_lock lock(mutex);
        role = _role;
    }
    UserRole User::GetRole() const
    {
        std::shared_lock lock(mutex);
        return role;
    }

    // Timestamps
    std::chrono::system_clock::time_point User::GetLastLogin() const
    {
        std::shared_lock lock(mutex);
        return last_login;
    }
    void User::UpdateLastLogin()
    {
        std::unique_lock lock(mutex);
        last_login = std::chrono::system_clock::now();
    }
    std::chrono::system_clock::time_point User::GetCreationDate() const
    {
        std::shared_lock lock(mutex);
        return creation_date;
    }

    // IPs
    size_t User::GetIpsCount() const
    {
        std::shared_lock lock(mutex);
        return ips.size();
    }
    bool User::AddIp(const std::string& _ip)
    {
        std::unique_lock lock(mutex);
        for (const auto& i : ips)
            if (i == _ip) return false; // ya existe
        ips.push_back(_ip);
        return true;
    }

    std::string User::GetIp(size_t _index) const
    {
        std::shared_lock lock(mutex);
        if (_index >= ips.size()) return {};
        return ips[_index];
    }
    
    // Cache
    void User::WriteCache(size_t offset, const void* data, size_t size)
    {
        std::unique_lock lock(mutex);
        if (offset + size > sizeof(cache)) return; // fuera de rango
        std::memcpy(cache + offset, data, size);
    }
    void User::ReadCache(size_t offset, void* out_data, size_t size) const
    {
        std::shared_lock lock(mutex);
        if (offset + size > sizeof(cache)) return; // fuera de rango
        std::memcpy(out_data, cache + offset, size);
    }

    // Username getter
    std::string User::GetUsername() const
    {
        std::shared_lock lock(mutex);
        return username;
    }

    #pragma endregion




    //[VARIABLES]
    std::mutex users_mutex;
    std::unordered_map<std::string, std::weak_ptr<User>> users;

    std::mutex logins_mutex;
    std::unordered_map<int, login_t> logins;

    //[FUNCTIONS]
    void Init()
    {
        Log::Message("Loading users...");

        //CREATE PATHS
        if(!FileSys::DirectoryExists("data"))
            FileSys::CreateDirectory("data");

        if(!FileSys::DirectoryExists("data/users"))
            FileSys::CreateDirectory("data/users");


        //LOAD USERS
        auto _files = FileSys::ListFiles("data/users", false);
        for(uint32_t _i = 0;_i < _files.size();_i++)
        {
            if(!StringUtils::EndsWith(_files[_i], ".usr"))
                continue;

            std::string _key = StringUtils::Replace(_files[_i], "data/users/", "");
            _key = StringUtils::Replace(_key, ".usr", "");

            //Add 2 dictionary
            users.emplace(_key, std::weak_ptr<User>());
        }
    }
    
    std::shared_ptr<User> CreateUser(const std::string& _usr, const std::string& _nick, const std::string& _password)
    {
        // LOCK (scope guard)
        std::lock_guard<std::mutex> lock(users_mutex);

        //Check if the name is free
        if(users.find(_usr) != users.end())
        {
            Log::Error("User %s already exists!", _usr.c_str());
            return nullptr; // Devolver puntero nulo
        }


        //Create new user
        auto _user = std::make_shared<User>(_usr, _nick, _password);

        //Add into the dicctionary and return
        users.emplace(_usr, _user);
        return _user;
    }
    std::shared_ptr<User> GetUser(const std::string& _usr)
    {
        // LOCK (scope guard)
        std::lock_guard<std::mutex> lock(users_mutex);

        // Check if the user exists in the map
        auto _entry = users.find(_usr);
        if (_entry == users.end())
        {
            Log::Error("User %s does not exist!", _usr.c_str());
            return nullptr;
        }

        //If isn't expired, return
        if (!_entry->second.expired())
        {
            return _entry->second.lock();
        }

        // Load the user
        auto _user = std::make_shared<User>(_usr);
        _entry->second = _user; // weak_ptr se asigna desde shared_ptr

        return _user;
    }



    int8_t Login(int _socket, const std::string& _usr, const std::string& _password, const std::string& _ip)
    {
        // LOCK (scope guard)
        std::lock_guard<std::mutex> lock(logins_mutex);

        //Check socket
        if(logins.find(_socket) != logins.end())
        {
            Log::Warning("Socket %d already has an active session.", _socket);
            return -1; // Ya existe sesión para este socket
        }

        //Try to get the user
        auto _user_ptr = GetUser(_usr);
        if (!_user_ptr) // nullptr o usuario no cargado
        {
            return -2; // Usuario no existe
        }

        //Compare password
        if(!_user_ptr->CheckPassword(_password))
        {
            return -3;
        }

        // Passed all checks, create login session
        login_t _session;
        _session.user = _user_ptr;
        _session.ip = _ip;

        logins[_socket] = _session;

        return 0; // login exitoso
    }
    bool Logout(int _sid)
    {
        // LOCK (scope guard)
        std::lock_guard<std::mutex> lock(logins_mutex);

        auto it = logins.find(_sid);
        if (it == logins.end())
        {
            // No hay sesión para este socket/ID
            return false;
        }

        // Eliminar la sesión
        logins.erase(it);
        return true;
    }

    login_t* GetSession(int _sid)
    {
        // LOCK (scope guard)
        std::lock_guard<std::mutex> lock(logins_mutex);

        auto it = logins.find(_sid);
        if (it == logins.end())
        {
            return nullptr; // no existe la sesión
        }

        // Devolver puntero al login_t dentro del map
        return &it->second;
    }
}
