import 'package:flutter/services.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:audio_player_gst/audio_player_gst_method_channel.dart';

void main() {
  TestWidgetsFlutterBinding.ensureInitialized();

  MethodChannelAudioPlayerGst platform = MethodChannelAudioPlayerGst();
  const MethodChannel channel = MethodChannel('audio_player_gst');

  setUp(() {
    TestDefaultBinaryMessengerBinding.instance.defaultBinaryMessenger.setMockMethodCallHandler(
      channel,
      (MethodCall methodCall) async {
        return '42';
      },
    );
  });

  tearDown(() {
    TestDefaultBinaryMessengerBinding.instance.defaultBinaryMessenger.setMockMethodCallHandler(channel, null);
  });
}
