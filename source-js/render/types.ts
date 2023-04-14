export interface CytosTimings {
    usage: number;
    threads: number;
    spawn_cells: number;
    handle_io: number;
    spawn_handles: number;
    update_cells: number;
    resolve_physics: number;
    io: number[];
    tree: number[];
    physics: number[];
    queries: number[];
    counter: number[];
}

export interface CytosRenderTimings {
    prep: number;
    update: number;
    remove: number;
    sort: number;
    buffer: number;
    upload: number;
    total: number;
}

export interface CytosVersion {
    version: string;
    timestamp: number;
    compile: string;
}

interface TabInputData {
    line: boolean;
    macro: boolean;
    spawn: boolean;
    splits: number;
    ejects: number;
    mouseX: number;
    mouseY: number;
}

export interface CytosInputData {
    spectate: number;
    activeTab: number;
    data: [TabInputData, TabInputData];
}
