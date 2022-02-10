import { createState } from '@hookstate/core';

export interface ServerData {
    name: string;
    region: string;
    players: number;
}

const MODES = ['ffa', 'instant', 'mega', 'omega', 'selffeed', 'ultra', 'rockslide'];
const BENCH = ['omega'];

export const ServerStore = createState<{ data: ServerData[] }>({
    data: MODES.map(name => ({ name, players: 0, region: 'local' })).concat(
        BENCH.map(name => ({ name, players: 0, region: 'benchmark' })),
    ),
});

const DefaultServer = {
    connected: false,
    name: '',
    mode: '',
    usage: 0,
};

export const CurrServer = createState<{
    connected: boolean;
    name: string;
    mode: string;
    usage: number;
}>({ ...DefaultServer });

export const OnGameDisconnect = () => CurrServer.set({ ...DefaultServer });
