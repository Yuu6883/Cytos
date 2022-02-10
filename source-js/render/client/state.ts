declare type DBInitFunc = () => Promise<IDBDatabase>;

export const initServerDB: DBInitFunc = () => {
    return new Promise((resolve, reject) => {
        const req = indexedDB.open('cytos-servers');
        req.onupgradeneeded = e => {
            const db = (e.target as any).result as IDBDatabase;
            db.createObjectStore('engine-data');
        };
        req.onsuccess = _ => resolve(req.result);
        req.onerror = reject;
    });
};

export const saveServer = (db: IDBDatabase, mode: string, buffer: Uint8Array) =>
    new Promise((resolve, reject) => {
        const tx = db.transaction(['engine-data'], 'readwrite');
        tx.objectStore('engine-data').put(buffer, mode); // overwrite
        tx.oncomplete = resolve;
        tx.onerror = reject;
    });

export const loadServer = async (db: IDBDatabase, mode: string) => {
    const data = await new Promise((resolve, reject) => {
        const tx = db.transaction(['engine-data'], 'readonly');
        const req = tx.objectStore('engine-data').get(mode);
        tx.oncomplete = () => resolve(req.result);
        tx.onerror = reject;
    });
    return data as Uint8Array;
};

export const deleteServer = (db: IDBDatabase, mode: string) => {
    new Promise((resolve, reject) => {
        const tx = db.transaction(['engine-data'], 'readwrite');
        tx.objectStore('engine-data').delete(mode);
        tx.oncomplete = resolve;
        tx.onerror = reject;
    });
};
