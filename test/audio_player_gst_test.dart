import 'package:flutter_test/flutter_test.dart';
import 'package:audio_player_gst/audio_player_gst.dart';
import 'package:audio_player_gst/audio_player_gst_platform_interface.dart';
import 'package:audio_player_gst/audio_player_gst_method_channel.dart';
import 'package:plugin_platform_interface/plugin_platform_interface.dart';

// class MockAudioPlayerGstPlatform
//     with MockPlatformInterfaceMixin
//     implements AudioPlayerGstPlatform {
//
//   @override
//   Future<String?> getPlatformVersion() => Future.value('42');
// }

void main() {
  final AudioPlayerGstPlatform initialPlatform = AudioPlayerGstPlatform.instance;

  test('$MethodChannelAudioPlayerGst is the default instance', () {
    expect(initialPlatform, isInstanceOf<MethodChannelAudioPlayerGst>());
  });

  // test('getPlatformVersion', () async {
  //   AudioPlayerGst audioPlayerGstPlugin = AudioPlayerGst();
  //   MockAudioPlayerGstPlatform fakePlatform = MockAudioPlayerGstPlatform();
  //   AudioPlayerGstPlatform.instance = fakePlatform;
  //
  //   expect(await audioPlayerGstPlugin.getPlatformVersion(), '42');
  // });
}
