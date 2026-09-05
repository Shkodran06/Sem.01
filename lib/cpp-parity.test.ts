import test from 'node:test';
import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import { fileURLToPath } from 'node:url';
import { TIMING } from './traffic-controller.ts';

const header = readFileSync(fileURLToPath(new URL('../simulator/safetylogic.h', import.meta.url)), 'utf8');
const source = readFileSync(fileURLToPath(new URL('../simulator/main.cpp', import.meta.url)), 'utf8');

const cppSeconds = (name: string): number => {
  const match = header.match(new RegExp(`${name}=([0-9.]+);`));
  assert.ok(match, `Missing C++ timing constant ${name}`);
  return Number(match[1]);
};

void test('C++ and web controllers share every fixed signal timing', () => {
  assert.equal(cppSeconds('AllRedSeconds') * 1000, TIMING.allRedMs);
  assert.equal(cppSeconds('RedYellowSeconds') * 1000, TIMING.redYellowMs);
  assert.equal(cppSeconds('MinimumVehicleGreenSeconds') * 1000, TIMING.minGreenMs);
  assert.equal(cppSeconds('VehicleYellowSeconds') * 1000, TIMING.yellowMs);
  assert.equal(cppSeconds('PedestrianGreenSeconds') * 1000, TIMING.pedestrianMinGreenMs);
  assert.equal(cppSeconds('PedestrianClearanceSeconds') * 1000, TIMING.pedestrianClearanceMs);
});

void test('C++ executable uses the shared red-yellow phase and rear-of-vehicle model', () => {
  assert.ok(source.includes('lamp=Lamp::RedYellow'));
  assert.ok(source.includes('SafetyTiming::RedYellowSeconds'));
  assert.ok(source.includes('normalizedRearGap(safetyKind(t.kind))'));
  assert.ok(source.includes('advancePhase(elapsed)'));
  assert.ok(source.includes('move(motionStep)'));
  assert.ok(source.includes('choosePedestrianCompatibleGrants()'));
  assert.ok(source.includes('laneGroupCompatibleWithCrossing(group,pedSide)'));
  assert.ok(source.includes('t.progress>=.285&&t.progress<.305&&!greenFor(t)'));
  assert.equal(source.includes('yellowFor(t)'), false);
});

void test('C++ lane policy is uniform on all four approaches', () => {
  assert.ok(header.includes('lane==0?movementBit(Maneuver::Left):movementBit(Maneuver::Straight)|movementBit(Maneuver::Right)'));
  assert.equal(source.includes('const bool vertical='), false);
});
