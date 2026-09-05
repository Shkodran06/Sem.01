import test from 'node:test';
import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import { fileURLToPath } from 'node:url';
import { initialControllerState, TIMING } from './traffic-controller.ts';

const fragment = readFileSync(fileURLToPath(new URL('../visualization/intersection.fragment.html', import.meta.url)), 'utf8');
const builder = readFileSync(fileURLToPath(new URL('../tools/build-intersection.mjs', import.meta.url)), 'utf8');

void test('visual live mode uses the same safety timing values as the verified controller', () => {
  assert.ok(fragment.includes('window.Sem01Core'));
  assert.ok(fragment.includes('advanceController'));
  assert.ok(builder.includes('browser-core-entry.ts'));
  assert.equal(TIMING.pedestrianClearanceMs, 13500);
});

void test('visual live mode starts fail-safe instead of granting an initial green', () => {
  assert.equal(initialControllerState.stage, 'all-red');
  assert.ok(fragment.includes('controller={...core.initialControllerState}'));
});

void test('visual protected-zone count includes the rear of long vehicles', () => {
  assert.ok(fragment.includes('vehicle.progress-vehicle.rearGap<.72'));
  for (const kind of ['car', 'bicycle', 'bus', 'truck', 'articulated']) assert.ok(fragment.includes(`${kind}:{`));
});

void test('visual pedestrian phase does not grant an approach-wide vehicle green', () => {
  assert.equal(fragment.includes("pedestrianSafeEntry=phase==='ped-green'"), false);
  assert.ok(fragment.includes('active=controller.activeSide'));
  assert.ok(fragment.includes("const pedState=phase==='ped-green'"));
  assert.ok(fragment.includes('.scenario-realtime .pedestrian-world>g:nth-child(3){display:none}'));
  assert.equal(fragment.includes("pedState==='clearance'?'#f0c34d'"), false);
});

void test('pedestrian live phase may release only fully checked conflict-free approaches', () => {
  assert.ok(fragment.includes('item.movements.every(movement=>core.movementMayEnterDuringPedestrianPhase(controller,movement))'));
  assert.ok(fragment.includes('core.phaseIsConflictFree([...selectedMovements,...candidate.movements])'));
  assert.ok(fragment.includes("phase==='all-red')spawn(now)"));
  assert.ok(fragment.includes("phase==='ped-green'&&controller.elapsedMs<timing.redYellow?'red-yellow'"));
  assert.ok(fragment.includes("pedVehicleGreen=phase==='ped-green'&&controller.elapsedMs>=timing.redYellow"));
  assert.ok(fragment.includes("phase==='ped-clearance'&&controller.elapsedMs<timing.yellow?'yellow'"));
});

void test('idle operation is presented as fail-safe all-red, never as powered-off signals', () => {
  assert.equal(fragment.includes('Tutti i semafori sono spenti'), false);
  assert.ok(fragment.includes('attende in tutto rosso'));
});

void test('visual queues keep lane identity and vehicle-length spacing', () => {
  assert.ok(fragment.includes("lane=move==='left'?'left':'through'"));
  assert.ok(fragment.includes('ahead.progress-ahead.rearGap-.025'));
});

void test('live mode releases an opposite approach only through the verified conflict matrix', () => {
  assert.ok(fragment.includes('core.phaseIsConflictFree([...primaryMovements,...oppositeMovements])'));
  assert.ok(fragment.includes('activeSides.includes(vehicle.side)'));
  assert.ok(fragment.includes('paintLive(activeSides,vehicleSignalState,pedState)'));
});

void test('signal timing uses wall-clock elapsed time, not the animation frame cap', () => {
  assert.ok(fragment.includes('const elapsedSeconds=Math.max(0,(now-last)/1000),dt=Math.min(.05,elapsedSeconds)'));
  assert.ok(fragment.includes('let remainingMs=elapsedSeconds*1000'));
  assert.ok(fragment.includes('const stepMs=Math.min(250,remainingMs)'));
});

void test('visual interface demonstrates latched faults and construction interlock', () => {
  assert.ok(fragment.includes('data-event="fault"'));
  assert.ok(fragment.includes('data-event="construction"'));
  assert.ok(fragment.includes("currentEvent==='fault'"));
  assert.ok(fragment.includes("currentEvent==='construction'"));
  assert.ok(fragment.includes('ripristino manuale'));
  assert.ok(fragment.includes('sensore di uscita'));
  assert.ok(fragment.includes('core.advanceConstruction'));
  assert.ok(fragment.includes('core.constructionSafe'));
  assert.ok(fragment.includes("faults.acknowledge('RED_LAMP_FAILURE'"));
  assert.ok(fragment.includes("faults.clear('RED_LAMP_FAILURE'"));
});
