export type ConstructionSide = 'master' | 'slave';
export type ConstructionStage = 'all-red' | 'green' | 'yellow' | 'fault';
export type ConstructionState = { stage: ConstructionStage; active: ConstructionSide | null; elapsedMs: number; last: ConstructionSide; faultLatched: boolean };
export type ConstructionInputs = { demand: Record<ConstructionSide, number>; corridorOccupied: boolean; linkOnline: boolean; emergency: boolean; resetFault?: boolean };
export type ConstructionTiming = { allRedMs: number; minGreenMs: number; maxGreenMs: number; yellowMs: number };

export const initialConstructionState: ConstructionState = { stage: 'all-red', active: null, elapsedMs: 0, last: 'slave', faultLatched: false };

export function advanceConstruction(state: ConstructionState, inputs: ConstructionInputs, timing: ConstructionTiming, deltaMs: number): ConstructionState {
  if (inputs.emergency || !inputs.linkOnline) return { ...state, stage: 'fault', active: null, elapsedMs: 0, faultLatched: true };
  if (state.faultLatched || state.stage === 'fault') {
    if (!inputs.resetFault) return { ...state, stage: 'fault', active: null, faultLatched: true };
    return { ...initialConstructionState, last: state.last };
  }
  const elapsedMs = state.elapsedMs + Math.max(0, deltaMs);
  if (state.stage === 'all-red') {
    if (inputs.corridorOccupied || elapsedMs < timing.allRedMs) return { ...state, elapsedMs };
    const other = state.last === 'master' ? 'slave' : 'master';
    const next = inputs.demand[other] ? other : inputs.demand[state.last] ? state.last : null;
    return next ? { ...state, stage: 'green', active: next, elapsedMs: 0 } : { ...state, elapsedMs };
  }
  if (state.stage === 'green') {
    const ownDemand = state.active ? inputs.demand[state.active] : 0;
    if (inputs.corridorOccupied || elapsedMs < timing.minGreenMs || (ownDemand && elapsedMs < timing.maxGreenMs)) return { ...state, elapsedMs };
    return { ...state, stage: 'yellow', elapsedMs: 0 };
  }
  return elapsedMs >= timing.yellowMs
    ? { stage: 'all-red', active: null, elapsedMs: 0, last: state.active ?? state.last, faultLatched: false }
    : { ...state, elapsedMs };
}

export function constructionSafe(state: ConstructionState): boolean {
  return state.stage === 'green' || state.stage === 'yellow' ? state.active !== null && !state.faultLatched : state.active === null;
}
