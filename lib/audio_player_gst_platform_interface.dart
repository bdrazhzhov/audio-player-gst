import 'package:plugin_platform_interface/plugin_platform_interface.dart';

import 'audio_player_gst_method_channel.dart';

abstract class AudioPlayerGstPlatform extends PlatformInterface {
  /// Constructs a AudioPlayerGstPlatform.
  AudioPlayerGstPlatform() : super(token: _token);

  static final Object _token = Object();

  static AudioPlayerGstPlatform _instance = MethodChannelAudioPlayerGst();

  /// The default instance of [AudioPlayerGstPlatform] to use.
  ///
  /// Defaults to [MethodChannelAudioPlayerGst].
  static AudioPlayerGstPlatform get instance => _instance;

  /// Platform-specific implementations should set this with their own
  /// platform-specific class that extends [AudioPlayerGstPlatform] when
  /// they register themselves.
  static set instance(AudioPlayerGstPlatform instance) {
    PlatformInterface.verifyToken(instance, _token);
    _instance = instance;
  }

  Future<String?> getPlatformVersion() {
    throw UnimplementedError('platformVersion() has not been implemented.');
  }

  Future<void> play() {
    throw UnimplementedError('play() has not been implemented.');
  }

  Future<void> pause() {
    throw UnimplementedError('pause() has not been implemented.');
  }

  Future<void> setVolume(double value) {
    throw UnimplementedError('setVolume() has not been implemented.');
  }

  Future<void> setUrl(String url) {
    throw UnimplementedError('setUri() has not been implemented.');
  }

  Future<void> seek(Duration position) {
    throw UnimplementedError('seek() has not been implemented.');
  }

  Future<void> setRate(double rate) {
    throw UnimplementedError('setRate() has not been implemented.');
  }
}
