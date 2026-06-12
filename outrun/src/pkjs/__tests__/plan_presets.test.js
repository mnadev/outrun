var PlanPresets = require('../plan_presets');

describe('plan_presets', function () {
  test('encodeSegment produces 7 bytes', function () {
    var bytes = PlanPresets.encodeSegment({
      label: PlanPresets.SegmentLabel.RUN,
      endType: PlanPresets.SegmentEndType.DURATION,
      endValue: 1200,
      targetType: PlanPresets.SegmentTargetType.PACE,
      targetLo: 300,
      targetHi: 330
    });

    expect(bytes).toHaveLength(7);
    expect(bytes[0]).toBe(0x09);
    expect(bytes[1]).toBe(0xB0);
    expect(bytes[2]).toBe(0x04);
  });

  test('parseSegment round-trips encoded segment', function () {
    var original = {
      label: PlanPresets.SegmentLabel.RUN,
      endType: PlanPresets.SegmentEndType.DISTANCE,
      endValue: 400,
      targetType: PlanPresets.SegmentTargetType.PACE,
      targetLo: 270,
      targetHi: 285
    };

    var bytes = PlanPresets.encodeSegment(original);
    var parsed = PlanPresets.parseSegment(bytes, 0);

    expect(parsed.label).toBe(original.label);
    expect(parsed.endType).toBe(original.endType);
    expect(parsed.endValue).toBe(original.endValue);
    expect(parsed.targetType).toBe(original.targetType);
    expect(parsed.targetLo).toBe(original.targetLo);
    expect(parsed.targetHi).toBe(original.targetHi);
  });

  test('getPresetPlans returns four plans', function () {
    var plans = PlanPresets.getPresetPlans();
    expect(plans).toHaveLength(4);
    expect(plans[0].name).toBe('Easy Run');
    expect(plans[3].segments[0].targetType).toBe(PlanPresets.SegmentTargetType.HR);
  });
});
