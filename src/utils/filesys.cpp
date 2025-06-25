//[INCLUDES]
#include "../../inc/utils/filesys.h"

#include <iostream>
#include <filesystem>
#include <sys/stat.h>
#include <dirent.h>

//[NAMESPACE]
namespace FileSys 
{
    //[FILE FUNCTIONS]
    bool FileExists(const std::string& path)
    {
        std::ifstream file(path);
        return file.good();
    }
    size_t GetFileSize(const std::string& path)
    {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file.is_open())
        {
            throw std::runtime_error("File does not exist or cannot be opened: " + path);
        }
        return file.tellg(); // Get the size of the file
        
    }

    std::string ReadString(const std::string& path)
    {
        std::ifstream file(path);
        if (!file.is_open())
        {
            throw std::runtime_error("File does not exist or cannot be opened: " + path);
        }
        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        file.close();
        return content;
    }
    std::vector<char> ReadBinary(const std::string& path)
    {
        std::ifstream file(path, std::ios::binary);
        if (!file.is_open())
        {
            throw std::runtime_error("File does not exist or cannot be opened: " + path);
        }
        std::vector<char> data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        file.close();
        return data;
    }
    std::vector<char> ReadBinaryPartial(const std::string& path, size_t offset, size_t length)
    {
        std::ifstream file(path, std::ios::binary);
        if (!file.is_open())
        {
            throw std::runtime_error("File does not exist or cannot be opened: " + path);
        }
        file.seekg(offset);
        std::vector<char> data(length);
        file.read(data.data(), length);
        file.close();
        return data;
    }

    void WriteString(const std::string& path, const std::string& content)
    {
        std::ofstream file(path);
        if (!file.is_open())
        {
            throw std::runtime_error("File cannot be opened for writing: " + path);
        }
        file << content;
        file.close();
    }
    void WriteBinary(const std::string& path, const std::vector<char>& data)
    {
        std::ofstream file(path, std::ios::binary);
        if (!file.is_open())
        {
            throw std::runtime_error("File cannot be opened for writing: " + path);
        }
        file.write(data.data(), data.size());
        file.close();
    }

    void DeleteFile(const std::string& path)
    {
        if (std::remove(path.c_str()) != 0)
        {
            throw std::runtime_error("Error deleting file: " + path);
        }
    }
    void RenameFile(const std::string& oldPath, const std::string& newPath)
    {
        if (std::rename(oldPath.c_str(), newPath.c_str()) != 0)
        {
            throw std::runtime_error("Error renaming file from " + oldPath + " to " + newPath);
        }

    }
    void CopyFile(const std::string& sourcePath, const std::string& destinationPath)
    {
        std::ifstream source(sourcePath, std::ios::binary);
        if (!source.is_open())
        {
            throw std::runtime_error("Source file does not exist or cannot be opened: " + sourcePath);
        }
        std::ofstream destination(destinationPath, std::ios::binary);
        if (!destination.is_open())
        {
            throw std::runtime_error("Destination file cannot be opened for writing: " + destinationPath);
        }
        destination << source.rdbuf();
        source.close();
        destination.close();
    }

    //[DIRECTORY FUNCTIONS]
    bool DirectoryExists(const std::string& path)
    {
    struct stat info;
        if (stat(path.c_str(), &info) != 0)
        {
            return false; // Directory does not exist
        }
        return (info.st_mode & S_IFDIR) != 0; // Check if it's a directory
    }
    std::vector<std::string> ListFiles(const std::string& path, bool recursive)
    {
        std::vector<std::string> files;

        namespace fs = std::filesystem;

        if (recursive) {
            for (auto& p : fs::recursive_directory_iterator(path)) {
                if (fs::is_regular_file(p)) {
                    files.push_back(p.path().string());
                }
            }
        }
        else {
            for (auto& p : fs::directory_iterator(path)) {
                if (fs::is_regular_file(p)) {
                    files.push_back(p.path().string());
                }
            }
        }

        return files;
    }
    void CreateDirectory(const std::string& path)
    {
        if (!std::filesystem::create_directory(path)) {
            throw std::runtime_error("Error creating directory: " + path);
        }
    }
    void DeleteDirectory(const std::string& path, bool recursive)
    {
        if (recursive)
        {
            std::filesystem::remove_all(path);
        }
        else 
        {
            if (!std::filesystem::is_empty(path)) 
            {
                throw std::runtime_error("Directory is not empty: " + path);
            }
            std::filesystem::remove(path);
        }
    }
    void RenameDirectory(const std::string& oldPath, const std::string& newPath)
    {
        std::filesystem::rename(oldPath, newPath);
    }
}