export type Side = 'north' | 'east' | 'south' | 'west';
export type Turn = 'left' | 'straight' | 'right';
export type Movement = `${Side}:${Turn}`;
export type Stage = 'all-red' | 'red-yellow' | 'green' | 'yellow' | 'ped-green' | 'ped-clearance' | 'fault';

export type ControllerState = { stage: Stage; activeSide: Side | null; activeCrossing: Side | null; elapsedMs: number; lastSide: Side; faultLatched: boolean };
export type ControllerInputs = {
  emergency: boolean; resetFault?: boolean; zoneOccupied: boolean;
  pedestrianWaiting: boolean; pedestrianCrossing: boolean; pedestrianSide?: Side;
  demand: Record<Side, number>; enabledSides: Side[];
};

// Conservative demo defaults. A real installation needs site-specific values.
export const TIMING = {
  allRedMs: 1200, redYellowMs: 900, minGreenMs: 4000, maxGreenMs: 9000,
  yellowMs: 3000, pedestrianMinGreenMs: 5000, pedestrianClearanceMs: 13500,
} as const;

export const initialControllerState: ControllerState = {
  stage: 'all-red', activeSide: null, activeCrossing: null, elapsedMs: 0, lastSide: 'west', faultLatched: false,
};

const order: Side[] = ['north', 'east', 'south', 'west'];

export function chooseNextSide(inputs: ControllerInputs, lastSide: Side): Side | null {
  const start = order.indexOf(lastSide);
  return order.map((side, index) => ({ side, distance: (index - start + 4) % 4 || 4 }))
    .filter(({ side }) => inputs.enabledSides.includes(side) && inputs.demand[side] > 0)
    .sort((a, b) => inputs.demand[b.side] - inputs.demand[a.side] || a.distance - b.distance)[0]?.side ?? null;
}

export function destinationOf(side: Side, turn: Turn): Side {
  return order[(order.indexOf(side) + (turn === 'left' ? 1 : turn === 'straight' ? 2 : 3)) % 4];
}

/** A crossing on an arm conflicts with vehicles entering or leaving that arm. */
export function movementConflictsWithCrossing(movement: Movement, crossing: Side): boolean {
  const [origin, turn] = movement.split(':') as [Side, Turn];
  return origin === crossing || destinationOf(origin, turn) === crossing;
}

export function movementsCompatibleWithCrossing(crossing: Side): Movement[] {
  return order.flatMap((side) => (['left', 'straight', 'right'] as const)
    .map((turn) => `${side}:${turn}` as Movement)
    .filter((movement) => !movementConflictsWithCrossing(movement, crossing)));
}

export function clearanceTimeMs(distanceM: number, speedKmh: number, minimumMs = TIMING.allRedMs, marginMs = 1500): number {
  const speed = Math.min(80, Math.max(3, speedKmh));
  return Math.max(minimumMs, Math.ceil((Math.max(0, distanceM) / (speed / 3.6)) * 1000 + marginMs));
}

export function advanceController(state: ControllerState, inputs: ControllerInputs, deltaMs: number): ControllerState {
  if (inputs.emergency) return { ...state, stage: 'fault', activeSide: null, activeCrossing: null, elapsedMs: 0, faultLatched: true };
  if (state.faultLatched || state.stage === 'fault') {
    if (!inputs.resetFault) return { ...state, stage: 'fault', activeSide: null, activeCrossing: null, faultLatched: true };
    return { ...initialControllerState, lastSide: state.lastSide };
  }
  const elapsedMs = state.elapsedMs + Math.max(0, deltaMs);
  switch (state.stage) {
    case 'all-red': {
      if (inputs.zoneOccupied || elapsedMs < TIMING.allRedMs) return { ...state, elapsedMs };
      if (inputs.pedestrianWaiting) return { ...state, stage: 'ped-green', activeSide: null, activeCrossing: inputs.pedestrianSide ?? 'north', elapsedMs: 0 };
      const next = chooseNextSide(inputs, state.lastSide);
      return next ? { ...state, stage: 'red-yellow', activeSide: next, activeCrossing: null, elapsedMs: 0 } : { ...state, elapsedMs };
    }
    case 'red-yellow': return elapsedMs >= TIMING.redYellowMs ? { ...state, stage: 'green', elapsedMs: 0 } : { ...state, elapsedMs };
    case 'green': {
      const demandRemains = state.activeSide ? inputs.demand[state.activeSide] > 0 : false;
      if (inputs.zoneOccupied || elapsedMs < TIMING.minGreenMs || (demandRemains && elapsedMs < TIMING.maxGreenMs)) return { ...state, elapsedMs };
      return { ...state, stage: 'yellow', elapsedMs: 0 };
    }
    case 'yellow': return elapsedMs >= TIMING.yellowMs
      ? { ...state, stage: 'all-red', activeSide: null, activeCrossing: null, elapsedMs: 0, lastSide: state.activeSide ?? state.lastSide }
      : { ...state, elapsedMs };
    case 'ped-green': return elapsedMs < TIMING.pedestrianMinGreenMs ? { ...state, elapsedMs } : { ...state, stage: 'ped-clearance', elapsedMs: 0 };
    case 'ped-clearance': return inputs.pedestrianCrossing || elapsedMs < TIMING.pedestrianClearanceMs
      ? { ...state, elapsedMs }
      : { ...state, stage: 'all-red', activeSide: null, activeCrossing: null, elapsedMs: 0 };
  }
}

export function vehicleMayEnter(state: ControllerState, side: Side): boolean {
  return state.stage === 'green' && state.activeSide === side && !state.faultLatched;
}
export function movementMayEnterDuringPedestrianPhase(state: ControllerState, movement: Movement): boolean {
  return state.stage === 'ped-green' && state.activeCrossing !== null && !movementConflictsWithCrossing(movement, state.activeCrossing) && !state.faultLatched;
}
export function pedestrianMayEnter(state: ControllerState, crossing?: Side): boolean {
  return state.stage === 'ped-green' && !state.faultLatched && (crossing === undefined || state.activeCrossing === crossing);
}
export function assertSafeState(state: ControllerState): boolean {
  if (state.faultLatched || state.stage === 'fault') return state.stage === 'fault' && state.activeSide === null && state.activeCrossing === null;
  if (['green', 'red-yellow', 'yellow'].includes(state.stage)) return state.activeSide !== null && state.activeCrossing === null;
  if (['ped-green', 'ped-clearance'].includes(state.stage)) return state.activeSide === null && state.activeCrossing !== null;
  return state.activeSide === null && state.activeCrossing === null;
}
