import { destinationOf, type Movement, type Side, type Turn } from './traffic-controller.ts';

const sides: Side[] = ['north', 'east', 'south', 'west'];
const turns: Turn[] = ['left', 'straight', 'right'];
export const ALL_MOVEMENTS: Movement[] = sides.flatMap((side) => turns.map((turn) => `${side}:${turn}` as Movement));

const opposite = (a: Side, b: Side): boolean => Math.abs(sides.indexOf(a) - sides.indexOf(b)) === 2;

/**
 * Conservative conflict model for the two-lane layout:
 * lane 1 is left-only; lane 2 is straight/right. Movements merging into the
 * same exit conflict. Adjacent approaches conflict except separated right
 * turns. Opposite non-left movements can run together.
 */
export function movementsConflict(a: Movement, b: Movement): boolean {
  if (a === b) return false;
  const [aSide, aTurn] = a.split(':') as [Side, Turn];
  const [bSide, bTurn] = b.split(':') as [Side, Turn];
  if (aSide === bSide) return false;
  if (destinationOf(aSide, aTurn) === destinationOf(bSide, bTurn)) return true;
  if (opposite(aSide, bSide)) return aTurn === 'left' || bTurn === 'left';
  return !(aTurn === 'right' && bTurn === 'right');
}
export function phaseIsConflictFree(movements: Movement[]): boolean {
  return movements.every((movement, index) => movements.slice(index + 1).every((other) => !movementsConflict(movement, other)));
}

/** Selects the highest-demand conflict-free set; stable order prevents flicker. */
export function chooseMovementPhase(demand: Partial<Record<Movement, number>>, enabled: Movement[] = ALL_MOVEMENTS): Movement[] {
  const candidates = enabled.filter((movement) => (demand[movement] ?? 0) > 0);
  let best: Movement[] = [], bestScore = 0;
  for (let mask = 1; mask < 1 << candidates.length; mask += 1) {
    const phase = candidates.filter((_, index) => mask & (1 << index));
    if (!phaseIsConflictFree(phase)) continue;
    const score = phase.reduce((sum, movement) => sum + (demand[movement] ?? 0), 0);
    if (score > bestScore || (score === bestScore && phase.length > best.length)) { best = phase; bestScore = score; }
  }
  return best;
}
