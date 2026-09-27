#pragma once

#include <string>

namespace txplay::library {

// Plain-value output struct. All fields are UTF-8 std::string.
// Empty string means the tag was absent or could not be read.
// TagLib types do not appear here; they are confined to MetadataReader.cpp.
struct MetadataResult {
    std::string title;
    std::string artist;
    std::string album;
};

class MetadataReader {
public:
    // Attempt to read embedded audio metadata from the file at canonical_path.
    // Returns a MetadataResult whose fields are empty strings when a tag is
    // absent, the file is unsupported, or any error occurs.
    // Never throws; never crashes the caller.
    static MetadataResult read(const std::string& canonical_path);
};

} // namespace txplay::library
