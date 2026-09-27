#include "MetadataReader.hpp"

// TagLib headers are confined to this translation unit only.
// No TagLib type is allowed to appear in MetadataReader.hpp or any other
// Txplay header/source file.
#include <fileref.h>
#include <tag.h>

namespace txplay::library {

MetadataResult MetadataReader::read(const std::string& canonical_path) {
    MetadataResult result; // all fields empty by default

    // Open the file for tag reading only (readAudioProperties=false skips the
    // expensive bitrate/sample-rate scan; we only need tag strings here).
    TagLib::FileRef f(canonical_path.c_str(), /*readAudioProperties=*/false);

    if (f.isNull()) {
        // File is unsupported, unreadable, or corrupt — return empty result.
        return result;
    }

    TagLib::Tag* tag = f.tag();
    if (!tag) {
        // Valid file but no tag block at all — return empty result.
        return result;
    }

    // to8Bit(true) converts TagLib's internal string (which handles UTF-16,
    // Latin-1, and UTF-8 source encodings) into a UTF-8 std::string.
    // An empty TagLib::String converts to an empty std::string.
    result.title  = tag->title().to8Bit(/*unicode=*/true);
    result.artist = tag->artist().to8Bit(/*unicode=*/true);
    result.album  = tag->album().to8Bit(/*unicode=*/true);

    return result;
}

} // namespace txplay::library
