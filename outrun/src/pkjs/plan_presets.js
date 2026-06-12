/**
 * plan_presets.js - Preset running plans encoded for watch transfer
 */

var SEGMENT_PACKED_SIZE = 7;

var SegmentLabel = {
  WARMUP: 0,
  RUN: 1,
  RECOVER: 2,
  COOLDOWN: 3
};

var SegmentEndType = {
  DURATION: 0,
  DISTANCE: 1
};

var SegmentTargetType = {
  NONE: 0,
  PACE: 1,
  HR: 2
};

function encodeSegment(segment) {
  var flags = segment.label |
    (segment.endType << 2) |
    (segment.targetType << 3);

  var bytes = [
    flags & 0xFF,
    segment.endValue & 0xFF,
    (segment.endValue >> 8) & 0xFF,
    segment.targetLo & 0xFF,
    (segment.targetLo >> 8) & 0xFF,
    segment.targetHi & 0xFF,
    (segment.targetHi >> 8) & 0xFF
  ];
  return bytes;
}

function encodePlan(segments) {
  var packed = [];
  for (var i = 0; i < segments.length; i++) {
    var segBytes = encodeSegment(segments[i]);
    packed = packed.concat(segBytes);
  }
  return packed;
}

function getPresetPlans() {
  return [
    {
      name: 'Easy Run',
      segments: [
        {
          label: SegmentLabel.RUN,
          endType: SegmentEndType.DURATION,
          endValue: 20 * 60,
          targetType: SegmentTargetType.PACE,
          targetLo: 300,
          targetHi: 330
        }
      ]
    },
    {
      name: 'Tempo',
      segments: [
        {
          label: SegmentLabel.WARMUP,
          endType: SegmentEndType.DURATION,
          endValue: 5 * 60,
          targetType: SegmentTargetType.NONE,
          targetLo: 0,
          targetHi: 0
        },
        {
          label: SegmentLabel.RUN,
          endType: SegmentEndType.DURATION,
          endValue: 15 * 60,
          targetType: SegmentTargetType.PACE,
          targetLo: 285,
          targetHi: 300
        },
        {
          label: SegmentLabel.COOLDOWN,
          endType: SegmentEndType.DURATION,
          endValue: 5 * 60,
          targetType: SegmentTargetType.NONE,
          targetLo: 0,
          targetHi: 0
        }
      ]
    },
    {
      name: 'Intervals',
      segments: buildIntervalSegments()
    },
    {
      name: 'Zone 2 Base',
      segments: [
        {
          label: SegmentLabel.RUN,
          endType: SegmentEndType.DURATION,
          endValue: 30 * 60,
          targetType: SegmentTargetType.HR,
          targetLo: 120,
          targetHi: 150
        }
      ]
    }
  ];
}

function buildIntervalSegments() {
  var segments = [
    {
      label: SegmentLabel.WARMUP,
      endType: SegmentEndType.DURATION,
      endValue: 5 * 60,
      targetType: SegmentTargetType.NONE,
      targetLo: 0,
      targetHi: 0
    }
  ];

  for (var i = 0; i < 5; i++) {
    segments.push({
      label: SegmentLabel.RUN,
      endType: SegmentEndType.DISTANCE,
      endValue: 400,
      targetType: SegmentTargetType.PACE,
      targetLo: 270,
      targetHi: 285
    });
    segments.push({
      label: SegmentLabel.RECOVER,
      endType: SegmentEndType.DISTANCE,
      endValue: 200,
      targetType: SegmentTargetType.NONE,
      targetLo: 0,
      targetHi: 0
    });
  }

  segments.push({
    label: SegmentLabel.COOLDOWN,
    endType: SegmentEndType.DURATION,
    endValue: 5 * 60,
    targetType: SegmentTargetType.NONE,
    targetLo: 0,
    targetHi: 0
  });

  return segments;
}

function parseSegment(bytes, offset) {
  var flags = bytes[offset];
  return {
    label: flags & 0x03,
    endType: (flags >> 2) & 0x01,
    targetType: (flags >> 3) & 0x03,
    endValue: bytes[offset + 1] | (bytes[offset + 2] << 8),
    targetLo: bytes[offset + 3] | (bytes[offset + 4] << 8),
    targetHi: bytes[offset + 5] | (bytes[offset + 6] << 8)
  };
}

module.exports = {
  SEGMENT_PACKED_SIZE: SEGMENT_PACKED_SIZE,
  SegmentLabel: SegmentLabel,
  SegmentEndType: SegmentEndType,
  SegmentTargetType: SegmentTargetType,
  encodeSegment: encodeSegment,
  encodePlan: encodePlan,
  getPresetPlans: getPresetPlans,
  parseSegment: parseSegment
};
