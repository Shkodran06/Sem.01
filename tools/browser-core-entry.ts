import { advanceController, initialControllerState, movementMayEnterDuringPedestrianPhase, TIMING } from '../lib/traffic-controller.ts';
import { chooseMovementPhase, movementsConflict, phaseIsConflictFree } from '../lib/movement-planner.ts';
import { advanceConstruction, constructionSafe, initialConstructionState } from '../lib/construction-controller.ts';
import { FaultRegister } from '../lib/diagnostics.ts';

declare global {
  interface Window {
    Sem01Core: {
      advanceController: typeof advanceController;
      initialControllerState: typeof initialControllerState;
      movementMayEnterDuringPedestrianPhase: typeof movementMayEnterDuringPedestrianPhase;
      TIMING: typeof TIMING;
      chooseMovementPhase: typeof chooseMovementPhase;
      movementsConflict: typeof movementsConflict;
      phaseIsConflictFree: typeof phaseIsConflictFree;
      advanceConstruction: typeof advanceConstruction;
      constructionSafe: typeof constructionSafe;
      initialConstructionState: typeof initialConstructionState;
      FaultRegister: typeof FaultRegister;
    };
  }
}

window.Sem01Core = {
  advanceController,
  initialControllerState,
  movementMayEnterDuringPedestrianPhase,
  TIMING,
  chooseMovementPhase,
  movementsConflict,
  phaseIsConflictFree,
  advanceConstruction,
  constructionSafe,
  initialConstructionState,
  FaultRegister,
};
