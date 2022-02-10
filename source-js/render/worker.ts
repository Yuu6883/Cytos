import { CytosInputData, CytosTimings, CytosVersion } from './types';

const ctx: Worker = self as any; // eslint-disable-line no-restricted-globals

interface CytosAddon {
    setInput(data: CytosInputData);

    setGameMode(mode: string);
    setThreads(threads: number);

    onBuffer(cb: (buffer: Buffer) => void);
    onInfo(cb: (info: object) => void);

    getTimings: () => CytosTimings;
    getVersion: () => CytosVersion;
}

const Cytos: CytosAddon = eval("require('../../build/Release/cytos-addon.node')");

ctx.postMessage({ version: Cytos.getVersion() });

Cytos.onBuffer(buf => ctx.postMessage(buf, [buf.buffer]));
Cytos.onInfo(info => ctx.postMessage(info));

setInterval(() => ctx.postMessage({ timings: Cytos.getTimings() }), 1000);

ctx.addEventListener(
    'message',
    (
        e: MessageEvent<{
            mode?: string;
            threads?: number;
            isBenchmark?: boolean;
            input?: CytosInputData;
        }>,
    ) => {
        const { data } = e;
        if (!data) return;
        const { mode, threads, isBenchmark, input } = data;

        if (mode !== undefined) {
            postMessage({ mode });
            if (mode && isBenchmark) {
                Cytos.setGameMode(`bench-${mode}`);
            } else Cytos.setGameMode(mode);
        }
        if (!isNaN(threads)) Cytos.setThreads(threads);
        if (input) Cytos.setInput(input);
    },
);

export default 0;
