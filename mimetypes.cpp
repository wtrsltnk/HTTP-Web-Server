#include "mimetypes.h"
#include <fstream>

MimeTypes::MimeTypes()
    : _defaultMimeType("text/plain")
{ }

std::string const &MimeTypes::operator [] (std::string const &extension) const
{
    auto found = _mimeTypes.find(extension);

    if (_mimeTypes.find(extension) == _mimeTypes.end())
    {
        return _defaultMimeType;
    }

    return found->second;
}

bool MimeTypes::LoadFromFile(std::string const &filename)
{
    std::ifstream infile(filename);

    if (!infile.is_open())
    {
        return false;
    }

    for(std::string line; getline(infile, line); )
    {
        if (line[0] != '.')
        {
            continue;
        }

        auto found = line.find_first_of(';');
        if (found == std::string::npos)
        {
            continue;
        }

        _mimeTypes.insert(std::make_pair(line.substr(0, found), line.substr(found + 1)));
    }

    return true;
}

bool MimeTypes::SaveToFile(std::string const &filename)
{
    std::ofstream outfile(filename);

    if (!outfile.is_open())
    {
        return false;
    }

    for (auto pair : _mimeTypes)
    {
        outfile << pair.first << ";" << pair.second << "\n";
    }

    outfile.close();

    return true;
}

void MimeTypes::Add(std::string const &extension, std::string const &mimetype)
{
    if (_mimeTypes.find(extension) == _mimeTypes.end())
    {
        _mimeTypes.insert(std::make_pair(extension, mimetype));
    }
}

void MimeTypes::Remove(std::string const &extension)
{
    _mimeTypes.erase(extension);
}

std::map<std::string,std::string> const &MimeTypes::Data() const
{
    return _mimeTypes;
}

void MimeTypes::LoadDefaultMimeTypes()
{
    _mimeTypes = {
        { ".aac", "audio/aac" },
        { ".abw", "application/x-abiword" },
        { ".arc", "application/octet-stream" },
        { ".avi", "video/x-msvideo" },
        { ".azw", "application/vnd.amazon.ebook" },
        { ".bin", "application/octet-stream" },
        { ".bz", "application/x-bzip" },
        { ".bz2", "application/x-bzip2" },
        { ".csh", "application/x-csh" },
        { ".css", "text/css" },
        { ".csv", "text/csv" },
        { ".doc", "application/msword" },
        { ".docx", "application/vnd.openxmlformats-officedocument.wordprocessingml.document" },
        { ".eot", "application/vnd.ms-fontobject" },
        { ".epub", "application/epub+zip" },
        { ".gif", "image/gif" },
        { ".htm", "text/html" },
        { ".html", "text/html" },
        { ".ico", "image/x-icon" },
        { ".ics", "text/calendar" },
        { ".jar", "application/java-archive" },
        { ".jpeg", "image/jpeg" },
        { ".jpg", "image/jpeg" },
        { ".js", "application/javascript" },
        { ".json", "application/json" },
        { ".mid", "audio/midi" },
        { ".midi", "audio/midi" },
        { ".mpeg", "video/mpeg" },
        { ".mpkg", "application/vnd.apple.installer+xml" },
        { ".odp", "application/vnd.oasis.opendocument.presentation" },
        { ".ods", "application/vnd.oasis.opendocument.spreadsheet" },
        { ".odt", "application/vnd.oasis.opendocument.text" },
        { ".oga", "audio/ogg" },
        { ".ogv", "video/ogg" },
        { ".ogx", "application/ogg" },
        { ".otf", "font/otf" },
        { ".png", "image/png" },
        { ".pdf", "application/pdf" },
        { ".ppt", "application/vnd.ms-powerpoint" },
        { ".pptx", "application/vnd.openxmlformats-officedocument.presentationml.presentation" },
        { ".rar", "application/x-rar-compressed" },
        { ".rtf", "application/rtf" },
        { ".sh", "application/x-sh" },
        { ".svg", "image/svg+xml" },
        { ".swf", "application/x-shockwave-flash" },
        { ".tar", "application/x-tar" },
        { ".tif", "image/tiff" },
        { ".tiff", "image/tiff" },
        { ".ts", "application/typescript" },
        { ".ttf", "font/ttf" },
        { ".vsd", "application/vnd.visio" },
        { ".wav", "audio/x-wav" },
        { ".weba", "audio/webm" },
        { ".webm", "video/webm" },
        { ".webp", "image/webp" },
        { ".woff", "font/woff" },
        { ".woff2", "font/woff2" },
        { ".xhtml", "application/xhtml+xml" },
        { ".xls", "application/vnd.ms-excel" },
        { ".xlsx", "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet" },
        { ".xml", "application/xml" },
        { ".xul", "application/vnd.mozilla.xul+xml" },
        { ".zip", "application/zip" },
        { ".3gp", "video/3gpp" },
        { ".3g2", "video/3gpp2" },
        { ".7z", "application/x-7z-compressed" },
    };
}
