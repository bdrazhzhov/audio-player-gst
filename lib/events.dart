import 'package:equatable/equatable.dart';

sealed class EventBase extends Equatable {}

class UnknownEvent extends EventBase {
  @override
  List<Object?> get props => [];
}

class DurationEvent extends EventBase {
  final Duration duration;

  DurationEvent(this.duration);

  @override
  List<Object> get props => [duration];
}

enum PlayingState { pending, idle, ready, playing, paused, completed, unknown }
class PlayingStateEvent extends EventBase {
  final PlayingState state;

  PlayingStateEvent(this.state);

  @override
  List<Object> get props => [state];
}

class PositionEvent extends EventBase {
  final Duration position;

  PositionEvent(this.position);

  @override
  List<Object?> get props => [position];
}

class BufferingEvent extends EventBase {
  final double percent;

  BufferingEvent(this.percent);

  @override
  List<Object?> get props => [percent];
}

class VolumeEvent extends EventBase {
  final double value;

  VolumeEvent(this.value);

  @override
  List<Object?> get props => [value];
}
