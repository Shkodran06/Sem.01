import test from 'node:test';
import assert from 'node:assert/strict';
import { advanceConstruction, constructionSafe, initialConstructionState, type ConstructionInputs } from './construction-controller.ts';
import { FaultRegister } from './diagnostics.ts';
import { OccupancyTracker, PedestrianQueue, rearProgressM, type TrackedRoadUser } from './traffic-domain.ts';
import { DEMO_CONFIGURATION, pedestrianClearanceMs, validateConfiguration } from './site-configuration.ts';
import { ALL_MOVEMENTS, chooseMovementPhase, movementsConflict, phaseIsConflictFree } from './movement-planner.ts';

const user = (partial: Partial<TrackedRoadUser> = {}): TrackedRoadUser => ({
  id: 'V1', kind: 'car', movement: 'north:straight', frontProgressM: 0,
  speedKmh: 20, lastSeenMs: 1000, sensorConfidence: 1, ...partial,
});

void test('a long vehicle occupies the zone until its rear has left', () => {
  const car = user({ kind: 'car', frontProgressM: 34 });
  const articulated = user({ id: 'T1', kind: 'articulated-truck', frontProgressM: 34 });
  assert.ok(rearProgressM(articulated) < rearProgressM(car));
  const tracker = new OccupancyTracker(); tracker.update(articulated);
  assert.equal(tracker.occupants(10, 30).length, 1);
  tracker.update({ ...articulated, frontProgressM: 49 });
  assert.equal(tracker.occupants(10, 30).length, 0);
});

void test('lost or uncertain tracking prevents a clear-zone confirmation', () => {
  const tracker = new OccupancyTracker();
  tracker.update(user({ frontProgressM: 50, lastSeenMs: 0 }));
  assert.equal(tracker.mayConfirmClear(10, 30, 1000), false);
  tracker.update(user({ frontProgressM: 50, lastSeenMs: 1000, sensorConfidence: 0.95 }));
  assert.equal(tracker.mayConfirmClear(10, 30, 1000), true);
});

void test('pedestrian calls from all four sides are stored once and served oldest first', () => {
  const queue = new PedestrianQueue();
  queue.request('south', 30); queue.request('north', 10); queue.request('east', 20); queue.request('west', 40); queue.request('north', 50);
  assert.equal(queue.size, 4);
  assert.deepEqual(queue.takeOldest(), { crossing: 'north', requestedAtMs: 10 });
});

void test('critical faults require acknowledgement and a safe condition before clearing', () => {
  const faults = new FaultRegister(); faults.raise('RED_LAMP_FAILURE', 'north:mother', 100);
  assert.equal(faults.hasCritical(), true);
  assert.equal(faults.clear('RED_LAMP_FAILURE', 'north:mother', true), false);
  faults.acknowledge('RED_LAMP_FAILURE', 'north:mother');
  assert.equal(faults.clear('RED_LAMP_FAILURE', 'north:mother', false), false);
  assert.equal(faults.clear('RED_LAMP_FAILURE', 'north:mother', true), true);
  assert.equal(faults.hasCritical(), false);
});

const constructionInputs = (partial: Partial<ConstructionInputs> = {}): ConstructionInputs => ({
  demand: { master: 0, slave: 0 }, corridorOccupied: false, linkOnline: true, emergency: false, ...partial,
});
const timing = { allRedMs: 4000, minGreenMs: 4000, maxGreenMs: 9000, yellowMs: 3000 };

void test('construction controller never reverses while the one-lane corridor is occupied', () => {
  let state = advanceConstruction(initialConstructionState, constructionInputs({ demand: { master: 1, slave: 1 } }), timing, timing.allRedMs);
  assert.equal(state.active, 'master');
  state = advanceConstruction(state, constructionInputs({ corridorOccupied: true, demand: { master: 0, slave: 5 } }), timing, 30000);
  assert.equal(state.stage, 'green');
  assert.equal(state.active, 'master');
});

void test('construction radio loss produces a latched all-red fault', () => {
  let state = advanceConstruction(initialConstructionState, constructionInputs({ linkOnline: false }), timing, 100);
  assert.equal(state.stage, 'fault'); assert.equal(constructionSafe(state), true);
  state = advanceConstruction(state, constructionInputs(), timing, 10000);
  assert.equal(state.stage, 'fault');
  state = advanceConstruction(state, constructionInputs({ resetFault: true }), timing, 100);
  assert.equal(state.stage, 'all-red');
});

void test('construction randomized run preserves its safety invariant', () => {
  let state = initialConstructionState;
  for (let step = 0; step < 10000; step += 1) {
    const emergency = step % 997 === 0;
    state = advanceConstruction(state, constructionInputs({
      emergency, resetFault: !emergency && state.faultLatched && step % 13 === 0,
      corridorOccupied: step % 41 === 0, demand: { master: step % 4, slave: step % 7 },
    }), timing, 100);
    assert.equal(constructionSafe(state), true);
  }
});

void test('default site configuration passes validation', () => {
  assert.deepEqual(validateConfiguration(DEMO_CONFIGURATION), []);
  assert.ok(pedestrianClearanceMs(DEMO_CONFIGURATION) >= 12000);
});

void test('unsafe site timing is rejected before the controller starts', () => {
  const issues = validateConfiguration({ ...DEMO_CONFIGURATION, minGreenMs: 3500, maxGreenMs: 3000 });
  assert.ok(issues.some((issue) => issue.field === 'minGreenMs' && issue.severity === 'error'));
  assert.ok(issues.some((issue) => issue.field === 'maxGreenMs' && issue.severity === 'error'));
});

void test('movement conflict matrix is symmetric for every pair', () => {
  for (const a of ALL_MOVEMENTS) for (const b of ALL_MOVEMENTS) assert.equal(movementsConflict(a, b), movementsConflict(b, a));
});

void test('opposite straight movements are compatible but a conflicting left turn is excluded', () => {
  assert.equal(movementsConflict('north:straight', 'south:straight'), false);
  assert.equal(movementsConflict('north:left', 'south:straight'), true);
  const phase = chooseMovementPhase({ 'north:straight': 3, 'south:straight': 2, 'north:left': 1 });
  assert.deepEqual(phase, ['north:straight', 'south:straight']);
});

void test('planner never returns a conflicting phase under varied demand', () => {
  for (let run = 0; run < 500; run += 1) {
    const demand = Object.fromEntries(ALL_MOVEMENTS.map((movement, index) => [movement, (run * (index + 3)) % 7]));
    assert.equal(phaseIsConflictFree(chooseMovementPhase(demand)), true);
  }
});
