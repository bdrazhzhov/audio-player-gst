
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

  Future<void> play() => AudioPlayerGstPlatform.instance.play();
  Future<void> pause() => AudioPlayerGstPlatform.instance.pause();
  Future<void> setVolume(double volume) => AudioPlayerGstPlatform.instance.setVolume(volume);
  Future<void> setUrl(String url) => AudioPlayerGstPlatform.instance.setUrl(url);
  Future<void> seek(Duration position) => AudioPlayerGstPlatform.instance.seek(position);
  Future<void> setRate(double rate) => AudioPlayerGstPlatform.instance.setRate(rate);
}
