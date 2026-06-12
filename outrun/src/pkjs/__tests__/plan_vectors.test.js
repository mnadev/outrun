var PlanPresets = require('../plan_presets');

describe('plan preset vectors', function () {
  test('Easy Run bytes match C parser expectations', function () {
    var plans = PlanPresets.getPresetPlans();
    var bytes = PlanPresets.encodePlan(plans[0].segments);
    expect(bytes).toEqual([0x09, 0xB0, 0x04, 0x2C, 0x01, 0x4A, 0x01]);
  });

  test('all preset plans round-trip through parseSegment', function () {
    var plans = PlanPresets.getPresetPlans();
    plans.forEach(function (plan) {
      var packed = PlanPresets.encodePlan(plan.segments);
      expect(packed.length).toBe(plan.segments.length * PlanPresets.SEGMENT_PACKED_SIZE);

      for (var i = 0; i < plan.segments.length; i++) {
        var parsed = PlanPresets.parseSegment(packed, i * PlanPresets.SEGMENT_PACKED_SIZE);
        var original = plan.segments[i];
        expect(parsed.label).toBe(original.label);
        expect(parsed.endType).toBe(original.endType);
        expect(parsed.endValue).toBe(original.endValue);
        expect(parsed.targetType).toBe(original.targetType);
        expect(parsed.targetLo).toBe(original.targetLo);
        expect(parsed.targetHi).toBe(original.targetHi);
      }
    });
  });

  test('Intervals plan uses distance segments', function () {
    var plans = PlanPresets.getPresetPlans();
    var intervals = plans[2];
    var distanceSegments = intervals.segments.filter(function (seg) {
      return seg.endType === PlanPresets.SegmentEndType.DISTANCE;
    });
    expect(distanceSegments.length).toBe(10);
    expect(distanceSegments[0].endValue).toBe(400);
  });
});
