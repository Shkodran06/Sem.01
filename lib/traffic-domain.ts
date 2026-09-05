import type { Movement, Side } from './traffic-controller.ts';

export type RoadUserKind = 'car' | 'bicycle' | 'bus' | 'truck' | 'articulated-truck';
export type SensorHealth = 'online' | 'degraded' | 'offline';

export type TrackedRoadUser = {
  id: string;
  kind: RoadUserKind;
  movement: Movement;
  frontProgressM: number;
  speedKmh: number;
  lastSeenMs: number;
  sensorConfidence: number;
};

export const USER_DIMENSIONS: Record<RoadUserKind, { lengthM: number; widthM: number; minSpeedKmh: number }> = {
  car: { lengthM: 4.6, widthM: 1.9, minSpeedKmh: 5 },
  bicycle: { lengthM: 1.9, widthM: 0.8, minSpeedKmh: 3 },
  bus: { lengthM: 12, widthM: 2.55, minSpeedKmh: 4 },
  truck: { lengthM: 10, widthM: 2.55, minSpeedKmh: 4 },
  'articulated-truck': { lengthM: 18.75, widthM: 2.55, minSpeedKmh: 3 },
};

export function rearProgressM(user: TrackedRoadUser): number {
  return user.frontProgressM - USER_DIMENSIONS[user.kind].lengthM;
}
export function occupiesProtectedZone(user: TrackedRoadUser, zoneStartM: number, zoneEndM: number): boolean {
  return user.frontProgressM > zoneStartM && rearProgressM(user) < zoneEndM;
}

export function trackingIsReliable(user: TrackedRoadUser, nowMs: number): boolean {
  return user.sensorConfidence >= 0.8 && nowMs - user.lastSeenMs <= 750;
}

export class OccupancyTracker {
  private readonly users = new Map<string, TrackedRoadUser>();

  update(user: TrackedRoadUser): void { this.users.set(user.id, user); }
  remove(id: string): boolean { return this.users.delete(id); }
  all(): TrackedRoadUser[] { return [...this.users.values()]; }
  occupants(zoneStartM: number, zoneEndM: number): TrackedRoadUser[] {
    return this.all().filter((user) => occupiesProtectedZone(user, zoneStartM, zoneEndM));
  }
  uncertain(nowMs: number): TrackedRoadUser[] { return this.all().filter((user) => !trackingIsReliable(user, nowMs)); }
  mayConfirmClear(zoneStartM: number, zoneEndM: number, nowMs: number): boolean {
    return this.occupants(zoneStartM, zoneEndM).length === 0 && this.uncertain(nowMs).length === 0;
  }
}

export type PedestrianRequest = { crossing: Side; requestedAtMs: number };

export class PedestrianQueue {
  private requests = new Map<Side, number>();
  request(crossing: Side, nowMs: number): void {
    if (!this.requests.has(crossing)) this.requests.set(crossing, nowMs);
  }
  has(crossing: Side): boolean { return this.requests.has(crossing); }
  takeOldest(): PedestrianRequest | null {
    const oldest = [...this.requests.entries()].sort((a, b) => a[1] - b[1])[0];
    if (!oldest) return null;
    this.requests.delete(oldest[0]);
    return { crossing: oldest[0], requestedAtMs: oldest[1] };
  }
  get size(): number { return this.requests.size; }
}
