import type { Side } from './traffic-controller.ts';

export type SiteConfiguration = {
  siteName: string;
  enabledSides: Side[];
  speedLimitKmh: number;
  protectedZoneLengthM: number;
  allRedMs: number;
  redYellowMs: number;
  minGreenMs: number;
  maxGreenMs: number;
  yellowMs: number;
  pedestrianWalkingSpeedMps: number;
  pedestrianCrossingLengthM: number;
  sensorTimeoutMs: number;
};

export type ConfigurationIssue = { field: keyof SiteConfiguration; severity: 'error' | 'warning'; message: string };

export const DEMO_CONFIGURATION: SiteConfiguration = {
  siteName: 'Sem.01 Demo', enabledSides: ['north', 'east', 'south', 'west'], speedLimitKmh: 30,
  protectedZoneLengthM: 22, allRedMs: 1200, redYellowMs: 900, minGreenMs: 4000,
  maxGreenMs: 9000, yellowMs: 3000, pedestrianWalkingSpeedMps: 1.0,
  pedestrianCrossingLengthM: 12, sensorTimeoutMs: 750,
};

export function pedestrianClearanceMs(config: SiteConfiguration, marginMs = 1500): number {
  return Math.ceil((config.pedestrianCrossingLengthM / config.pedestrianWalkingSpeedMps) * 1000 + marginMs);
}
export function validateConfiguration(config: SiteConfiguration): ConfigurationIssue[] {
  const issues: ConfigurationIssue[] = [];
  const error = (field: keyof SiteConfiguration, message: string) => issues.push({ field, severity: 'error' as const, message });
  const warning = (field: keyof SiteConfiguration, message: string) => issues.push({ field, severity: 'warning' as const, message });
  if (!config.siteName.trim()) error('siteName', 'Site name is required.');
  if (!config.enabledSides.length) error('enabledSides', 'At least one approach must be enabled.');
  if (new Set(config.enabledSides).size !== config.enabledSides.length) error('enabledSides', 'Approaches must be unique.');
  if (config.speedLimitKmh <= 0 || config.speedLimitKmh > 80) error('speedLimitKmh', 'Demonstration speed must be between 1 and 80 km/h.');
  if (config.protectedZoneLengthM <= 0) error('protectedZoneLengthM', 'Protected-zone length must be positive.');
  if (config.allRedMs <= 0) error('allRedMs', 'All-red clearance must be positive.');
  if (config.redYellowMs <= 0) error('redYellowMs', 'Red-yellow time must be positive.');
  if (config.minGreenMs < 4000) error('minGreenMs', 'Minimum green must not be below the project lower bound of 4 seconds.');
  if (config.maxGreenMs < config.minGreenMs) error('maxGreenMs', 'Maximum green must not be shorter than minimum green.');
  if (config.yellowMs <= 0) error('yellowMs', 'Yellow time must be positive.');
  if (config.pedestrianWalkingSpeedMps <= 0) error('pedestrianWalkingSpeedMps', 'Walking speed must be positive.');
  if (config.pedestrianWalkingSpeedMps > 1.2) warning('pedestrianWalkingSpeedMps', 'Verify that the assumed walking speed also protects slower pedestrians.');
  if (config.pedestrianCrossingLengthM <= 0) error('pedestrianCrossingLengthM', 'Crossing length must be positive.');
  if (config.sensorTimeoutMs < 100 || config.sensorTimeoutMs > 5000) error('sensorTimeoutMs', 'Sensor timeout must be between 100 and 5000 ms.');
  return issues;
}
