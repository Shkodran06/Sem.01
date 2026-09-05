import test from 'node:test';
import assert from 'node:assert/strict';
import { advanceController, assertSafeState, clearanceTimeMs, initialControllerState, movementConflictsWithCrossing, movementMayEnterDuringPedestrianPhase, pedestrianMayEnter, TIMING, vehicleMayEnter, type ControllerInputs } from './traffic-controller.ts';

const inputs = (partial: Partial<ControllerInputs> = {}): ControllerInputs => ({
  emergency: false,
  zoneOccupied: false,
  pedestrianWaiting: false,
  pedestrianCrossing: false,
  demand: { north: 0, east: 0, south: 0, west: 0 },
  enabledSides: ['north', 'east', 'south', 'west'],
  ...partial,
});

void test('never releases traffic while the protected zone is occupied', () => {
  const state = advanceController(initialControllerState, inputs({ zoneOccupied: true, demand: { north: 3, east: 0, south: 0, west: 0 } }), 10000);
  assert.equal(state.stage, 'all-red');
});

void test('uses all-red, red-yellow, green, yellow and all-red sequence', () => {
  const demand = { north: 2, east: 0, south: 0, west: 0 };
  let state = advanceController(initialControllerState, inputs({ demand }), TIMING.allRedMs);
  assert.equal(state.stage, 'red-yellow');
  state = advanceController(state, inputs({ demand }), TIMING.redYellowMs);
  assert.equal(state.stage, 'green');
  state = advanceController(state, inputs({ demand: { ...demand, north: 0 } }), TIMING.minGreenMs);
  assert.equal(state.stage, 'yellow');
  state = advanceController(state, inputs(), TIMING.yellowMs);
  assert.equal(state.stage, 'all-red');
});

void test('only one cardinal approach can receive green', () => {
  let state = advanceController(initialControllerState, inputs({ demand: { north: 1, east: 5, south: 2, west: 4 } }), TIMING.allRedMs);
  state = advanceController(state, inputs(), TIMING.redYellowMs);
  assert.equal(state.activeSide, 'east');
  assert.equal(vehicleMayEnter(state, 'east'), true);
  assert.equal(vehicleMayEnter(state, 'north'), false);
  assert.equal(vehicleMayEnter(state, 'south'), false);
  assert.equal(vehicleMayEnter(state, 'west'), false);
});

void test('pedestrian request receives an exclusive phase and is held during crossing', () => {
  let state = advanceController(initialControllerState, inputs({ pedestrianWaiting: true, pedestrianSide: 'north' }), TIMING.allRedMs);
  assert.equal(state.stage, 'ped-green');
  assert.equal(pedestrianMayEnter(state), true);
  assert.equal(vehicleMayEnter(state, 'north'), false);
  state = advanceController(state, inputs({ pedestrianCrossing: true }), TIMING.pedestrianMinGreenMs);
  assert.equal(state.stage, 'ped-clearance');
  state = advanceController(state, inputs({ pedestrianCrossing: true }), 10000);
  assert.equal(state.stage, 'ped-clearance');
});

void test('pedestrian clearance cannot end before the configured safe interval', () => {
  let state = advanceController(initialControllerState, inputs({ pedestrianWaiting: true, pedestrianSide: 'west' }), TIMING.allRedMs);
  state = advanceController(state, inputs(), TIMING.pedestrianMinGreenMs);
  state = advanceController(state, inputs(), TIMING.pedestrianClearanceMs - 1);
  assert.equal(state.stage, 'ped-clearance');
  state = advanceController(state, inputs(), 1);
  assert.equal(state.stage, 'all-red');
});

void test('accepted pedestrian call does not hold green forever', () => {
  let state = advanceController(initialControllerState, inputs({ pedestrianWaiting: true, pedestrianSide: 'east' }), TIMING.allRedMs);
  state = advanceController(state, inputs({ pedestrianWaiting: true }), TIMING.pedestrianMinGreenMs);
  assert.equal(state.stage, 'ped-clearance');
});

void test('pedestrian phase permits only geometrically compatible movements', () => {
  const state = advanceController(initialControllerState, inputs({ pedestrianWaiting: true, pedestrianSide: 'north' }), TIMING.allRedMs);
  assert.equal(movementConflictsWithCrossing('north:straight', 'north'), true);
  assert.equal(movementConflictsWithCrossing('south:straight', 'north'), true);
  assert.equal(movementConflictsWithCrossing('east:straight', 'north'), false);
  assert.equal(movementMayEnterDuringPedestrianPhase(state, 'east:straight'), true);
  assert.equal(movementMayEnterDuringPedestrianPhase(state, 'south:straight'), false);
});

void test('emergency produces fail-safe all-red fault state', () => {
  const green = { ...initialControllerState, stage: 'green' as const, activeSide: 'north' as const };
  const state = advanceController(green, inputs({ emergency: true }), 100);
  assert.equal(state.stage, 'fault');
  assert.equal(state.activeSide, null);
  assert.equal(assertSafeState(state), true);
});

void test('fault remains latched until an explicit reset', () => {
  let state = advanceController(initialControllerState, inputs({ emergency: true }), 100);
  state = advanceController(state, inputs(), 10000);
  assert.equal(state.stage, 'fault');
  assert.equal(state.faultLatched, true);
  state = advanceController(state, inputs({ emergency: true, resetFault: true }), 100);
  assert.equal(state.stage, 'fault');
  state = advanceController(state, inputs({ resetFault: true }), 100);
  assert.equal(state.stage, 'all-red');
  assert.equal(state.faultLatched, false);
});

void test('minimum green is at least four seconds', () => {
  assert.ok(TIMING.minGreenMs >= 4000);
});

void test('slow users extend calculated clearance time', () => {
  assert.ok(clearanceTimeMs(30, 9) > clearanceTimeMs(30, 30));
  assert.ok(clearanceTimeMs(0, 30) >= TIMING.allRedMs);
});

void test('randomized long run never creates an invalid signal state', () => {
  let state = initialControllerState;
  for (let step = 0; step < 20000; step += 1) {
    const demand = { north: step % 7, east: step % 5, south: step % 3, west: step % 11 };
    state = advanceController(state, inputs({ demand, zoneOccupied: step % 97 === 0, pedestrianWaiting: step % 211 === 0, pedestrianCrossing: step % 223 === 0 }), 100);
    assert.equal(assertSafeState(state), true);
    if (pedestrianMayEnter(state)) sides.forEach(side => assert.equal(vehicleMayEnter(state, side), false));
  }
});

const sides = ['north', 'east', 'south', 'west'] as const;
