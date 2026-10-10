sealed class EventBase {}

class UnknownEvent extends EventBase {}

class DurationEvent extends EventBase {
  final Duration duration;

  DurationEvent(this.duration);

  @override
  bool operator ==(Object other) =>
      identical(this, other) || other is DurationEvent && duration == other.duration;

  @override
  int get hashCode => duration.hashCode;
}

enum PlayingState { pending, idle, ready, playing, paused, completed, unknown }

class PlayingStateEvent extends EventBase {
  final PlayingState state;

  PlayingStateEvent(this.state);

  @override
  bool operator ==(Object other) =>
      identical(this, other) || other is PlayingStateEvent && state == other.state;

  @override
  int get hashCode => state.hashCode;
}

class PositionEvent extends EventBase {
  final Duration position;

  PositionEvent(this.position);

  @override
  bool operator ==(Object other) =>
      identical(this, other) || other is PositionEvent && position == other.position;

  @override
  int get hashCode => position.hashCode;
}

class BufferingEvent extends EventBase {
  final double percent;

  BufferingEvent(this.percent);

  @override
  bool operator ==(Object other) =>
      identical(this, other) || other is BufferingEvent && percent == other.percent;

  @override
  int get hashCode => percent.hashCode;
}

class VolumeEvent extends EventBase {
  final double value;

  VolumeEvent(this.value);

  @override
  bool operator ==(Object other) =>
      identical(this, other) || other is VolumeEvent && value == other.value;

  @override
  int get hashCode => value.hashCode;
}
