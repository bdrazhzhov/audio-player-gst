# audio_player_gst

A Flutter plugin allowing play audio (local or from web) on linux using gstreamer libs.

## Dependencies

libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev gstreamer1.0-plugins-good gstreamer1.0-plugins-bad

## Quick start

```dart
import 'package:audio_player_gst/audio_player_gst.dart';

final player = AudioPlayerGst();                // Create player
await player.setUrl(                            // Load a URL
  'https://example.com/song.mp3');              // Schemes: (https: | file:)
await player.play();                            // Start playing
await player.pause();                           // Pause currently playing media
await player.seek(Duration(second: 10));        // Jump to the 10 second position
await player.setRate(2.0);                      // Double playing speed
await player.setVolume(0.5);                    // Half playing speed
```

## Working with events stream

```dart
AudioPlayerGst.eventsStream().listen((EventBase event) {
  switch(event.runtimeType) {
    case DurationEvent: ...
    case PlayingStateEvent: ...
    case PositionEvent: ...
    case BufferingEvent: ...
    case PlayingCompletedEvent: ...
  }
});
```
There is no ability to create multiple GStreamer player instances for now.
