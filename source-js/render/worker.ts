import { initServerDB, loadServer, saveServer } from './client/state';
import { CytosInputData, CytosTimings, CytosVersion } from './types';

const ctx: Worker = self as any; // eslint-disable-line no-restricted-globals

interface SaveResult {
    mode: string;
    buffer: Uint8Array;
}

interface CytosAddon {
    setInput(data: CytosInputData);

    setGameMode(mode: string);
    setThreads(threads: number);

    onBuffer(cb: (buffer: Buffer) => void);
    onInfo(cb: (info: object) => void);

    getTimings: () => CytosTimings;
    getVersion: () => CytosVersion;

    save: () => SaveResult;
    restore: (mode: string, buffer: Uint8Array) => boolean;
}

let db: IDBDatabase;

const Cytos: CytosAddon = eval("require('../../build/Release/cytos-addon.node')");

ctx.postMessage({ version: Cytos.getVersion() });

Cytos.onBuffer(buf => ctx.postMessage(buf, [buf.buffer]));
Cytos.onInfo(info => ctx.postMessage(info));

setInterval(() => ctx.postMessage({ timings: Cytos.getTimings() }), 1000);

initServerDB().then(o => (db = o));

type WorkerDataEvent = MessageEvent<{
    save?: boolean;
    restore?: string;
    mode?: string;
    threads?: number;
    isBenchmark?: boolean;
    input?: CytosInputData;
}>;

const afterSave = async (result: SaveResult) => {
    if (result) {
        await saveServer(db, result.mode, result.buffer);
        postMessage({ save: result.mode, bytes: result.buffer.byteLength });
    }
};

ctx.addEventListener('message', async (e: WorkerDataEvent) => {
    const { data } = e;
    if (!data || !db) return;

    const { save, restore, mode, threads, input } = data;

    if (mode !== undefined) {
        const result = Cytos.save();

        postMessage({ mode });
        Cytos.setGameMode(mode);

        if (mode) {
            const data = await loadServer(db, mode);
            if (data) {
                const restore = Cytos.restore(mode, data);
                if (restore) postMessage({ restore: mode, bytes: data.byteLength });
            }
        }

        await afterSave(result);
    }
    if (!isNaN(threads)) Cytos.setThreads(threads);
    if (input) Cytos.setInput(input);

    if (save) afterSave(Cytos.save());

    if (restore) {
        const data = await loadServer(db, restore);
        const result = Cytos.restore(restore, data);
        if (result) postMessage({ restore, bytes: data.byteLength });
    }
});

export default 0;
