export type FaultSeverity = 'information' | 'warning' | 'critical';
export type FaultCode =
  | 'UNEXPECTED_DIRECTION' | 'SENSOR_DEGRADED' | 'SENSOR_OFFLINE' | 'TRACK_LOST'
  | 'RED_LAMP_FAILURE' | 'GREEN_CONFLICT' | 'OUTPUT_FEEDBACK_MISMATCH'
  | 'COMMUNICATION_LOSS' | 'POWER_LOW' | 'WATCHDOG_FAILURE' | 'ZONE_BLOCKED';

export type Fault = { code: FaultCode; severity: FaultSeverity; source: string; occurredAtMs: number; active: boolean; acknowledged: boolean };

export const FAULT_SEVERITY: Record<FaultCode, FaultSeverity> = {
  UNEXPECTED_DIRECTION: 'information', SENSOR_DEGRADED: 'warning', SENSOR_OFFLINE: 'critical', TRACK_LOST: 'critical',
  RED_LAMP_FAILURE: 'critical', GREEN_CONFLICT: 'critical', OUTPUT_FEEDBACK_MISMATCH: 'critical',
  COMMUNICATION_LOSS: 'critical', POWER_LOW: 'warning', WATCHDOG_FAILURE: 'critical', ZONE_BLOCKED: 'critical',
};

export class FaultRegister {
  private faults = new Map<string, Fault>();
  raise(code: FaultCode, source: string, occurredAtMs: number): Fault {
    const key = `${code}:${source}`;
    const fault = this.faults.get(key) ?? { code, source, occurredAtMs, severity: FAULT_SEVERITY[code], active: true, acknowledged: false };
    fault.active = true;
    this.faults.set(key, fault);
    return fault;
  }
  acknowledge(code: FaultCode, source: string): void {
    const fault = this.faults.get(`${code}:${source}`); if (fault) fault.acknowledged = true;
  }
  clear(code: FaultCode, source: string, conditionSafe: boolean): boolean {
    const fault = this.faults.get(`${code}:${source}`);
    if (!fault || !conditionSafe || !fault.acknowledged) return false;
    fault.active = false; return true;
  }
  active(): Fault[] { return [...this.faults.values()].filter((fault) => fault.active); }
  hasCritical(): boolean { return this.active().some((fault) => fault.severity === 'critical'); }
}
