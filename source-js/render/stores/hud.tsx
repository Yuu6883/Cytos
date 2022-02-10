import { createState, none } from '@hookstate/core';
import Client from '../client';
import { CytosTimings } from '../types';

export interface SimpleUserInfo {
    id: number;
    uid: string;
    name: string;
    color: string;
    avatar: string;
    provider: string;
    chatFlags: number;
}

export interface SimplePlayerInfo {
    id: number;
    pids: [number, number];
    name: string;
    skin1: string;
    skin2: string;
    color: string;
    guest?: boolean;
}

export interface ILeaderboardData {
    uid: number;
    score: number;
}

export interface IStats {
    fps: number;
    line: number;
    cells: number;
    score: number;
}

export interface INerdStats {
    version: string;
    map: [number, number];
    uptime: number;
    compile: string;
    rendercells: number;
    bandwidth: number;
    gpuBandwidth: number;
    timings?: CytosTimings;
}

export enum ChatType {
    NONE = 'NONE',
    SYSTEM = 'SYSTEM',
    GAME = 'GAME',
}

export const getChatTypeColor = (t: ChatType) => {
    if (t === ChatType.GAME) return '#49cbf5';
    if (t === ChatType.SYSTEM) return '#ac2a3d';
    return '#ffffff';
};

export interface ChatMessage {
    id: number;
    type?: ChatType;
    gain?: 'xp' | 'coin';
    amount?: number;
    system?: boolean;
    time: number; // Date.now()
    name?: string;
    color?: string;
    text: string;
    elem?: HTMLElement;
    jsx?: JSX.Element[];
}

export interface UserContext {
    empty?: boolean;
    unknown?: boolean;
    bot?: boolean;
    id?: number;
    pid?: number;
    cyt?: boolean;
    exp?: boolean;
    offline?: boolean;
    skinState?: number;
}

export const HUDStore = {
    visible: createState({
        all: true,
        chat: true,
        stats: true,
        mm: true,
        lb: true,
    }),
    chatInput: createState(false),
    menu: createState(true),
    leaderboard: createState<{ data: ILeaderboardData[] }>({ data: [] }),
    stats: createState<IStats>({
        fps: 0,
        line: 0,
        cells: 0,
        score: 0,
    }),
    nerdVisible: createState(true),
    nerdStats: createState<INerdStats>({
        version: 'N/A',
        map: [NaN, NaN],
        uptime: NaN,
        compile: 'N/A',
        rendercells: 0,
        bandwidth: 0,
        gpuBandwidth: 0,
        timings: null,
    }),
    displayIndex: createState<number>(0),
    messages: createState<{
        all: ChatMessage[];
        lobby: ChatMessage[];
        system: ChatMessage[];
    }>({ all: [], lobby: [], system: [] }),
    spectating: createState(false),
    ctxMenu: createState<{
        open: boolean;
        x: number;
        y: number;
        data: UserContext;
    }>({
        open: false,
        x: 0,
        y: 0,
        data: {},
    }),
};

export const addMsg = (msg: ChatMessage) => {
    const m = HUDStore.messages;
    if (!msg.type) msg.type = ChatType.NONE;
    const cat = [m.all];

    if (msg.name) msg.name = Client.instance.filterLongString(msg.name);

    switch (msg.type) {
        case ChatType.SYSTEM:
        case ChatType.GAME:
            cat.push(m.lobby, m.system);
            break;
    }

    msg.jsx = [<span>{msg.text}</span>];

    m.batch(() => {
        for (const c of cat) {
            if (c.length > 100) c.merge({ 0: none });
            c.merge([msg]);
        }
    });
};

export const sysMsg = (type: ChatType, msg: string, ...args: string[]) => {
    args.reduceRight(
        (_, arg, index) =>
            (msg = msg.replace(
                `$${index}`,
                Client.instance?.filterLongString(arg) || arg,
            )),
        '',
    );
    addMsg({
        type: type,
        system: true,
        text: msg,
        id: 0,
        time: Date.now(),
    });
};

export const clearChat = () => {
    HUDStore.messages.set({
        all: [],
        lobby: [],
        system: [],
    });
};

export const scoreToString = (score: number) => {
    if (score < 1000) return Math.round(score).toString();
    if (score < 1000000) return Math.round(score / 1000).toString() + 'K';
    if (score < 100000000) return (score / 1000000).toFixed(2) + 'M'; // 10M mass lmao
    return Math.round(score / 1000000) + 'M';
};

const DAY = 1000 * 60 * 60 * 24;

export const prettyTime = (dt: number, precise = false) => {
    if (isNaN(dt)) return 'NaN';
    if (dt < 1000) return Math.round(dt) + ' milliseconds';
    if (precise && dt < 3 * 60 * 1000) return (dt / 1000).toFixed(3) + ' seconds';
    if (dt < 60 * 1000) return Math.round(dt / 1000) + ' seconds';
    if (dt < 120 * 60 * 1000) return Math.round(dt / 1000 / 60) + ' minutes';
    if (dt < 48 * 60 * 60 * 1000) return Math.round(dt / 1000 / 60 / 60) + ' hours';
    return Math.round(dt / DAY) + ' days';
};

export const prettyMass = (m: number) =>
    m > 1000000
        ? (m / 1000000).toFixed(2) + 'M'
        : m > 1000
        ? (m / 1000).toFixed(1) + 'K'
        : Math.round(m).toString();

export const expireTime = (dt: number) => {
    if (dt < 0) return 'expiring';
    const day = ~~(dt / DAY);
    if (day > 3) return `${Math.round(dt / DAY)}days`;

    dt -= day * DAY;
    if (day) return `${day}day${Math.round(dt / 1000 / 60 / 60)}hr`;

    const hr = ~~(dt / 1000 / 60 / 60);
    if (hr >= 10) return `${Math.round(dt / 1000 / 60 / 60)}hrs`;
    dt -= hr * 1000 * 60 * 60;

    const min = ~~(dt / 1000 / 60);
    if (hr) return `${hr}hr${min}min`;

    dt -= min * 1000 * 60;
    const sec = ~~(dt / 1000);
    return `${min}:${sec.toString().padStart(2, '0')}`;
};

export const prettyBytes = (b: number) => {
    if (b < 1000) return `${~~b}B`;
    else if (b < 1000 * 1000) return `${Math.round(b / 1000)}KB`;
    else if (b < 5 * 1000 * 1000) return `${(b / 1000 / 1000).toFixed(2)}MB`;
    return `${Math.round(b / 1000 / 1000)}MB`;
};

const pn = (n = 0, s = 2) => n.toString().padStart(s, '0');

export const prettyDate = (d: Date) => {
    return (
        `[${d.getMonth() + 1}/${d.getDate()}/${d.getFullYear()} ` +
        `${pn(d.getHours())}:${pn(d.getMinutes())}:${pn(d.getSeconds())}]`
    );
};
