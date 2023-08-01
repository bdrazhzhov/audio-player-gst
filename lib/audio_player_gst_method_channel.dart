import 'package:flutter/foundation.dart';
import 'package:flutter/services.dart';

import 'audio_player_gst_platform_interface.dart';

/// An implementation of [AudioPlayerGstPlatform] that uses method channels.
class MethodChannelAudioPlayerGst extends AudioPlayerGstPlatform {
  /// The method channel used to interact with the native platform.
  @visibleForTesting
  final methodChannel = const MethodChannel('audio_player_gst');

  @override
  Future<void> play() => methodChannel.invokeMethod('play');

  @override
  Future<void> pause()  => methodChannel.invokeMethod('pause');

  @override
  Future<void> setVolume(double volume) => methodChannel.invokeMethod('setVolume', volume);

  @override
  Future<void> setUrl(String url) => methodChannel.invokeMethod('setUrl', url);

  @override
  Future<void> seek(Duration position) => methodChannel.invokeMethod('seek', position.inMilliseconds);

  @override
  Future<void> setRate(double rate) => methodChannel.invokeMethod('setRate', rate);
}
