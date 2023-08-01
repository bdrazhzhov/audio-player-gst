
import 'package:flutter/services.dart';

import 'events.dart';
import 'audio_player_gst_platform_interface.dart';

class AudioPlayerGst {
  static const _playingStates = {
    "GST_STATE_VOID_PENDING": PlayingState.pending,
    "GST_STATE_NULL": PlayingState.idle,
    "GST_STATE_READY": PlayingState.ready,
    "GST_STATE_PAUSED": PlayingState.paused,
    "GST_STATE_PLAYING": PlayingState.playing
  };

  static const _eventChannel = EventChannel('audio_player_gst/events');
  /// Stream containing events of the player state changes
  ///
  /// List of event types:
  /// - [PlayingStateEvent]
  /// - [DurationEvent] - currently playing audio length is in [duration] field of the event
  /// - [PositionEvent] - currently playing audio position is in [position] field of the event
  /// - [BufferingEvent] - occurs when audio is being downloaded from network until it's fully finished.
  /// [percent] contains amount of downloaded data
  static Stream<EventBase> eventsStream() {
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
          return PlayingCompletedEvent();
      }

      return UnknownEvent();
    }).distinct();
  }

  /// Starts playing
  Future<void> play() => AudioPlayerGstPlatform.instance.play();
  /// Pauses currently playing audio
  Future<void> pause() => AudioPlayerGstPlatform.instance.pause();
  /// Changes volume of audio. Allowed interval: [[0, 1.0]]
  Future<void> setVolume(double volume) => AudioPlayerGstPlatform.instance.setVolume(volume);
  /// Sets audio url
  Future<void> setUrl(String url) => AudioPlayerGstPlatform.instance.setUrl(url);
  /// Sets the position currently playing audio
  Future<void> seek(Duration position) => AudioPlayerGstPlatform.instance.seek(position);
  /// Changes playing audio speed.
  ///  - Default: 1.0
  ///  - Double speed: 2.0
  ///  - Half speed: 0.5
  ///  - Positive values for playing forward. The negative ones for playing backward
  Future<void> setRate(double rate) => AudioPlayerGstPlatform.instance.setRate(rate);
}
