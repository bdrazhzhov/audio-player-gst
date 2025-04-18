
import 'package:flutter/services.dart';

import 'events.dart';

class AudioPlayerGst {
  static const _playingStates = {
    "GST_STATE_VOID_PENDING": PlayingState.pending,
    "GST_STATE_NULL": PlayingState.idle,
    "GST_STATE_READY": PlayingState.ready,
    "GST_STATE_PAUSED": PlayingState.paused,
    "GST_STATE_PLAYING": PlayingState.playing
  };

  final _eventChannel = const EventChannel('audio_player_gst/events');
  final _methodChannel = const MethodChannel('audio_player_gst');

  /// Stream containing events of the player state changes
  ///
  /// List of event types:
  /// - [PlayingStateEvent] contains player state. Available states:
  ///   - [pending]
  ///   - [idle] - initial state
  ///   - [ready] - player is ready to be paused
  ///   - [paused] - player is paused and not playing now
  ///   - [playing] - player is playing specified media
  /// - [DurationEvent] currently playing audio length is in [duration] field of the event
  /// - [PositionEvent] currently playing audio position is in [position] field of the event
  /// - [BufferingEvent] occurs when audio is being downloaded from network until it's fully finished.
  /// [percent] contains amount of downloaded data
  /// - [PlayingCompletedEvent] occurs when the end of file is reached
  /// - [VolumeEvent] currently playing audio volume is in [value] field of the event
  Stream<EventBase> eventsStream() {
    return _eventChannel.receiveBroadcastStream().map((dynamic event) {
      final map = event as Map<Object?, Object?>;
      switch(map['event']) {
        case 'audio.playingState':
          final value = map['value'] as String;
          return PlayingStateEvent(_playingStates[value] ?? PlayingState.unknown);
        case 'audio.duration':
          final value = map['value'] as int;
          return DurationEvent(Duration(milliseconds: value));
        case 'audio.position':
          final value = map['value'] as int;
          return PositionEvent(Duration(microseconds: value ~/ 1000));
        case 'audio.buffering':
          final value = map['value'] as double;
          return BufferingEvent(value);
        case 'audio.completed':
          return PlayingStateEvent(PlayingState.completed);
        case 'audio.volume':
          final value = map['value'] as double;
          return VolumeEvent(value);
      }

      return UnknownEvent();
    }).distinct();
  }

  /// Starts playing
  Future<void> play() => _methodChannel.invokeMethod('play');
  /// Pauses currently playing audio
  Future<void> pause() => _methodChannel.invokeMethod('pause');
  /// Changes volume of audio. Allowed interval: [[0, 1.0]]
  Future<void> setVolume(double volume) => _methodChannel.invokeMethod('setVolume', volume);
  /// Sets audio url
  Future<void> setUrl(String url, [String? encryptionKey]) =>
      _methodChannel.invokeMethod('setUrl', {'url': url, 'encryptionKey': encryptionKey});
  /// Sets the position currently playing audio
  Future<void> seek(Duration position) => _methodChannel.invokeMethod('seek', position.inMilliseconds);
  /// Changes playing audio speed.
  ///  - Default: 1.0
  ///  - Double speed: 2.0
  ///  - Half speed: 0.5
  ///  - Positive values for playing forward. The negative ones for playing backward
  Future<void> setRate(double rate) => _methodChannel.invokeMethod('setRate', rate);
}
