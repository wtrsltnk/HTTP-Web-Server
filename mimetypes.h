#ifndef MIMETYPES_H
#define MIMETYPES_H

#include <map>
#include <string>

class MimeTypes
{
    std::map<std::string,std::string> _mimeTypes;
    std::string _defaultMimeType;
public:
    MimeTypes();

    std::string const & operator [] (std::string const &extension) const;
    bool LoadFromFile(std::string const &filename);
    bool SaveToFile(std::string const &filename);
    void LoadDefaultMimeTypes();
    void Add(std::string const &extension, std::string const &mimetype);
    void Remove(std::string const &extension);

    std::map<std::string,std::string> const &Data() const;
};

#endif // MIMETYPES_H
