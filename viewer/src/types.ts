export type AnalysisView = 'syntax' | 'cfg';
export interface SourceRange { start: number; end: number }
export interface Diagnostic { code: string; message: string; range: SourceRange }
export interface AnalysisResponse {
  protocolVersion: 1;
  requestId: number;
  status: 'ok' | 'syntax_error' | 'error';
  diagnostics: Diagnostic[];
  dot: string | null;
  nodeRanges: Record<string, SourceRange>;
}
