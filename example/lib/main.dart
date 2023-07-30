import 'package:flutter/material.dart';

import 'package:audio_player_gst/audio_player_gst.dart';

void main() {
  runApp(const MyApp());
}

class MyApp extends StatefulWidget {
  const MyApp({super.key});

  @override
  State<MyApp> createState() => _MyAppState();
}

class _MyAppState extends State<MyApp> {
  double _volume = 1;
  final player = AudioPlayerGst();

  _MyAppState() {
    // player.setUri(Uri.parse('file:///media/Media/Music/Avril Lavigne/Let Go/03. Sk8ter Boi.mp3'));
    // player.setUrl('https://drive.google.com/uc?id=1LfoEzQ55EgQZHzxm-C6Z50eThKmdrHDl&export=download');
    // player.setUrl('https://drive.google.com/uc?id=1mzrftwlkaC5p933HscexLj-i74qx1wdf&export=download');
    // player.setUrl('https://drive.google.com/uc?id=12zb-VuLh18hKlBXL5qd3As8MLDypx2G1&export=download');
    player.setUrl('https://drive.google.com/uc?id=1rOrHdLuhDx7F4kMAbtaxScazCG0kIlL8&export=download');
    // AudioPlayerGst.eventsStream().listen((event)
    // {
    //   debugPrint('Event: $event');
    // });
  }

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      home: Scaffold(
        appBar: AppBar(
          title: const Text('Plugin example app'),
        ),
        body: Row(
          children: [
            TextButton(
                onPressed: (){
                  player.play();
                },
                child: const Text('Play')
            ),
            TextButton(
                onPressed: (){
                  player.pause();
                },
                child: const Text('Pause')
            ),
            Slider(
                value: _volume,
                onChanged: (double value){
                  _volume = value;
                  setState(() {});
                  player.setVolume(value);
                }
            )
          ],
        ),
      ),
    );
  }
}
