#ifndef _FILESYS_H
#define _FILESYS_H
//[INCLUDES]
#include <string>
#include <fstream>
#include <vector>

//[NAMESPACE]
namespace FileSys 
{
    //[FILE FUNCTIONS]
    bool FileExists(const std::string& path);
    size_t GetFileSize(const std::string& path);

    std::string ReadString(const std::string& path);
    std::vector<char> ReadBinary(const std::string& path);
    std::vector<char> ReadBinaryPartial(const std::string& path, size_t offset, size_t length);

    void WriteString(const std::string& path, const std::string& content);
    void WriteBinary(const std::string& path, const std::vector<char>& data);

    void DeleteFile(const std::string& path);
    void RenameFile(const std::string& oldPath, const std::string& newPath);
    void CopyFile(const std::string& sourcePath, const std::string& destinationPath);


    //[DIRECTORY FUNCTIONS]
    bool DirectoryExists(const std::string& path);
    std::vector<std::string> ListFiles(const std::string& path, bool recursive);
    void CreateDirectory(const std::string& path);
    void DeleteDirectory(const std::string& path, bool recursive);
    void RenameDirectory(const std::string& oldPath, const std::string& newPath);

}
#endif