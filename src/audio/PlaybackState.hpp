#pragma once

// PlaybackState is a shared playback-domain type consumed by AudioEngine,
// Application, and the UI layer. It lives here so that Application and the UI
// can include just this header rather than pulling in all of AudioEngine.hpp.

namespace txplay::audio {

enum class PlaybackState {
    Stopped,
    Playing,
    Paused
};

} // namespace txplay::audio
