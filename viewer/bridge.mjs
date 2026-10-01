import { spawn } from 'node:child_process';

export function runAnalyzer(analyzer, request, { signal, timeoutMs = 5000 } = {}) {
  return new Promise((resolve, reject) => {
    if (signal?.aborted) return reject(new Error('Analysis cancelled.'));
    const child = spawn(analyzer, ['--request-json'], { shell: false, windowsHide: true, stdio: ['pipe', 'pipe', 'pipe'] });
    let stdout = '';
    let stderr = '';
    let bytes = 0;
    let failure;
    const stop = (message) => {
      failure ??= new Error(message);
      child.kill('SIGKILL');
    };
    const abort = () => stop('Analysis cancelled.');
    signal?.addEventListener('abort', abort, { once: true });
    const timer = setTimeout(() => stop('Analyzer timed out.'), timeoutMs);
    child.stdout.setEncoding('utf8');
    child.stderr.setEncoding('utf8');
    child.stdout.on('data', chunk => {
      bytes += Buffer.byteLength(chunk);
      if (bytes > 32 * 1024 * 1024) stop('Analyzer output exceeds 32 MiB.');
      else stdout += chunk;
    });
    child.stderr.on('data', chunk => { if (stderr.length < 16384) stderr += chunk; });
    child.stdin.on('error', () => { /* The close event reports early process exit. */ });
    child.on('error', error => { failure = new Error(`Cannot launch analyzer: ${error.message}`); });
    child.on('close', code => {
      clearTimeout(timer);
      signal?.removeEventListener('abort', abort);
      if (failure) return reject(failure);
      try {
        const body = JSON.parse(stdout);
        const expectedCode = { ok: 0, syntax_error: 1, error: 2 }[body.status];
        if (code !== expectedCode || body.protocolVersion !== 1 || body.requestId !== request.requestId ||
            !Array.isArray(body.diagnostics) || !(body.dot === null || typeof body.dot === 'string') ||
            !body.nodeRanges || typeof body.nodeRanges !== 'object') {
          throw new Error('Invalid analyzer response.');
        }
        resolve(body);
      } catch (error) {
        reject(new Error(`Analyzer failed (exit ${code}): ${stderr.trim() || error.message}`));
      }
    });
    child.stdin.end(JSON.stringify(request));
  });
}
