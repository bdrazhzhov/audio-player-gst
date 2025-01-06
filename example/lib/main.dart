import 'package:audio_player_gst/events.dart';
import 'package:audio_video_progress_bar/audio_video_progress_bar.dart';
import 'package:flutter/material.dart';

import 'package:audio_player_gst/audio_player_gst.dart';

void main() {
  runApp(const MyApp());
}

class MyApp extends StatelessWidget {
  const MyApp({super.key});

  @override
  Widget build(BuildContext context) {
    return const MaterialApp(
      home: HomePage(),
    );
  }
}

class HomePage extends StatefulWidget {
  const HomePage({super.key});

  @override
  State<HomePage> createState() => _HomePageState();
}

class _HomePageState extends State<HomePage> {
  bool _playing = false;
  double _volume = 0.05;
  var _duration = Duration.zero;
  var _position = Duration.zero;
  var _buffered = Duration.zero;
  final player = AudioPlayerGst();

  _HomePageState() {
    AudioPlayerGst.eventsStream().listen((EventBase event) {
      switch(event.runtimeType) {
        case DurationEvent:
          _duration = (event as DurationEvent).duration;
          setState(() {});
        case PlayingStateEvent:
          final playingStateEvent = event as PlayingStateEvent;
          switch(playingStateEvent.state)
          {
            case PlayingState.pending:
            case PlayingState.idle:
            case PlayingState.ready:
              break;
            case PlayingState.playing:
              _playing = true;
              setState(() {});
            case PlayingState.paused:
              _playing = false;
              setState(() {});
            case PlayingState.completed:
              // TODO: Handle this case.
              throw UnimplementedError();
            case PlayingState.unknown:
              // TODO: Handle this case.
              throw UnimplementedError();
          }
        case PositionEvent:
          _position = (event as PositionEvent).position;
          setState(() {});
        case BufferingEvent:
          if(_duration.inMilliseconds == 0) break;
          final buffEvent = event as BufferingEvent;
          _buffered = Duration(microseconds: (_duration.inMicroseconds * buffEvent.percent).round());
          setState(() {});
      }
    });
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('Plugin example app'),
      ),
      body: Column(
        children: [
          Row(
            children: [
              IconButton(
                onPressed: (){
                  if(_playing) {
                    player.pause();
                  }
                  else {
                    player.play();
                  }
                },
                icon: Icon(_playing ? Icons.pause : Icons.play_arrow)
              ),
              const Text('Volume:'),
              Slider(
                value: _volume,
                onChanged: (double value){
                  _volume = value;
                  setState(() {});
                  player.setVolume(value);
                },
              ),
              const Text('Speed:'),
              TextButton(onPressed: () => player.setRate(0.5), child: const Text('0.5x')),
              TextButton(onPressed: () => player.setRate(1.0), child: const Text('1.0x')),
              TextButton(onPressed: () => player.setRate(1.5), child: const Text('1.5x'))
            ],
          ),
          ProgressBar(
            progress: _position,
            total: _duration,
            buffered: _buffered,
            onSeek: (Duration position){
              player.seek(position);
            },
          ),
          const Text('Playlist'),
          ListView(
            shrinkWrap: true,
            children: [
              ListTile(
                leading: IconButton(
                  icon: const Icon(Icons.play_arrow),
                  // onPressed: () => _showMyDialog(context),
                  onPressed: () async {
                    await player.setUrl('file:///media/Media/Music/Avril Lavigne/Let Go/03. Sk8ter Boi.mp3');
                    await player.play();
                  },
                ),
                title: const Text('Track #1'),
              ),
              ListTile(
                leading: IconButton(
                  icon: const Icon(Icons.play_arrow),
                  onPressed: () => _showMyDialog(context),
                ),
                title: const Text('Track #2'),
              ),
              ListTile(
                leading: IconButton(
                  icon: const Icon(Icons.play_arrow),
                  onPressed: () => _showMyDialog(context),
                ),
                title: const Text('Track #3'),
              )
            ],
          ),
        ],
      ),
    );
  }
}

Future<void> _showMyDialog(context) async {
  return showDialog<void>(
    context: context,
    barrierDismissible: false,
    builder: (BuildContext context) {
      return AlertDialog(
        title: const Text('No track specified'),
        content: const SingleChildScrollView(
          child: ListBody(
            children: <Widget>[
              Text('Specify track url and start playing:'),
              Text("await player.setUrl('<track-url>');"),
              Text('await player.play();'),
            ],
          ),
        ),
        actions: <Widget>[
          TextButton(
            child: const Text('OK'),
            onPressed: () {
              Navigator.of(context).pop();
            },
          ),
        ],
      );
    },
  );
}
